#include "ovpCBoxAlgorithmTimeout.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Stimulation;

boolean CBoxAlgorithmTimeout::initialize(void)
{
	m_ui32TimeoutState = 0;

	m_oStimulationEncoder.initialize(*this,0);
	
	m_ui64Timeout = static_cast<uint64>(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)) << 32;
	m_ui64StimulationToSend = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

	m_ui64LastTimePolled = 0;
	m_ui64PreviousTime = 0;

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmTimeout::uninitialize(void)
{
	m_oStimulationEncoder.uninitialize();

	return true;
}
/*******************************************************************************/


boolean CBoxAlgorithmTimeout::processClock(IMessageClock& rMessageClock)
{
	// if there was nothing received on the input for a period of time we raise the
	// timeout flag and let the box send a stimulation
	if (m_ui32TimeoutState == 0 && getPlayerContext().getCurrentTime() > m_ui64LastTimePolled + m_ui64Timeout)
	{
		getLogManager() << LogLevel_Info << "Timeout reached" << "\n";
		m_ui32TimeoutState = 1;
	}

	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/


uint64 CBoxAlgorithmTimeout::getClockFrequency(void)
{
	return 10LL<<32; // the box clock frequency
}
/*******************************************************************************/


boolean CBoxAlgorithmTimeout::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	// every time we receive input we store the last kernel time
	m_ui64LastTimePolled = getPlayerContext().getCurrentTime();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmTimeout::process(void)
{

	IStimulationSet* l_pStimulationSet = m_oStimulationEncoder.getInputStimulationSet();
	l_pStimulationSet->clear();

	const uint64 l_ui64StimulationDate = this->getPlayerContext().getCurrentTime();

	// If the timeout is reached we send the stimulation on the output 0
	if (m_ui32TimeoutState == 1)
	{
		l_pStimulationSet->appendStimulation(m_ui64StimulationToSend, l_ui64StimulationDate, 0);
		m_ui32TimeoutState = 2;
	}

	// we need to send an empty chunk even if there's no stim
	m_oStimulationEncoder.encodeBuffer();
	this->getDynamicBoxContext().markOutputAsReadyToSend(0, m_ui64PreviousTime, l_ui64StimulationDate);

	m_ui64PreviousTime = l_ui64StimulationDate;

	return true;
}
