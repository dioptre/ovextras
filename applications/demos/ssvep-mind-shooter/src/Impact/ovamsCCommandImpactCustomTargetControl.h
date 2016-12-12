/// Used to insert enemy ships into the scene upon receiving a trigger

#ifndef __OpenViBEApplication_CCommandCustomImpactTargetControl_
#define __OpenViBEApplication_CCommandCustomImpactTargetControl_

#include "../ovamsICommandVRPNButton.h"
#include "../ovamsCVRPNServer.h"

namespace OpenViBESSVEPMindShooter
{
	class CImpactApplication;

	class CCommandImpactCustomTargetControl : public ICommandVRPNButton
	{
		public:
			CCommandImpactCustomTargetControl(CImpactApplication* poApplication);
			~CCommandImpactCustomTargetControl() {}

			void execute(int iButton, int iState);
			void processFrame();

		private:
			CVRPNServer* m_poVRPNServer;

	};
}


#endif // __OpenViBEApplication_CCommandImpactTargetControl_
