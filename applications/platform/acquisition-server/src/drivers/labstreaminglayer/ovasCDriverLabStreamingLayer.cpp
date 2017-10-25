
#if defined(TARGET_HAS_ThirdPartyLSL)

/*
 *
 * Notes: This code should be kept compatible with changes to LSL Output plugin in OpenViBE Acquisition Server,
 * and LSL Export box in Designer.
 *
 * This driver makes a few assumptions:
 *
 * Signal streams
 *    - are float32
 *    - dense, i.e. there are no dropped or extra samples in them
 *    - the driver fills a sample block consequently until either the block has been filled or
 *      time runs out. In case of the latter, the chunk is padded with NaNs.
 * Markers
 *    - are int32
 *    - not dense
 *    - markers are retimed wrt the current signal chunk
 *
 * Other notes: Due to network delays, it may be better to disable drift correction or
 * set the drift tolerance to high. This is because
 *  1) Signals are assumed dense, i.e. LSL pull results in samples stamped t,t+1,t+2,...
 *  2) However, Acquisition Server works in real time, so if there is a network delay between
 *     the samples stamped t and t+1, this delay does not indicate a delay in the original signal
 *     and it might not be correct to pad the corresponding signal block during the delay.
 *
 * Todo. It might make sense to improve this driver in the future to take into account the LSL
 * stamps when constructing each signal block. This is currently not done.
 *

 *
 *
 *
 */

#include <limits> // for NaN

#include "ovasCDriverLabStreamingLayer.h"
#include "ovasCConfigurationLabStreamingLayer.h"

#include <system/ovCTime.h>

#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>

#include <lsl_cpp.h>

#include <cmath>
#include <algorithm>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

// In seconds
static const float64 g_LSL_samplingRateEstimatationDuration = 2.0;
static const float64 g_LSL_resolveTimeOut = 2.0;
static const float64 g_LSL_openTimeOut = 2.0;
static const float64 g_LSL_readTimeOut = 2.0;

static uint32 getClosestMultipleOf25(uint32 value)
{
	return uint32(((value + 12) / 25) * 25);
}

static uint32 getClosestPowerOf2(uint32 value)
{
	uint32 l_ui32BitShiftCount = 0;
	while ((1LL << l_ui32BitShiftCount) < value)
	{
		l_ui32BitShiftCount++;
	}

	if (l_ui32BitShiftCount > 0)
	{
		uint32 l_ui32Greater = uint32(1LL << l_ui32BitShiftCount);
		uint32 l_ui32Lesser = l_ui32Greater >> 1;
		if (l_ui32Greater - value < value - l_ui32Lesser)
		{
			return l_ui32Greater;
		}
		else
		{
			return l_ui32Lesser;
		}
	}

	return uint32(1LL << l_ui32BitShiftCount);
}

static uint32 getSmartFallbackSamplingRateEstimate(uint32 ui32LSLSamplingRate, bool bRoundToPowerOfTwoAndMultiple25=true)
{
	if (bRoundToPowerOfTwoAndMultiple25)
	{
		uint32 l_ui32ClosestMultipleOf25 = ::getClosestMultipleOf25(ui32LSLSamplingRate);
		uint32 l_ui32ClosestPowerOfTwo = ::getClosestPowerOf2(ui32LSLSamplingRate);
		uint32 l_ui32ClosestMultipleOf25Diff = uint32(std::abs(int32(l_ui32ClosestMultipleOf25) - int32(ui32LSLSamplingRate)));
		uint32 l_ui32ClosestPowerOfTwoDiff = uint32(std::abs(int32(l_ui32ClosestPowerOfTwo) - int32(ui32LSLSamplingRate)));
		if (l_ui32ClosestMultipleOf25Diff < l_ui32ClosestPowerOfTwoDiff)
		{
			ui32LSLSamplingRate = l_ui32ClosestMultipleOf25;
		}
		else
		{
			ui32LSLSamplingRate = l_ui32ClosestPowerOfTwo;
		}
	}
	return ui32LSLSamplingRate;
}

//___________________________________________________________________//
//                                                                   //

