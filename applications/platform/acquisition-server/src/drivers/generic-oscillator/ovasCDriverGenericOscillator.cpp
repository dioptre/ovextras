#include "ovasCDriverGenericOscillator.h"
#include "ovasCConfigurationDriverGenericOscillator.h"

#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>

#include <system/ovCTime.h>

#include <cmath>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;

//___________________________________________________________________//
//                                                                   //

CDriverGenericOscillator::CDriverGenericOscillator(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_GenericOscillator", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_ui64TotalSampleCount(0)
	,m_bSendPeriodicStimulations(false)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericOscillator::CDriverGenericOscillator\n";

	m_oHeader.setSamplingFrequency(512);
	m_oHeader.setChannelCount(4);

	m_oSettings.add("Header", &m_oHeader);
	m_oSettings.add("SendPeriodicStimulations", &m_bSendPeriodicStimulations);
	m_oSettings.load();
}

void CDriverGenericOscillator::release(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericOscillator::release\n";

	delete this;
}

const char* CDriverGenericOscillator::getName(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericOscillator::getName\n";

	return "Generic Oscillator";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverGenericOscillator::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericOscillator::initialize\n";

	if(m_rDriverContext.isConnected()) { return false; }

	if(!m_oHeader.isChannelCountSet()
	 ||!m_oHeader.isSamplingFrequencySet())
	{
		return false;
	}

	for (uint32 i = 0; i < m_oHeader.getChannelCount(); i++)
	{
		m_oHeader.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Base);
		if (CString(m_oHeader.getChannelName(i)) == CString(""))
		{
			std::stringstream ss; ss << "Oscillator " << (i + 1);
			m_oHeader.setChannelName(i, ss.str().c_str());
		}
	}

	m_vSample.resize(m_oHeader.getChannelCount()*ui32SampleCountPerSentBlock);

	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;

	return true;
}

boolean CDriverGenericOscillator::start(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericOscillator::start\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	m_ui64TotalSampleCount=0;

	return true;
}

boolean CDriverGenericOscillator::loop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Debug << "CDriverGenericOscillator::loop\n";

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

		const uint64 l_ui64Elapsed = System::Time::zgetTime() - m_rDriverContext.getStartTime();
		const uint64 l_ui64SamplesNeededSoFar = ITimeArithmetics::timeToSampleCount(m_oHeader.getSamplingFrequency(), l_ui64Elapsed);
		if (l_ui64SamplesNeededSoFar <= m_ui64TotalSampleCount)
		{
			// Too early
			return true;
		}
		const uint32 l_ui32RemainingSamples = static_cast<uint32>(l_ui64SamplesNeededSoFar - m_ui64TotalSampleCount);
		if (l_ui32RemainingSamples * m_oHeader.getChannelCount() > m_vSample.size())
		{
			m_vSample.resize(l_ui32RemainingSamples * m_oHeader.getChannelCount());
		}

		// std::cout << "At " << ITimeArithmetics::timeToSeconds(l_ui64Elapsed) * 1000 << "ms, remaining " << l_ui32RemainingSamples << " samples\n";
		for (uint32 i = 0; i<l_ui32RemainingSamples; i++)
		{
			for (uint32 j = 0; j<m_oHeader.getChannelCount(); j++)
			{
				const float64 l_f64Value=
					::sin((m_ui64TotalSampleCount*(j+1)*12.3)/m_oHeader.getSamplingFrequency())+
					::sin((m_ui64TotalSampleCount*(j+1)* 4.5)/m_oHeader.getSamplingFrequency())+
					::sin((m_ui64TotalSampleCount*(j+1)*67.8)/m_oHeader.getSamplingFrequency());
				m_vSample[j*l_ui32RemainingSamples + i] = (float32)l_f64Value;
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

boolean CDriverGenericOscillator::stop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericOscillator::stop\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return false; }
	return true;
}

boolean CDriverGenericOscillator::uninitialize(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericOscillator::uninitialize\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverGenericOscillator::isConfigurable(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericOscillator::isConfigurable\n";

	return true;
}

boolean CDriverGenericOscillator::configure(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericOscillator::configure\n";

	CConfigurationDriverGenericOscillator m_oConfiguration(m_rDriverContext, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-Generic-Oscillator.ui", 
		m_bSendPeriodicStimulations);

	if(m_oConfiguration.configure(m_oHeader)) 
	{
		m_oSettings.save();
		return true;
	}

	return false;
}
