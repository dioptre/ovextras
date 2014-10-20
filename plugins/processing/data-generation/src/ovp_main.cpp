#include "ovp_defines.h"

#include "box-algorithms/ovpCBoxAlgorithmNoiseGenerator.h"
#include "box-algorithms/ovpCSinusSignalGenerator.h"
#include "box-algorithms/ovpCTimeSignalGenerator.h"

#include "box-algorithms/ovpCBoxAlgorithmClockStimulator.h"



#include "box-algorithms/ovpCBoxAlgorithmMeanVariance.h"

OVP_Declare_Begin();

	OVP_Declare_New(OpenViBEPlugins::DataGeneration::CNoiseGeneratorDesc);
	OVP_Declare_New(OpenViBEPlugins::DataGeneration::CSinusSignalGeneratorDesc);
	OVP_Declare_New(OpenViBEPlugins::DataGeneration::CTimeSignalGeneratorDesc);

	OVP_Declare_New(OpenViBEPlugins::DataGeneration::CBoxAlgorithmClockStimulatorDesc);

	OVP_Declare_New(OpenViBEPlugins::Samples::CBoxAlgorithmMeanVarianceDesc);

OVP_Declare_End();
