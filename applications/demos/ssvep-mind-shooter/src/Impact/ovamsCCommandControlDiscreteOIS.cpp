
#if defined(TARGET_HAS_ThirdPartyOgre3DTerrain)

#include "ovamsCCommandControlDiscreteOIS.h"
#include "ovamsCImpactApplication.h"

using namespace OpenViBESSVEPMindShooter;

CCommandControlDiscreteOIS::CCommandControlDiscreteOIS(CImpactApplication* poApplication)
	: ICommandOIS(poApplication)
{
}

void CCommandControlDiscreteOIS::processFrame()
{
	ICommandOIS::processFrame();

	CImpactApplication* l_poShooterApplication = dynamic_cast<CImpactApplication*>(m_poApplication);

	if (m_vKeyPressed[OIS::KC_UP] || m_vKeyPressed[OIS::KC_DOWN])
	{
		l_poShooterApplication->getShip()->shoot();
	}

	if (m_vKeyPressed[OIS::KC_LEFT])
	{
		l_poShooterApplication->getShip()->move( -6 );
	}

	if (m_vKeyPressed[OIS::KC_RIGHT])
	{
		l_poShooterApplication->getShip()->move( 6 );
	}
}

void CCommandControlDiscreteOIS::receiveKeyPressedEvent( const OIS::KeyCode oKey )
{
	if (oKey == OIS::KC_UP)
	{
		m_vKeyPressed[OIS::KC_UP] = true;
	}

	if (oKey == OIS::KC_DOWN)
	{
		m_vKeyPressed[OIS::KC_DOWN] = true;
	}

	if (oKey == OIS::KC_LEFT)
	{
		m_vKeyPressed[OIS::KC_LEFT] = true;
	}

	if (oKey == OIS::KC_RIGHT)
	{
		m_vKeyPressed[OIS::KC_RIGHT] = true;
	}
}

void CCommandControlDiscreteOIS::receiveKeyReleasedEvent( const OIS::KeyCode oKey )
{
	if (oKey == OIS::KC_UP)
	{
		m_vKeyPressed[OIS::KC_UP] = false;
	}

	if (oKey == OIS::KC_DOWN)
	{
		m_vKeyPressed[OIS::KC_DOWN] = false;
	}

	if (oKey == OIS::KC_LEFT)
	{
		m_vKeyPressed[OIS::KC_LEFT] = false;
	}

	if (oKey == OIS::KC_RIGHT)
	{
		m_vKeyPressed[OIS::KC_RIGHT] = false;
	}}

#endif