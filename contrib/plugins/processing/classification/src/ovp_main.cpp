
#include <vector>
#include <openvibe/ov_all.h>
#include "ovp_defines.h"

// @BEGIN inserm-gpl

#include "algorithms/ovpCComputeFisherLdaFunction.h"
#include "algorithms/ovpCApplyFisherLdaFunction.h"
#include "algorithms/ovpCFeatureExtractionLda.h"
#include "box-algorithms/ovpCLDABoxAlgorithm.h"

#include "algorithms/ovpCNaiveBayesApplyFunction.h"
#include "box-algorithms/ovpCNaiveBayesApplyBoxAlgorithm.h"

// @END inserm-gpl

#include "algorithms/ovpCAlgorithmClassifierAdaptiveGradientDescentLDA.h"

OVP_Declare_Begin();

// @BEGIN inserm-gpl
#if defined TARGET_HAS_ThirdPartyITPP
	OVP_Declare_New(OpenViBEPlugins::SignalProcessingGpl::CComputeFisherLdaFunctionDesc);
	OVP_Declare_New(OpenViBEPlugins::SignalProcessingGpl::CApplyFisherLdaFunctionDesc);
	OVP_Declare_New(OpenViBEPlugins::SignalProcessingGpl::CFeatureExtractionLdaDesc);

	OVP_Declare_New(OpenViBEPlugins::SignalProcessingGpl::CNaiveBayesApplyFunctionDesc);			// Requires ITPP
	OVP_Declare_New(OpenViBEPlugins::SignalProcessingGpl::CNaiveBayesApplyBoxAlgorithmDesc);		// Depends on the previous

	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Adaptive Gradient Descent LDA", OVP_ClassId_Algorithm_ClassifierAdaptiveGradientDescentLDA.toUInteger());
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierAdaptiveGradientDescentLDADesc);			// Requires ITPP
#endif // TARGET_HAS_ThirdPartyITPP

	OVP_Declare_New(OpenViBEPlugins::SignalProcessingGpl::CLDABoxAlgorithmDesc);
// @END inserm-gpl
		 
OVP_Declare_End();
