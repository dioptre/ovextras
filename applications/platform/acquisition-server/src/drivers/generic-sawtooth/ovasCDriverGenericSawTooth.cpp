#include "ovasCDriverGenericSawTooth.h"
#include "../ovasCConfigurationBuilder.h"

#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>

#include <system/ovCTime.h>

#include <cmath>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;

//___________________________________________________________________//
//                                                                   //

CDriverGenericSawTooth::CDriverGenericSawTooth(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_pCallback(NULL)
	,m_ui32ExternalBlockSize(0)
	,m_vSample(NULL)
	,m_ui64TotalSampleCount(0)
	,m_ui64StartTime(0)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericSawTooth::CDriverGenericSawTooth\n";

	m_oHeader.setSamplingFrequency(512);
	m_oHeader.setChannelCount(1);
	m_oHeader.setChannelName(0, "Sawtooth");
	m_oHeader.setChannelUnits(0, OVTK_UNIT_Volts, OVTK_FACTOR_Base);
}

void CDriverGenericSawTooth::release(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericSawTooth::release\n";

	delete this;
}

const char* CDriverGenericSawTooth::getName(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericSawTooth::getName\n";

	return "Generic Saw Tooth";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverGenericSawTooth::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericSawTooth::initialize\n";

	if(m_rDriverContext.isConnected()) { return false; }

	if(!m_oHeader.isChannelCountSet()
	 ||!m_oHeader.isSamplingFrequencySet())
	{
		return false;
	}


	m_vSample.resize(m_oHeader.getChannelCount()*ui32SampleCountPerSentBlock);

	m_pCallback=&rCallback;

	m_ui32ExternalBlockSize=ui32SampleCountPerSentBlock;

	return true;
}

boolean CDriverGenericSawTooth::start(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericSawTooth::start\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	m_ui64TotalSampleCount=0;
	m_ui64StartTime = System::Time::zgetTime();

	return true;
}

#include <iostream>

boolean CDriverGenericSawTooth::loop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Debug << "CDriverGenericSawTooth::loop\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return true; }

	// Find out how many samples to send
	const uint64 l_ui64Elapsed = System::Time::zgetTime() - m_ui64StartTime;
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

	// Generate the data
	// The result should be a linear ramp between [0,1] for each block sent *out* by the acquisition server
	for(uint32 i=0; i<l_ui32RemainingSamples; i++)
	{
		for (uint32 j = 0; j<m_oHeader.getChannelCount(); j++)
		{
			m_vSample[j*l_ui32RemainingSamples + i] = float32(m_ui64TotalSampleCount % m_ui32ExternalBlockSize) / (m_ui32ExternalBlockSize - 1);
		}
		m_ui64TotalSampleCount++;
	}

	m_pCallback->setSamples(&m_vSample[0],l_ui32RemainingSamples);

	m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());

	return true;
}

boolean CDriverGenericSawTooth::stop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericSawTooth::stop\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return false; }
	return true;
}

boolean CDriverGenericSawTooth::uninitialize(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericSawTooth::uninitialize\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverGenericSawTooth::isConfigurable(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericSawTooth::isConfigurable\n";

	return false;
}

boolean CDriverGenericSawTooth::configure(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericSawTooth::configure\n";

	return false;
}