CDriverLabStreamingLayer::CDriverLabStreamingLayer(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_LabStreamingLayer", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_ui32LSLFallbackSamplingRate(0)
	,m_pSample(NULL)
	,m_pBuffer(NULL)
	,m_pSignalInlet(NULL)
	,m_pMarkerInlet(NULL)
	,m_bLimitSpeed(false)
{
	// The following class allows saving and loading driver settings from the acquisition server .conf file
	m_oSettings.add("Header", &m_oHeader);
	// To save your custom driver settings, register each variable to the SettingsHelper
	m_oSettings.add("LimitSpeed",       &m_bLimitSpeed);
	m_oSettings.add("SignalStreamName", &m_sSignalStream);
	m_oSettings.add("SignalStreamID",   &m_sSignalStreamID);
	m_oSettings.add("MarkerStreamName", &m_sMarkerStream);
	m_oSettings.add("MarkerStreamID",   &m_sMarkerStreamID);
	m_oSettings.add("FallbackSamplingRate", &m_ui32LSLFallbackSamplingRate);
	m_oSettings.load();

	/*
	for (uint32 i = 1; i < 260; i++)
	{
		rDriverContext.getLogManager() << LogLevel_Error << i << " --> " << getSmartFallbackSamplingRateEstimate(i) << "\n";
	}
	*/
}

CDriverLabStreamingLayer::~CDriverLabStreamingLayer(void)
{
}

const char* CDriverLabStreamingLayer::getName(void)
{
	return "LabStreamingLayer (LSL)";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverLabStreamingLayer::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	if(m_rDriverContext.isConnected()) return false;

	// Find the signal stream
	const std::vector<lsl::stream_info> l_vStreams = lsl::resolve_stream("name", m_sSignalStream.toASCIIString(), 1, g_LSL_resolveTimeOut);
	if(!l_vStreams.size()) {
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error resolving signal stream with name [" << m_sSignalStream.toASCIIString() << "]\n";
		return false;
	}
	for(uint32 i=0;i<l_vStreams.size();i++)
	{
		m_oSignalStream = l_vStreams[i];
		if(l_vStreams[i].source_id() == std::string(m_sSignalStreamID.toASCIIString()))
		{
			// This is the best one
			break;
		}
		m_rDriverContext.getLogManager() << LogLevel_Trace << "Finally resolved signal stream to " << m_oSignalStream.name().c_str() << ", id " << m_oSignalStream.source_id().c_str() << "\n";
	}

	// Find the marker stream
	if(m_sMarkerStream!=CString("None"))
	{
		const std::vector<lsl::stream_info> l_vMarkerStreams = lsl::resolve_stream("name", m_sMarkerStream.toASCIIString(), 1, g_LSL_resolveTimeOut);
		if(!l_vMarkerStreams.size()) {
			m_rDriverContext.getLogManager() << LogLevel_Error <<  "Error resolving marker stream with name [" << m_sMarkerStream.toASCIIString() << "]\n";
			return false;
		}
		for(uint32 i=0;i<l_vMarkerStreams.size();i++)
		{
			m_oMarkerStream = l_vMarkerStreams[i];
			if(l_vMarkerStreams[i].source_id() == std::string(m_sMarkerStreamID.toASCIIString()))
			{
				// This is the best one
				break;
			}
		}
		m_rDriverContext.getLogManager() << LogLevel_Trace << "Finally resolved marker stream to " << m_oMarkerStream.name().c_str() << ", id " << m_oMarkerStream.source_id().c_str() << "\n";
	}
	else
	{
		// We do not have a marker stream. This is ok.
	}

	// Get the channel names. We open a temporary inlet for this, it will be closed after going out of scope. The actual inlet will be opened in start().
	m_rDriverContext.getLogManager() << LogLevel_Trace << "Polling channel names\n";
	lsl::stream_inlet l_oTmpInlet(m_oSignalStream, 360, 0, false);
	try
	{
		l_oTmpInlet.open_stream(g_LSL_openTimeOut);
	}
	catch(...)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error <<  "Failed to open signal stream with name [" << m_oSignalStream.name().c_str() << "] for polling channel names\n";
		return false;
	}

	lsl::stream_info l_oFullInfo;
	try
	{
		l_oFullInfo = l_oTmpInlet.info(g_LSL_readTimeOut);
	}
	catch(...)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error <<  "Timeout reading full stream info for [" << m_oSignalStream.name().c_str() << "] for polling channel names\n";
		return false;
	}

	// Now sets channel count and names
	m_oHeader.setChannelCount(m_oSignalStream.channel_count());

	const lsl::xml_element l_oChannels = l_oFullInfo.desc().child("channels");
	lsl::xml_element l_oChannel = l_oChannels.child("channel");
	for(uint32 i=0;i<m_oHeader.getChannelCount(); i++)
	{
		const char *l_sLabel = l_oChannel.child_value("label");

		if(l_sLabel)
		{
			m_oHeader.setChannelName(i, l_sLabel);
		}

		l_oChannel = l_oChannel.next_sibling("channel");
	}

	// Buffer to store a single sample
	m_pBuffer = new float32[m_oHeader.getChannelCount()];
	if(!m_pBuffer)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error <<  "Memory allocation problem\n";
		return false;
	}

	// Buffer to store the signal chunk
	m_pSample=new float32[m_oHeader.getChannelCount()*ui32SampleCountPerSentBlock];
	if(!m_pSample)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error <<  "Memory allocation problem\n";
		return false;
	}

	// Now sets sampling rate

	uint32 l_ui32LSLSamplingRate = static_cast<uint32>(m_oSignalStream.nominal_srate());
	if(l_ui32LSLSamplingRate == 0)
	{
		// Check GUI fallback
		if(m_ui32LSLFallbackSamplingRate != 0)
		{
			l_ui32LSLSamplingRate = m_ui32LSLFallbackSamplingRate;
			m_rDriverContext.getLogManager() << LogLevel_Info << "LSL sampling rate is not defined, falls back to [" << l_ui32LSLSamplingRate << "] - You can change this in the driver settings\n";
		}
		else
		{
			// Autodetection was requested, let`s go for a round
			m_rDriverContext.getLogManager() << LogLevel_Info << "LSL sampling rate is not defined, falls back to [" << CString("Autodetection") << "] - You can change this in the driver settings\n";

			try
			{
				l_oTmpInlet.open_stream(g_LSL_openTimeOut);

				// First drop all available samples
				while (l_oTmpInlet.samples_available() != 0)
				{
					l_oTmpInlet.pull_sample(m_pBuffer, m_oHeader.getChannelCount(), g_LSL_readTimeOut);
				}

				// Now capture incoming samples over a period of 2 secs
				uint32 l_ui32SampleCount = 0;
				float64 l_f64StartCaptureTime = l_oTmpInlet.pull_sample(m_pBuffer, m_oHeader.getChannelCount(), g_LSL_readTimeOut);
				float64 l_f64CurrentCaptureTime = l_f64StartCaptureTime;
				while(l_f64CurrentCaptureTime != 0.0 && l_f64CurrentCaptureTime-l_f64StartCaptureTime < g_LSL_samplingRateEstimatationDuration)
				{
					float l_fPulledSampleTime = l_oTmpInlet.pull_sample(m_pBuffer, m_oHeader.getChannelCount(), g_LSL_readTimeOut);
					if (l_fPulledSampleTime != 0)
					{
						l_ui32SampleCount++;
						l_f64CurrentCaptureTime = l_fPulledSampleTime;
					}
					else
					{
						m_rDriverContext.getLogManager() << LogLevel_Error << "Timed out while receiving samples from LSL stream, avoiding auto detection.\n";
						delete[] m_pBuffer;
						delete[] m_pSample;
						return false;
					}
				}

				// Finally set estimated sampling rate
				float64 l_f64Duration = l_f64CurrentCaptureTime-l_f64StartCaptureTime;
				uint32 l_ui32RawLSLSamplingRate = uint32(l_ui32SampleCount / l_f64Duration);
				uint32 l_ui32SmartLSLSamplingRate = getSmartFallbackSamplingRateEstimate(l_ui32RawLSLSamplingRate);

				if (l_ui32SmartLSLSamplingRate != l_ui32RawLSLSamplingRate)
				{
					m_rDriverContext.getLogManager() << LogLevel_Info << "Acquired " << l_ui32SampleCount << " on a duration of " << l_f64Duration << " seconds, sampling rate might be approx " << l_ui32RawLSLSamplingRate << " hz (adjusted to " << l_ui32SmartLSLSamplingRate << " hz to conform with typical EEG sampling rates).\n";
				}
				else
				{
					m_rDriverContext.getLogManager() << LogLevel_Info << "Acquired " << l_ui32SampleCount << " on a duration of " << l_f64Duration << " seconds, sampling rate might be approx " << l_ui32SmartLSLSamplingRate << " hz.\n";
				}

				l_ui32LSLSamplingRate = l_ui32SmartLSLSamplingRate;
			}
			catch(...)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "Failed to detected actual LSL sampling rate\n";
				delete [] m_pBuffer;
				delete [] m_pSample;
				return false;
			}
		}
	}

	m_oHeader.setSamplingFrequency(l_ui32LSLSamplingRate);

	if(l_ui32LSLSamplingRate != m_oSignalStream.nominal_srate())
	{
		m_rDriverContext.getLogManager() << LogLevel_Info
			<< "Opened an LSL stream with " << m_oSignalStream.channel_count()
			<< " channels and a nominal rate of " << m_oSignalStream.nominal_srate() << " hz.\n";
	}
	else
	{
		m_rDriverContext.getLogManager() << LogLevel_Info
			<< "Opened an LSL stream with " << m_oSignalStream.channel_count()
			<< " channels and a nominal rate of " << m_oSignalStream.nominal_srate() << " hz"
			<< " adjusted to " << l_ui32LSLSamplingRate << " hz.\n";
	}

	// Stores parameters
	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;
	return true;
}

