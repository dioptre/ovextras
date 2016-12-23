/// Control ship movements with the keyboard

#ifndef __OpenViBEApplication_CCommandControlDiscreteOIS_H__
#define __OpenViBEApplication_CCommandControlDiscreteOIS_H__

#include <map>

#include "../ovamsICommandOIS.h"

namespace OpenViBESSVEPMindShooter
{
	class CImpactApplication;

	class CCommandControlDiscreteOIS : public ICommandOIS
	{
	public:
		CCommandControlDiscreteOIS(CImpactApplication* poApplication);
		~CCommandControlDiscreteOIS() {}

		void processFrame();

		void receiveKeyPressedEvent( const OIS::KeyCode oKey );
		void receiveKeyReleasedEvent( const OIS::KeyCode oKey );
	private:
		std::map< OIS::KeyCode, bool > m_vKeyPressed;




	};
}


#endif // __OpenViBEApplication_CCommandControlDiscreteOIS_H__
