
#if defined(TARGET_HAS_ThirdPartyOgre3DTerrain)

#include "ovamsCCommandControlAnalog.h"
#include "ovamsCImpactApplication.h"

using namespace OpenViBE;
using namespace OpenViBESSVEPMindShooter;

CCommandControlAnalog::CCommandControlAnalog(CImpactApplication* poApplication, boolean bTakeControl) :
	ICommandVRPNAnalog(poApplication, "SSVEP_VRPN_AnalogControl"), 
	m_poApplication( poApplication ),
	m_bInControl(bTakeControl)
{
	m_vCommandStates[0] = 0;
	m_vCommandStates[1] = 0;
	m_vCommandStates[2] = 0;

	m_pMaxFeedbackLevel[0] = 0;
	m_pMaxFeedbackLevel[1] = 0;
	m_pMaxFeedbackLevel[2] = 0;
}

void CCommandControlAnalog::execute(int iChannelCount, double* pChannel)
{
	CImpactApplication* l_poImpactApplication = dynamic_cast<CImpactApplication*>(m_poApplication);

	if (iChannelCount != 4)
	{
		l_poImpactApplication->getLogManager() << OpenViBE::Kernel::LogLevel_Error << "Incorrect analog VRPN input with " << iChannelCount << " channels. Mind Shooter needs 4 channels.\n";
		return;
	}
	
	/*
	std::cout << "Probs " << pChannel[0] << " "
		<< pChannel[1] << " "
		<< pChannel[2] << " "
		<< pChannel[3] << "\n";
	*/

	// We only use the predictions of the 3 channels related to the flickering targets.
	// The class coding is assumed as follows, 
	//  0 == 'focus on the middle of the ship')
	//  1 == focus tip
	//  2 == focus left
	//  3 == focus right

	if (m_bInControl)
	{
		commandeerShip(pChannel);
	}

	const int l_iNChannels = 3;
	static int l_vPreviousLevels[l_iNChannels] = { 0, 0, 0 };

	double l_vFeedback[l_iNChannels];

	// Assume input is layered like [prob1,prob2,prob3,prob4]. First ignored for feedback (focus middle of ship).
	l_vFeedback[0] = pChannel[1];
	l_vFeedback[1] = pChannel[2];
	l_vFeedback[2] = pChannel[3];

	int l_vLevel[l_iNChannels];

	//std::cout << "feedback: ";
	for (int i = 0; i < l_iNChannels; i++)
	{
		if (l_vFeedback[i] < 0) 
		{
			// Note: shouldn't happen, we're expecting positive numbers
			l_vFeedback[i] = 0;
		}

		if (l_vFeedback[i] > m_pMaxFeedbackLevel[i])
		{
			m_pMaxFeedbackLevel[i] += 0.1;
		}
		else
		{
			// Slowly decrease max over time in case it was due to artifact
			m_pMaxFeedbackLevel[i] *= 0.995;
		}

		const float64 l_f64CLevel = (m_pMaxFeedbackLevel[i] > 0 ? (l_vFeedback[i] / m_pMaxFeedbackLevel[i]) : 0);

		//std::cout << cLevel << ", ";
		l_vLevel[i] = 0;

		if (l_f64CLevel > 0.7)
		{
			l_vLevel[i] = 3;
		}
		else if (l_f64CLevel > 0.3)
		{
			l_vLevel[i] = 2;
		}
		else if (l_f64CLevel > 0.0)
		{
			l_vLevel[i] = 1;
		}
	}
	//std::cout << "\n";


	if (l_vLevel[0] != l_vPreviousLevels[0] || l_vLevel[1] != l_vPreviousLevels[1] || l_vLevel[2] != l_vPreviousLevels[2])
	{
		l_poImpactApplication->getLogManager() << OpenViBE::Kernel::LogLevel_Trace << "Feedback levels : " << l_vLevel[0] << " " << l_vLevel[1] << " " << l_vLevel[2] << "\n";

		l_vPreviousLevels[0] = l_vLevel[0];
		l_vPreviousLevels[1] = l_vLevel[1];
		l_vPreviousLevels[2] = l_vLevel[2];

		l_poImpactApplication->getShip()->setFeedbackLevels(l_vLevel[0], l_vLevel[1], l_vLevel[2]);
	}

}