boolean CDriverLabStreamingLayer::start(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) return false;

	m_pSignalInlet = new lsl::stream_inlet(m_oSignalStream, 360, 0, false);
	if(!m_pSignalInlet)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error getting signal inlet for [" << m_oSignalStream.name().c_str()  << "]\n";
		return false;
	}

	try
	{
		m_pSignalInlet->open_stream(g_LSL_openTimeOut);
	}
	catch(...)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error <<  "Failed to open signal stream with name [" << m_oSignalStream.name().c_str() << "]\n";
		return false;
	}

	if(m_sMarkerStream!=CString("None"))
	{
		m_pMarkerInlet = new lsl::stream_inlet(m_oMarkerStream, 360, 0, false);
		if(!m_pMarkerInlet)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error <<  "Error getting marker inlet for [" << m_oMarkerStream.name().c_str()  << "]\n";
			return false;
		}

		try
		{
			m_pMarkerInlet->open_stream(g_LSL_openTimeOut);
		}
		catch(...)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "Failed to open marker stream with name [" << m_oSignalStream.name().c_str()  << "]\n";
			return false;
		}
	}

	m_ui64StartTime=System::Time::zgetTime();
	m_ui64SampleCount = 0;

	return true;
}

boolean CDriverLabStreamingLayer::loop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return true;

	const uint32 l_ui32nChannels = m_oHeader.getChannelCount();

	bool l_bTimeOut = false;
	uint32 l_ui32TimeOutAt = 0;
	bool l_bBlockStartTimeSet = false;
	float64 l_f64BlockStartTime = 0;

	// receive signal from the stream
	for(uint32 i=0;i<m_ui32SampleCountPerSentBlock;i++)
	{
		double captureTime = 0;
		try
		{
			captureTime = m_pSignalInlet->pull_sample(m_pBuffer,l_ui32nChannels,g_LSL_readTimeOut);
		}
		catch (...)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error <<  "Failed to get signal sample from [" << m_oSignalStream.name().c_str()  << "]\n";
			return false;
		}
		if(captureTime!=0)
		{
			if(!l_bBlockStartTimeSet)
			{
				l_bBlockStartTimeSet = true;
				l_f64BlockStartTime = captureTime;
				//m_rDriverContext.getLogManager() << LogLevel_Info << "First cap time is " << captureTime << "\n";
			}

			// Sample ok, fill
			for(uint32 j=0;j<l_ui32nChannels;j++)
			{
				m_pSample[j*m_ui32SampleCountPerSentBlock+i] = m_pBuffer[j];
			}
		}
		else
		{
			// Timeout
			l_ui32TimeOutAt = i;
			l_bTimeOut = true;
			break;
		}
	}

	if(l_bTimeOut)
	{
		// We fill the rest of the buffer with NaNs
		for(uint32 i=l_ui32TimeOutAt;i<m_ui32SampleCountPerSentBlock;i++)
		{
			for(uint32 j=0;j<l_ui32nChannels;j++)
			{
				m_pSample[j*m_ui32SampleCountPerSentBlock+i] =  std::numeric_limits<float>::quiet_NaN();
			}
		}
		m_rDriverContext.getLogManager() << LogLevel_Info << "Timeout reading sample from " << l_ui32TimeOutAt << ", filled rest of block with NaN\n";
	}

	if(m_bLimitSpeed)
	{
		// If we were faster than what the AS expects, sleep.
		// n.b. This sleep may not be accurate on Windows (it may oversleep)
		const uint64 l_ui64TimeNow = System::Time::zgetTime() - m_ui64StartTime;
		const uint64 l_ui64TimeLimitForBuffer = ITimeArithmetics::sampleCountToTime(m_oHeader.getSamplingFrequency(), m_ui64SampleCount+m_ui32SampleCountPerSentBlock);

		if(l_ui64TimeNow < l_ui64TimeLimitForBuffer)
		{
			const uint64 l_ui64TimeToSleep = l_ui64TimeLimitForBuffer - l_ui64TimeNow;

//			std::cout << "PostNap " << ITimeArithmetics::timeToSeconds(l_ui64TimeToSleep)*1000 << "ms at " << m_ui64SampleCount+m_ui32SampleCountPerSentBlock << "\n";

			System::Time::zsleep(l_ui64TimeToSleep);
		}
	}

	m_ui64SampleCount += m_ui32SampleCountPerSentBlock;

	m_pCallback->setSamples(m_pSample);

	// receive and pass markers. Markers are timed wrt the beginning of the signal block.
	OpenViBE::CStimulationSet l_oStimulationSet;
	if(m_pMarkerInlet)
	{
		while(true)
		{
			int32 l_i32Marker;
			double captureTime = 0;
			try
			{
				captureTime = m_pMarkerInlet->pull_sample(&l_i32Marker,1,0); // timeout is 0 here on purpose, either we have markers or not
			}
			catch(...)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "Failed to get marker from [" << m_oMarkerStream.name().c_str()  << "]\n";
				return false;
			}
			if(captureTime==0)
			{
				// no more markers available at the moment
				break;
			}
			// float64 l_f64Correction = m_pMarkerInlet->time_correction();
			// float64 l_f64StimTime = captureTime + l_f64Correction - l_f64FirstCaptureTime;

			// For openvibe, we need to set the stimulus time relative to the start of the chunk
			const float64 l_f64StimTime = captureTime - l_f64BlockStartTime;

			// m_rDriverContext.getLogManager() << LogLevel_Info << "Got a marker " << l_i32Marker << " at " << captureTime << " -> "
			//	<< l_f64StimTime << "s."
			//	<< "\n";

			l_oStimulationSet.appendStimulation(static_cast<uint64>(l_i32Marker), ITimeArithmetics::secondsToTime(l_f64StimTime), 0);
			// std::cout << "date " << l_f64StimTime << "\n";
		}
	}

	m_pCallback->setStimulationSet(l_oStimulationSet);

	// LSL is not forcing the sample stream to confirm to the nominal sample rate. Hence, data may be incoming
	// with slower or faster speed than implied by the rate (a little like reading from a file). In some
	// cases it may be meaningful to disable the following drift correction from the AS settings.
	m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());

	return true;
}

