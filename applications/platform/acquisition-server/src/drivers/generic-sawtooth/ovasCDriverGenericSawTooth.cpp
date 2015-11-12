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
	,m_ui32SampleCountPerSentBlock(0)
	,m_pSample(NULL)
	,m_ui32TotalSampleCount(0)
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

	m_pSample=new float32[m_oHeader.getChannelCount()*ui32SampleCountPerSentBlock];
	if(!m_pSample)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Sawtooth: Memory allocation error\n";
		return false;
	}

	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;

	return true;
}

boolean CDriverGenericSawTooth::start(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericSawTooth::start\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	m_ui32TotalSampleCount=0;
	m_ui64StartTime=System::Time::zgetTime();

	return true;
}

#include <iostream>

boolean CDriverGenericSawTooth::loop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Debug << "CDriverGenericSawTooth::loop\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return true; }

	// Generate the data
	for(uint32 j=0; j<m_oHeader.getChannelCount(); j++)
	{
		for(uint32 i=0; i<m_ui32SampleCountPerSentBlock; i++)
		{
			m_pSample[j*m_ui32SampleCountPerSentBlock+i]=float32(i)/(m_ui32SampleCountPerSentBlock-1);
		}
	}

	// If we're early, sleep before sending. Otherwise, push the chunk out immediately
	const uint64 l_ui64CurrentTime = System::Time::zgetTime() - m_ui64StartTime;
	const uint64 l_ui64NextTime = ITimeArithmetics::sampleCountToTime(m_oHeader.getSamplingFrequency(), m_ui32TotalSampleCount+m_ui32SampleCountPerSentBlock);
	if(l_ui64NextTime>l_ui64CurrentTime)
	{
		const uint64 l_ui64SleepTime = l_ui64NextTime - l_ui64CurrentTime;
		System::Time::zsleep(l_ui64SleepTime);
	}

#ifdef TIMINGDEBUG
	m_rDriverContext.getLogManager() << LogLevel_Info << "At " << ITimeArithmetics::timeToSeconds(l_ui64CurrentTime)*1000 << "ms filling for " 
		<< ITimeArithmetics::timeToSeconds(l_ui64NextTime)*1000 << "ms  -> nSamples = " << m_ui32TotalSampleCount + m_ui32SampleCountPerSentBlock << "\n";
#endif


	m_pCallback->setSamples(m_pSample);

	m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());

	m_ui32TotalSampleCount+=m_ui32SampleCountPerSentBlock;

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

	delete [] m_pSample;
	m_pSample=NULL;
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
