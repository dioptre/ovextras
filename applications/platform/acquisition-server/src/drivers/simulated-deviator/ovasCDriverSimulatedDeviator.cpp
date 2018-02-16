#include "ovasCDriverSimulatedDeviator.h"
#include "ovasCConfigurationDriverSimulatedDeviator.h"

#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>

#include <system/ovCTime.h>
#include <system/ovCMath.h>

#include <cmath>
#include <algorithm>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;

//___________________________________________________________________//
//                                                                   //

CDriverSimulatedDeviator::CDriverSimulatedDeviator(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_SimulatedDeviator", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_ui64TotalSampleCount(0)
	,m_ui64StartTime(0)
	,m_bSendPeriodicStimulations(false)
	,m_Offset(0)
	,m_Spread(0.1)
	,m_MaxDev(3)
	,m_Pullback(0.001)
	,m_Update(0.1)
	,m_Wavetype(0)	// 0 = square, 1 = sine
	,m_FreezeFrequency(0)
	,m_FreezeDuration(0)
	,m_NextFreezeTime(0)
	,m_gen{m_rd()}
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverSimulatedDeviator::CDriverSimulatedDeviator\n";

	m_oHeader.setSamplingFrequency(512);
	m_oHeader.setChannelCount(3);

	m_oSettings.add("Header", &m_oHeader);
	m_oSettings.add("SendPeriodicStimulations", &m_bSendPeriodicStimulations);
	m_oSettings.add("Offset",   &m_Offset);
	m_oSettings.add("Spread",   &m_Spread);
	m_oSettings.add("MaxDev",   &m_MaxDev);
	m_oSettings.add("Pullback", &m_Pullback);
	m_oSettings.add("Update",   &m_Update);
	m_oSettings.add("Wavetype", &m_Wavetype);
	m_oSettings.add("FreezeFrequency", &m_FreezeFrequency);
	m_oSettings.add("FreezeDuration", &m_FreezeDuration);
	m_oSettings.load();
}

void CDriverSimulatedDeviator::release(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverSimulatedDeviator::release\n";

	delete this;
}

const char* CDriverSimulatedDeviator::getName(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverSimulatedDeviator::getName\n";

	return "Simulated Deviator";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverSimulatedDeviator::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverSimulatedDeviator::initialize\n";

	if(m_rDriverContext.isConnected()) { return false; }

	if(!m_oHeader.isChannelCountSet()
	 ||!m_oHeader.isSamplingFrequencySet())
	{
		return false;
	}

	m_oHeader.setChannelCount(3);

	m_oHeader.setChannelName(0, (m_Wavetype == 0 ? "SquareWave" : "SineWave"));
	m_oHeader.setChannelName(1, "SamplingRate");
	m_oHeader.setChannelName(2, "DriftMs");

	m_oHeader.setChannelUnits(0, OVTK_UNIT_Volts,   OVTK_FACTOR_Micro);
	m_oHeader.setChannelUnits(1, OVTK_UNIT_Hertz,   OVTK_FACTOR_Base);
	m_oHeader.setChannelUnits(2, OVTK_UNIT_Second,  OVTK_FACTOR_Milli);

	m_vSample.resize(m_oHeader.getChannelCount()*ui32SampleCountPerSentBlock);

	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;

	return true;
}

boolean CDriverSimulatedDeviator::start(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverSimulatedDeviator::start\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	m_ui64TotalSampleCount=0;
	m_ui64TotalSampleCountReal=0;
	m_ui64StartTime = System::Time::zgetTime();
	m_ui64LastAdjustment = m_ui64StartTime;

	m_usedSamplingFrequency = m_oHeader.getSamplingFrequency() + m_Offset;
	m_oHeader.setChannelCount(3);

	return true;
}

