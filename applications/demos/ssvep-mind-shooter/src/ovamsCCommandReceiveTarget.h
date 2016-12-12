#ifndef __OpenViBEApplication_CCommandReceiveTarget_H__
#define __OpenViBEApplication_CCommandReceiveTarget_H__

#include "ovamsICommandVRPNButton.h"

namespace OpenViBESSVEPMindShooter
{
	class CApplication;

	class CCommandReceiveTarget : public ICommandVRPNButton
	{
		public:
			CCommandReceiveTarget(CApplication* poApplication);
			~CCommandReceiveTarget() {}

			void execute(int iButton, int iState);


	};
}


#endif // __OpenViBEApplication_CCommandReceiveTarget_H__
