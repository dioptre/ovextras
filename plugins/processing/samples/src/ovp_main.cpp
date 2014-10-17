#include "ovp_defines.h"

#include "box-algorithms/ovpCCrashingBox.h"
#include "box-algorithms/ovpCBoxAlgorithmNoiseGenerator.h"
#include "box-algorithms/ovpCSinusSignalGenerator.h"
#include "box-algorithms/ovpCTimeSignalGenerator.h"
#include "box-algorithms/ovpCIdentity.h"
#include "box-algorithms/ovpCLog.h"

#include "box-algorithms/ovpCBoxAlgorithmClock.h"
#include "box-algorithms/ovpCBoxAlgorithmClockStimulator.h"

#include "algorithms/ovpCAlgorithmAddition.h"
#include "box-algorithms/ovpCBoxAlgorithmAdditionTest.h"

#include "box-algorithms/ovpCBoxAlgorithmNothing.h"

#include "box-algorithms/ovpCBoxAlgorithmMeanVariance.h"

#include "box-algorithms/ovpCTestCodecToolkit.h"

OVP_Declare_Begin();

	OVP_Declare_New(OpenViBEPlugins::Samples::CNoiseGeneratorDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CSinusSignalGeneratorDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CTimeSignalGeneratorDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CIdentityDesc);
	// OVP_Declare_New(OpenViBEPlugins::Samples::CLogDesc);
	// OVP_Declare_New(OpenViBEPlugins::Samples::CBoxAlgorithmClockDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CBoxAlgorithmClockStimulatorDesc);

	// OVP_Declare_New(OpenViBEPlugins::Samples::CBoxAlgorithmAdditionTestDesc);
	// OVP_Declare_New(OpenViBEPlugins::Samples::CAlgorithmAdditionDesc);

	// OVP_Declare_New(OpenViBEPlugins::Samples::CBoxAlgorithmNothingDesc);

	OVP_Declare_New(OpenViBEPlugins::Samples::CBoxAlgorithmMeanVarianceDesc);

	OVP_Declare_New(OpenViBEPlugins::Samples::CTestCodecToolkitDesc);
OVP_Declare_End();
