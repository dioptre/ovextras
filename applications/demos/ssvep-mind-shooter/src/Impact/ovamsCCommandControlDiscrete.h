/// Control ship movements with VRPN using label predictions from the classifier
// If bTakeControl is false, the class will do nothing.

#ifndef __OpenViBEApplication_CCommandControlDiscrete_H__
#define __OpenViBEApplication_CCommandControlDiscrete_H__

#include "../ovamsICommandVRPNButton.h"

namespace OpenViBESSVEPMindShooter
{
	class CImpactApplication;

	class CCommandControlDiscrete : public ICommandVRPNButton
	{
		public:
			CCommandControlDiscrete(CImpactApplication* poApplication, OpenViBE::boolean bTakeControl);
			~CCommandControlDiscrete() {}

			void execute(int iButton, int iState);
		protected:
			OpenViBE::boolean m_bInControl;
	};
}


#endif // __OpenViBEApplication_CCommandControlDiscrete_H__
