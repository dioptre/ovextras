#ifndef __OpenViBEApplication_CCommandStimulatorControl_H__
#define __OpenViBEApplication_CCommandStimulatorControl_H__

#include "ovamsICommandVRPNButton.h"

namespace OpenViBESSVEPMindShooter
{
	class CCommandStimulatorControl : public ICommandVRPNButton
	{
		public:
			CCommandStimulatorControl(CApplication* poApplication);
			~CCommandStimulatorControl() {};

			void execute(int iButton, int iState);
	};
}


#endif // __OpenViBEApplication_CCommandStimulatorControl_H__
