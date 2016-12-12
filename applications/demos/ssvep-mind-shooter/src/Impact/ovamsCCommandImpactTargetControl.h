#ifndef __OpenViBEApplication_CCommandImpactTargetControl_
#define __OpenViBEApplication_CCommandImpactTargetControl_

#include "../ovamsICommandVRPNButton.h"
#include "../ovamsCVRPNServer.h"

namespace OpenViBESSVEPMindShooter
{
	class CImpactApplication;

	class CCommandImpactTargetControl : public ICommandVRPNButton
	{
		public:
			CCommandImpactTargetControl(CImpactApplication* poApplication);
			~CCommandImpactTargetControl() {}

			void execute(int iButton, int iState);
			void processFrame();

		private:
			CVRPNServer* m_poVRPNServer;

	};
}


#endif // __OpenViBEApplication_CCommandImpactTargetControl_
