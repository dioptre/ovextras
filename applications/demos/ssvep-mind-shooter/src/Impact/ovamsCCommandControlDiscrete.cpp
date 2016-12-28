
#if defined(TARGET_HAS_ThirdPartyOgre3DTerrain)

#include "ovamsCCommandControlDiscrete.h"
#include "ovamsCImpactApplication.h"

#include <CEGUI.h>
#if !((CEGUI_VERSION_MAJOR > 0) || (CEGUI_VERSION_MINOR >= 8))
namespace CEGUI
{
    typedef CEGUI::UVector2 USize;
};
#endif

using namespace OpenViBESSVEPMindShooter;

CCommandControlDiscrete::CCommandControlDiscrete(CImpactApplication* poApplication, OpenViBE::boolean bTakeControl)
	: ICommandVRPNButton(poApplication, "SSVEP_VRPN_DiscreteControl"),
	m_bInControl(bTakeControl)
{
}

void CCommandControlDiscrete::execute(int iButton, int iState)
{
	if (!m_bInControl) 
	{
		return;
	}

	CImpactApplication* l_poImpactApplication = dynamic_cast<CImpactApplication*>(m_poApplication);

	const OpenViBE::boolean l_bParalyzed = false;
	if (l_bParalyzed)
	{
		// In this mode, the ship does not move but the prediction is displayed instead. May be useful for debugging.
		char text[512];
		sprintf(text, "Button %d", iButton);

		float pos = (iButton / 3.0f) * 0.85f;

		l_poImpactApplication->getInstructionWindow()->setSize(CEGUI::USize(CEGUI::UDim(0.10f, 0), CEGUI::UDim(0.10f, 0)));
		l_poImpactApplication->getInstructionWindow()->setPosition(CEGUI::UVector2(CEGUI::UDim(pos, 0), CEGUI::UDim(0.5f, 0)));
		l_poImpactApplication->getInstructionWindow()->setText(text);
		l_poImpactApplication->getInstructionWindow()->show();

		return;
	}

	switch (iButton)
	{
		case 0:
			// Nop
			break;
		case 1:
			l_poImpactApplication->getShip()->shoot();
			break;
		case 2:
			l_poImpactApplication->getShip()->move( -6 );
			break;
		case 3:
			l_poImpactApplication->getShip()->move( 6 );
			break;
		default:
			m_poApplication->getLogManager() << OpenViBE::Kernel::LogLevel_Warning << "Unhandled button " << iButton << " received\n";
			break;
	}
}

#endif
