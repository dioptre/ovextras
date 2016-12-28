/// Controls the ship movement by the continuous values from the classifier probabilities
// If bTakeControl is false, feedback levels will still be provided by the analog control.

#ifndef __OpenViBEApplication_CCommandControlAnalog_H__
#define __OpenViBEApplication_CCommandControlAnalog_H__

#include <vector>
#include "../ovamsICommandVRPNAnalog.h"

namespace OpenViBESSVEPMindShooter
{
	class CImpactApplication;

	class CCommandControlAnalog : public ICommandVRPNAnalog
	{
		enum ShipCommand { NOTHING = 0, SHOOT, MOVE_LEFT, MOVE_RIGHT };

	public:
		CCommandControlAnalog(CImpactApplication* poApplication, OpenViBE::boolean bTakeControl);
		~CCommandControlAnalog() {}

		void execute(int iChannelCount, double* pChannel);

	private:
		CImpactApplication* m_poApplication;
		int m_vCommandStates[3];

		OpenViBE::float64 m_pMaxFeedbackLevel[3];

		void commandeerShip(const double *pProbs);

		OpenViBE::boolean m_bInControl;


	};
}


#endif // __OpenViBEApplication_CCommandControlAnalog_H__
