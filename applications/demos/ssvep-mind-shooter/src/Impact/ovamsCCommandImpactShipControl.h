/// Control ship movements with VRPN

#ifndef __OpenViBEApplication_CCommandImpactShipControl_H__
#define __OpenViBEApplication_CCommandImpactShipControl_H__

#include "../ovamsICommandVRPNButton.h"

namespace OpenViBESSVEPMindShooter
{
	class CImpactApplication;

	class CCommandImpactShipControl : public ICommandVRPNButton
	{
		public:
			CCommandImpactShipControl(CImpactApplication* poApplication);
			~CCommandImpactShipControl() {}

			void execute(int iButton, int iState);

	};
}


#endif // __OpenViBEApplication_CCommandImpactShipControl_H__