boolean CDriverSimulatedDeviator::loop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Debug << "CDriverSimulatedDeviator::loop\n";

	if(!m_rDriverContext.isConnected()) { return false; }

	if (m_rDriverContext.isStarted())
	{
	

		// Generate the contents we want to send next
		CStimulationSet l_oStimulationSet;
		if (m_bSendPeriodicStimulations)
		{
			l_oStimulationSet.setStimulationCount(1);
			l_oStimulationSet.setStimulationIdentifier(0, 0);
			l_oStimulationSet.setStimulationDate(0, 0);
			l_oStimulationSet.setStimulationDuration(0, 0);
		}

		const uint64 now = System::Time::zgetTime();

		// Is it time to freeze?
		if(m_FreezeDuration>0 && m_FreezeFrequency>0)
		{
			if(m_NextFreezeTime>0 && now>=m_NextFreezeTime ) 
			{
				// Simulate a freeze by setting all samples as sent up to the de-freeze point
				const uint64 freezeDurationFixedPoint = ITimeArithmetics::secondsToTime(m_FreezeDuration);
				const uint64 samplesToSkip = ITimeArithmetics::timeToSampleCount(m_oHeader.getSamplingFrequency(), freezeDurationFixedPoint);
				m_ui64TotalSampleCount += samplesToSkip;
				m_ui64TotalSampleCountReal += samplesToSkip;
			}
			if(now>=m_NextFreezeTime) 
			{
				// Compute the next time to freeze; simulate a Poisson process
				const float64 secondsUntilNext = -std::log(1.0 - System::Math::randomFloat32BetweenZeroAndOne()) / m_FreezeFrequency;
				const uint64 secondsUntilNextFixedPoint = ITimeArithmetics::secondsToTime(secondsUntilNext);

				m_NextFreezeTime = now + secondsUntilNextFixedPoint;
			}
		}

		// Drift the sampling frequency?
		if(now-m_ui64LastAdjustment > ITimeArithmetics::secondsToTime(m_Update))
		{
			// Make the sampling frequency random walk up or down
			if(m_Spread>0) 
			{
				std::normal_distribution<> distro{0,m_Spread};
				const float64 jitter = distro(m_gen);
				m_usedSamplingFrequency += jitter;
			}

			// Use linear interpolation to pull the drifting sample frequency towards the center frequency
			m_usedSamplingFrequency = m_Pullback * (m_oHeader.getSamplingFrequency() + m_Offset) + (1-m_Pullback) * m_usedSamplingFrequency;

			// Make sure the result stays bounded
			m_usedSamplingFrequency = std::min(m_usedSamplingFrequency, m_oHeader.getSamplingFrequency() + m_Offset + m_MaxDev);
			m_usedSamplingFrequency = std::max(m_usedSamplingFrequency, m_oHeader.getSamplingFrequency() + m_Offset - m_MaxDev);
			m_usedSamplingFrequency = std::max(m_usedSamplingFrequency, 0.0);

			m_ui64LastAdjustment = now;
			// std::cout << "freq " << m_usedSamplingFrequency << "\n";
		}

		const uint64 l_ui64Elapsed = now - m_ui64StartTime;
		const uint64 l_ui64SamplesNeededSoFar = static_cast<uint64>(m_usedSamplingFrequency * ITimeArithmetics::timeToSeconds(l_ui64Elapsed));
		const uint64 l_ui64SamplesNeededSoFarReal = static_cast<uint64>(m_oHeader.getSamplingFrequency() * ITimeArithmetics::timeToSeconds(l_ui64Elapsed));
		if (l_ui64SamplesNeededSoFar <= m_ui64TotalSampleCount)
		{
			if(l_ui64SamplesNeededSoFarReal>m_ui64TotalSampleCountReal)
			{
				m_ui64TotalSampleCountReal = l_ui64SamplesNeededSoFarReal;
			}
			// Too early
			return true;
		}
		const uint32 l_ui32RemainingSamples = static_cast<uint32>(l_ui64SamplesNeededSoFar - m_ui64TotalSampleCount);
		if (l_ui32RemainingSamples * m_oHeader.getChannelCount() > m_vSample.size())
		{
			m_vSample.resize(l_ui32RemainingSamples * m_oHeader.getChannelCount());
		}

		float64 driftMeasure = 0;
		if(l_ui64SamplesNeededSoFarReal > l_ui64SamplesNeededSoFar)
		{
			driftMeasure = -ITimeArithmetics::timeToSeconds(ITimeArithmetics::sampleCountToTime(m_oHeader.getSamplingFrequency(), l_ui64SamplesNeededSoFarReal - l_ui64SamplesNeededSoFar));
		}
		else
		{
			driftMeasure = ITimeArithmetics::timeToSeconds(ITimeArithmetics::sampleCountToTime(m_oHeader.getSamplingFrequency(), l_ui64SamplesNeededSoFar - l_ui64SamplesNeededSoFarReal));
		}

		for (uint32 i = 0; i<l_ui32RemainingSamples; i++)
		{
			float64 l_f64Value;
			if(m_Wavetype==0)
			{
				const uint64_t sampleTimeInSeconds = ((ITimeArithmetics::sampleCountToTime(m_oHeader.getSamplingFrequency(), m_ui64TotalSampleCountReal)) >> 32);
				l_f64Value = (sampleTimeInSeconds % 2 == 0 ? -1 : 1);
			}
			else
			{
				const float64 l_f64pi = 3.14159265358979323846;
				l_f64Value = std::sin( 2 * l_f64pi  * m_ui64TotalSampleCountReal / static_cast<float64>(m_oHeader.getSamplingFrequency()) );
			}

			m_vSample[0*l_ui32RemainingSamples+i] = static_cast<float32>(l_f64Value);

			m_vSample[1*l_ui32RemainingSamples+i] = static_cast<float32>(m_usedSamplingFrequency);

			m_vSample[2*l_ui32RemainingSamples+i] = static_cast<float32>(driftMeasure * 1000.0);

			if(l_ui64SamplesNeededSoFarReal >= l_ui64SamplesNeededSoFar)
			{
				m_ui64TotalSampleCountReal++;
			}
			else
			{
				// nop: we don't move the 'real' process forward if the sampler is in advance, it gets a duplicate sample on purpose, 
				// we assume that the hardware sampling process has not had time to change state.
				// @todo this implementation is not quite correct... if we take twice the amount of samples (512hz->1024hz), 
				// we'd expect each sample to be replicated twice but we observe that only approximately. The sinusoid 1hz stays correct nevertheless.
			}

			m_ui64TotalSampleCount++;
		}

		m_pCallback->setSamples(&m_vSample[0], l_ui32RemainingSamples);
		m_pCallback->setStimulationSet(l_oStimulationSet);
		m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());

	}
	else
	{
		if(m_rDriverContext.isImpedanceCheckRequested())
		{
			for(uint32 j=0; j<m_oHeader.getChannelCount(); j++)
			{
				m_rDriverContext.updateImpedance(j, 1);
			}
		}
	}

	return true;
}

boolean CDriverSimulatedDeviator::stop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverSimulatedDeviator::stop\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return false; }
	return true;
}

boolean CDriverSimulatedDeviator::uninitialize(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverSimulatedDeviator::uninitialize\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverSimulatedDeviator::isConfigurable(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverSimulatedDeviator::isConfigurable\n";

	return true;
}

boolean CDriverSimulatedDeviator::configure(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverSimulatedDeviator::configure\n";

	CConfigurationDriverSimulatedDeviator m_oConfiguration(m_rDriverContext, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-Simulated-Deviator.ui", 
		m_bSendPeriodicStimulations
		,m_Offset
		,m_Spread
		,m_MaxDev
		,m_Pullback
		,m_Update		
		,m_Wavetype
		,m_FreezeFrequency
		,m_FreezeDuration
		);

	if(m_oConfiguration.configure(m_oHeader)) 
	{
		m_oSettings.save();
		return true;
	}

	return false;
}
