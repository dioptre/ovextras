
#if defined(TARGET_HAS_ThirdPartyOgre3DTerrain)

#include "ovamsCCommandStimulatorControl.h"
#include "ovamsCApplication.h"

#include <toolkit/ovtk_stimulations.h>

using namespace OpenViBESSVEPMindShooter;

CCommandStimulatorControl::CCommandStimulatorControl(CApplication* poApplication)
	: ICommandVRPNButton(poApplication, "SSVEP_VRPN_StimulatorControl")
{
}

void CCommandStimulatorControl::execute(int iButton, int iState)
{
	// only run the commands once, skip

	switch (iButton)
	{

		case 0:
			m_poApplication->startExperiment();
			m_poApplication->m_pStimulusSender->sendStimulation(OVTK_StimulationId_ExperimentStart);
			break;
			
		case 1:
			m_poApplication->stopExperiment();
			m_poApplication->m_pStimulusSender->sendStimulation(OVTK_StimulationId_ExperimentStop);
			break;
			
		case 2:
			m_poApplication->startFlickering();
			m_poApplication->m_pStimulusSender->sendStimulation(OVTK_StimulationId_VisualStimulationStart);
			break;
			
		case 3:
			m_poApplication->stopFlickering();
			m_poApplication->m_pStimulusSender->sendStimulation(OVTK_StimulationId_VisualStimulationStop);
			break;
			
		default:
			m_poApplication->getLogManager() << OpenViBE::Kernel::LogLevel_Error << "[ERROR] Unknown command\n";
			break;

	}

}

#endif