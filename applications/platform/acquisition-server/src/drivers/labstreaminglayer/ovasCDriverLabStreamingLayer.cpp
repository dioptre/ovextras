
#if defined(TARGET_HAS_ThirdPartyLSL)

#include <limits> // for NaN

#include "ovasCDriverLabStreamingLayer.h"
#include "ovasCConfigurationLabStreamingLayer.h"

#include <system/Time.h>

#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>

#include <lsl_cpp.h>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

//___________________________________________________________________//
//                                                                   //

CDriverLabStreamingLayer::CDriverLabStreamingLayer(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_LabStreamingLayer", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_pSample(NULL)
	,m_pBuffer(NULL)
	,m_pInlet(NULL)
	,m_pMarkerInlet(NULL)
{	
	// The following class allows saving and loading driver settings from the acquisition server .conf file
	m_oSettings.add("Header", &m_oHeader);
	// To save your custom driver settings, register each variable to the SettingsHelper
	m_oSettings.add("StreamName", &m_sStream);
	m_oSettings.add("MarkerStreamName", &m_sMarkerStream);
	m_oSettings.load();	
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

	std::vector<lsl::stream_info> l_vStreams = lsl::resolve_stream("name", m_sStream.toASCIIString(), 1, 1.0);
	if(!l_vStreams.size()) {
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error opening signal stream with name [" << m_sStream.toASCIIString() << "]\n";
		return false;
	}
	m_oStream = l_vStreams[0];

	if(m_sMarkerStream!=CString(""))
	{
		std::vector<lsl::stream_info> l_vMarkerStreams = lsl::resolve_stream("name", m_sMarkerStream.toASCIIString(), 1, 1.0);
		if(!l_vMarkerStreams.size()) {
			m_rDriverContext.getLogManager() << LogLevel_Error <<  "Error opening marker stream with name [" << m_sMarkerStream.toASCIIString() << "]\n";
			return false;
		}
		m_oMarkerStream = l_vMarkerStreams[0];
	} else {
		// We do not have a marker stream. This is ok.
	}

	m_oHeader.setChannelCount(m_oStream.channel_count());
	m_oHeader.setSamplingFrequency(static_cast<uint32>(m_oStream.nominal_srate()));

	// std::cout << "nCh " << m_oHeader.getChannelCount() << "\n";
	// std::cout << "freq " << m_oHeader.getSamplingFrequency() << "\n";

	// Buffer to store a single sample
	m_pBuffer = new float32[m_oHeader.getChannelCount()];

	// Buffer to store the signal
	m_pSample=new float32[m_oHeader.getChannelCount()*ui32SampleCountPerSentBlock];
	if(!m_pSample)
	{
		delete [] m_pSample;
		m_pSample=NULL;
		return false;
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

	m_pInlet = new lsl::stream_inlet(m_oStream);
	if(!m_pInlet)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error getting signal inlet for [" << m_oStream.name().c_str()  << "]\n";
		return false;
	}

	try {
		m_pInlet->open_stream(10);
	} catch(...) {
		m_rDriverContext.getLogManager() << LogLevel_Error <<  "Failed to open signal stream with name [" << m_oStream.name().c_str() << "]\n";
		return false;
	}

	if(m_sMarkerStream!=CString("")) 
	{
		m_pMarkerInlet = new lsl::stream_inlet(m_oMarkerStream);
		if(!m_pMarkerInlet)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error <<  "Error getting marker inlet for [" << m_oMarkerStream.name().c_str()  << "]\n";
			return false;
		}

		try {
			m_pMarkerInlet->open_stream(10);
		} catch(...) {
			m_rDriverContext.getLogManager() << LogLevel_Error << "Failed to open marker stream with name [" << m_oStream.name().c_str()  << "]\n";
			return false;
		}
	}

	return true;
}

boolean CDriverLabStreamingLayer::loop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return true;

	const uint32 l_ui32nChannels = m_oHeader.getChannelCount();
	
	bool l_bCaptureTimeSet = false;
	float64 l_f64FirstCaptureTime = 0;

	// receive signal from the stream
	for(uint32 i=0;i<m_ui32SampleCountPerSentBlock;i++)
	{
		double captureTime = 0;
		try {
			captureTime = m_pInlet->pull_sample(m_pBuffer,l_ui32nChannels,10);
		} catch (...) {
			m_rDriverContext.getLogManager() << LogLevel_Error <<  "Failed to get signal sample from [" << m_oStream.name().c_str()  << "]\n";
			return false;
		}
		if(captureTime==0) 
		{
			// Timeout
			for(uint32 j=0;j<l_ui32nChannels;j++)
			{
				m_pSample[j*m_ui32SampleCountPerSentBlock+i] =  std::numeric_limits<float>::quiet_NaN();
			}
			continue;
		}
		else if(!l_bCaptureTimeSet) {
			l_bCaptureTimeSet = true;
			l_f64FirstCaptureTime = captureTime;
		}

		// Sample ok, fill
		for(uint32 j=0;j<l_ui32nChannels;j++)
		{
			m_pSample[j*m_ui32SampleCountPerSentBlock+i] = m_pBuffer[j];
		}
	}

	m_pCallback->setSamples(m_pSample);

	// LSL is not forcing the sample stream to confirm to the nominal sample rate. Hence, data may be incoming
	// with slower or faster speed than implied by the rate (a little like reading from a file). For this
	// reason, we do not apply drift correction. 
	// m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());

	// receive and pass markers. Markers are timed wrt the beginning of the signal block.
	OpenViBE::CStimulationSet l_oStimulationSet;
	if(m_pMarkerInlet)
	{
		while(1) 
		{
			int32 l_i32Marker;
			double captureTime = 0;
			try {
				captureTime = m_pMarkerInlet->pull_sample(&l_i32Marker,1,0);
			} catch(...) {
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
			const float64 l_f64StimTime = captureTime - l_f64FirstCaptureTime;

			l_oStimulationSet.appendStimulation(static_cast<uint64>(l_i32Marker), ITimeArithmetics::secondsToTime(l_f64StimTime), 0);
			// std::cout << "date " << l_f64StimTime << "\n";
		}
	}

	m_pCallback->setStimulationSet(l_oStimulationSet);

	return true;
}

boolean CDriverLabStreamingLayer::stop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return false;

	if(m_pInlet)
	{
		m_pInlet->close_stream();

		delete m_pInlet;
		m_pInlet = NULL;
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

	delete [] m_pSample;
	m_pSample=NULL;
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
		m_sStream,
		m_sMarkerStream);
	
	if(!m_oConfiguration.configure(m_oHeader))
	{
		return false;
	}
	m_oSettings.save();
	
	return true;
}

#endif