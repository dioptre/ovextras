#include "ovp_defines.h"

#include "box-algorithms/ovpCBoxAlgorithmEBMLStreamSpy.h"
#include "box-algorithms/ovpCBoxAlgorithmStimulationListener.h"

// #include "ovpCBoxAlgorithmLatencyEvaluation.h"
#include "box-algorithms/ovpCBoxAlgorithmMatrixValidityChecker.h"

#include "box-algorithms/ovpCBoxAlgorithmMessageSpy.h"
#include "box-algorithms/ovpCBoxAlgorithmMouseTracking.h"
#include "box-algorithms/ovpCBoxAlgorithmAngleSpeed.h"

OVP_Declare_Begin();

	OVP_Declare_New(OpenViBEPlugins::Tools::CBoxAlgorithmStimulationListenerDesc);
	OVP_Declare_New(OpenViBEPlugins::Tools::CBoxAlgorithmEBMLStreamSpyDesc);
// 	OVP_Declare_New(OpenViBEPlugins::Tools::CBoxAlgorithmLatencyEvaluationDesc);
	OVP_Declare_New(OpenViBEPlugins::Tools::CBoxAlgorithmMatrixValidityCheckerDesc);
    OVP_Declare_New(OpenViBEPlugins::Tools::CBoxAlgorithmMessageSpyDesc);
    OVP_Declare_New(OpenViBEPlugins::Tools::CBoxAlgorithmMouseTrackingDesc);
    OVP_Declare_New(OpenViBEPlugins::Tools::CBoxAlgorithmAngleSpeedDesc);
OVP_Declare_End();
