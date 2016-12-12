
#if defined(TARGET_HAS_ThirdPartyOgre3DTerrain)

#include "ovamsCCommandReceiveTarget.h"
#include "ovamsCApplication.h"

using namespace OpenViBESSVEPMindShooter;

CCommandReceiveTarget::CCommandReceiveTarget(CApplication* poApplication)
	: ICommandVRPNButton(poApplication, "SSVEP_VRPN_TargetControl")
{
}

void CCommandReceiveTarget::execute(int iButton, int iState)
{
	dynamic_cast<CApplication*>(m_poApplication)->setTarget(iButton);
}

#endif