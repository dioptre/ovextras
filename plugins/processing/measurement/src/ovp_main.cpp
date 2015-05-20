#include "ovp_defines.h"

#include "box-algorithms/ovpCBoxAlgorithmStatisticGenerator.h"
#include "box-algorithms/ovpCBoxAlgorithmKappaCoefficient.h"
#include "box-algorithms/ovpCBoxAlgorithmConfusionMatrix.h"

#include "algorithms/ovpCAlgorithmConfusionMatrix.h"

OVP_Declare_Begin()
	OVP_Declare_New(OpenViBEPlugins::Measurement::CBoxAlgorithmStatisticGeneratorDesc)
	OVP_Declare_New(OpenViBEPlugins::Measurement::CBoxAlgorithmKappaCoefficientDesc)

	OVP_Declare_New(OpenViBEPlugins::Measurement::CAlgorithmConfusionMatrixDesc);
	OVP_Declare_New(OpenViBEPlugins::Measurement::CBoxAlgorithmConfusionMatrixDesc);
OVP_Declare_End()
