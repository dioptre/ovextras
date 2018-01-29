#include "ovasCDriverGenericTimeSignal.h"
#include "../ovasCConfigurationBuilder.h"

#include "ovasCConfigurationGenericTimeSignal.h"

#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>

#include <system/ovCTime.h>

#include <cmath>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;

//___________________________________________________________________//
//                                                                   //

CDriverGenericTimeSignal::CDriverGenericTimeSignal(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_pCallback(NULL)
	,m_oSettings("AcquisitionServer_Driver_GenericTimeSignal", m_rDriverContext.getConfigurationManager())
	,m_ui64TotalSampleCount(0)
	,m_ui64StartTime(0)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericTimeSignal::CDriverGenericTimeSignal\n";

	m_oHeader.setSamplingFrequency(512);
	m_oHeader.setChannelCount(1);
	m_oHeader.setChannelName(0, "Time(s)");
	m_oHeader.setChannelUnits(0, OVTK_UNIT_Second, OVTK_FACTOR_Base);

	m_oSettings.add("Header", &m_oHeader);
	m_oSettings.load();
}

void CDriverGenericTimeSignal::release(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericTimeSignal::release\n";

	delete this;
}

const char* CDriverGenericTimeSignal::getName(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericTimeSignal::getName\n";

	return "Generic Time Signal";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverGenericTimeSignal::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericTimeSignal::initialize\n";

	if(m_rDriverContext.isConnected()) { return false; }

	if(!m_oHeader.isChannelCountSet()
	 ||!m_oHeader.isSamplingFrequencySet())
	{
		return false;
	}

	m_pCallback=&rCallback;

	return true;
}

boolean CDriverGenericTimeSignal::start(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericTimeSignal::start\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	m_ui64TotalSampleCount=0;
	m_ui64StartTime = System::Time::zgetTime();

	return true;
}

#include <iostream>

boolean CDriverGenericTimeSignal::loop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Debug << "CDriverGenericTimeSignal::loop\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return true; }

	const uint64 ui64TimeNow = System::Time::zgetTime();

	// Find out how many samples to send; note that we always just send 1 at a time with this driver
	const uint64 l_ui64Elapsed = ui64TimeNow - m_ui64StartTime;
	const uint64 l_ui64SamplesNeededSoFar = (m_oHeader.getSamplingFrequency() * l_ui64Elapsed) >> 32;
	if (l_ui64SamplesNeededSoFar <= m_ui64TotalSampleCount)
	{
		// Too early
		return true;
	}

	const float32 timeNow = static_cast<float32>(ITimeArithmetics::timeToSeconds(ui64TimeNow));

	m_pCallback->setSamples(&timeNow, 1);

	m_ui64TotalSampleCount++;

	m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());

	return true;
}

boolean CDriverGenericTimeSignal::stop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericTimeSignal::stop\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return false; }
	return true;
}

boolean CDriverGenericTimeSignal::uninitialize(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericTimeSignal::uninitialize\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverGenericTimeSignal::isConfigurable(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericTimeSignal::isConfigurable\n";

	return true;
}

boolean CDriverGenericTimeSignal::configure(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericTimeSignal::configure\n";

	CConfigurationGenericTimeSignal m_oConfiguration(m_rDriverContext,
		OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-Generic-TimeSignal.ui");

	if(!m_oConfiguration.configure(m_oHeader))
	{
		return false;
	}

	m_oSettings.save();

	return true;

}