boolean CDriverLabStreamingLayer::stop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return false;

	if(m_pSignalInlet)
	{
		m_pSignalInlet->close_stream();

		delete m_pSignalInlet;
		m_pSignalInlet = NULL;
	}

	if(m_pMarkerInlet)
	{
		m_pMarkerInlet->close_stream();

		delete m_pMarkerInlet;
		m_pMarkerInlet = NULL;
	}

	return true;
}

boolean CDriverLabStreamingLayer::uninitialize(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) return false;

	if(m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer=NULL;
	}

	if(m_pSample)
	{
		delete[] m_pSample;
		m_pSample=NULL;
	}

	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //
boolean CDriverLabStreamingLayer::isConfigurable(void)
{
	return true; // change to false if your device is not configurable
}

boolean CDriverLabStreamingLayer::configure(void)
{
	// Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
	CConfigurationLabStreamingLayer m_oConfiguration(m_rDriverContext,
		OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-LabStreamingLayer.ui",
		m_oHeader,
		m_bLimitSpeed,
		m_sSignalStream,
		m_sSignalStreamID,
		m_sMarkerStream,
		m_sMarkerStreamID,
		m_ui32LSLFallbackSamplingRate);

	if(!m_oConfiguration.configure(m_oHeader))
	{
		return false;
	}
	m_oSettings.save();

	return true;
}

#endif
