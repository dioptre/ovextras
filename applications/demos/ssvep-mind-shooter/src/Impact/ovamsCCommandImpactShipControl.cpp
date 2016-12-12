
#if defined(TARGET_HAS_ThirdPartyOgre3DTerrain)

#include "ovamsCCommandImpactShipControl.h"
#include "ovamsCImpactApplication.h"

using namespace OpenViBESSVEPMindShooter;

CCommandImpactShipControl::CCommandImpactShipControl(CImpactApplication* poApplication)
	: ICommandVRPNButton(poApplication, "SSVEP_VRPN_ShipControl")
{
}

void CCommandImpactShipControl::execute(int iButton, int iState)
{
	CImpactApplication* l_poImpactApplication = dynamic_cast<CImpactApplication*>(m_poApplication);

	switch (iButton)
	{
		case 0:
			l_poImpactApplication->getShip()->shoot();
			break;
		case 1:
			l_poImpactApplication->getShip()->move( -6 );
			break;
		case 2:
			l_poImpactApplication->getShip()->move( 6 );
			break;
	}
}

#endif