const std::string stateToString(CImpactShip::TargetState ts)
{
	if (ts == CImpactShip::TS_NORMAL)
	{
		return "N";
	}
	if (ts == CImpactShip::TS_INHIBITED)
	{
		return "I";
	}
	if (ts == CImpactShip::TS_OFF)
	{
		return "X";
	}

	return "-";
}

// Expects 4 non-negative inputs
void CCommandControlAnalog::commandeerShip(const double *pProbs)
{
	m_vCommandStates[0] = 0;
	m_vCommandStates[1] = 0;
	m_vCommandStates[2] = 0;

	// If true, a NOP classification (class 0) will never be selected
	const boolean l_bIgnoreIdle = false;

	// Find the strongest activation. 
	uint32 l_ui32MaxIdx = 0;
	float64 l_f64MaxVal = -std::numeric_limits<float64>::max();
	float64 l_f64Sum = 0;
	for (uint32 i = 0; i<4; i++) {
		if (pProbs[i] >= l_f64MaxVal)
			// && !(i==0 && l_bIgnoreIdle))
		{
			l_f64MaxVal = pProbs[i];
			l_ui32MaxIdx = i;
		}
		l_f64Sum += pProbs[i];
	}
	if (l_f64Sum <= 0) 
	{
		l_f64Sum = 1;
	}

	const boolean l_bParalyzed = false;
	if (l_bParalyzed)
	{
		// In this mode, the ship does not move but the prediction is displayed instead. May be useful for debugging.
		char text[512];
		sprintf(text, "Button %d", l_ui32MaxIdx);

		float pos = (l_ui32MaxIdx / 3.0f) * 0.85f;

		m_poApplication->getInstructionWindow()->setSize(CEGUI::USize(CEGUI::UDim(0.10f, 0), CEGUI::UDim(0.10f, 0)));
		m_poApplication->getInstructionWindow()->setPosition(CEGUI::UVector2(CEGUI::UDim(pos, 0), CEGUI::UDim(0.5f, 0)));
		m_poApplication->getInstructionWindow()->setText(text);
		m_poApplication->getInstructionWindow()->show();

		return;
	}

	// In principle here we could do different things with the probabilities. If we predict just the
	// maximum, this can be expected to be identical to the Classifier Processor box label output, 
	// hence no difference is expected between the two control schemes (discrete vs analog).
	// To make the two controls a bit different, the probabilities can be used to
	// modulate the strength of the ship movement.
	
	// Modulating with the probabilities makes the movements a bit jittery
	// const float64 l_f64Strength = l_f64MaxVal / l_f64Sum;
	
	// This choice is smoother, but will make the behavior identical to discrete control
	const float64 l_f64Strength = 1.0;

	CImpactShip* l_poShip = m_poApplication->getShip();

	if (l_ui32MaxIdx == NOTHING)
	{
		// NOP
	}
	else if (l_ui32MaxIdx == SHOOT && m_poApplication->getShip()->getTargetState(0) != CImpactShip::TS_OFF)
	{
		m_vCommandStates[0] = 1;
		l_poShip->shoot();
	}
	else if (l_ui32MaxIdx == MOVE_LEFT && m_poApplication->getShip()->getTargetState(1) != CImpactShip::TS_OFF)
	{
		m_vCommandStates[1] = 1;
		l_poShip->move(static_cast<int>(-6 * l_f64Strength));
	}
	else if (l_ui32MaxIdx == MOVE_RIGHT && m_poApplication->getShip()->getTargetState(2) != CImpactShip::TS_OFF)
	{
		m_vCommandStates[2] = 1;
		l_poShip->move(static_cast<int>(6 * l_f64Strength));

	}

	/*
	std::cout << "commands " << m_vCommandStates[0] << " " << m_vCommandStates[1] << " " << m_vCommandStates[2] << "\n";
	std::cout << "states " << stateToString(m_poApplication->getShip()->getTargetState(0)) << " "
			  << stateToString(m_poApplication->getShip()->getTargetState(1))
			  << " " << stateToString(m_poApplication->getShip()->getTargetState(2)) << "\n";
			  */
}


#endif