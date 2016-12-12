#ifndef __OpenViBEApplication_CCommandFeedbackHandler_H__
#define __OpenViBEApplication_CCommandFeedbackHandler_H__

#include "../ovamsICommandVRPNAnalog.h"

namespace OpenViBESSVEP
{
	class CImpactApplication;

	class CCommandFeedbackHandler : public ICommandVRPNAnalog
	{
		public:
			CCommandFeedbackHandler(CImpactApplication* poApplication);
			~CCommandFeedbackHandler() {}

			void execute(int iChannelCount, double* channel);

	};
}


#endif // __OpenViBEApplication_CCommandFeedbackHandler_H__
