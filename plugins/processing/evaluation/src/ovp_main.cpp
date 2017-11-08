#include "ovp_defines.h"

#include "box-algorithms/ovpCBoxAlgorithmStatisticGenerator.h"
#include "box-algorithms/ovpCBoxAlgorithmKappaCoefficient.h"
#include "box-algorithms/ovpCBoxAlgorithmConfusionMatrix.h"
#include "box-algorithms/ovpCBoxAlgorithmROCCurve.h"
#include "box-algorithms/ovpCBoxAlgorithmClassifierAccuracyMeasure.h"

#include "algorithms/ovpCAlgorithmConfusionMatrix.h"

OVP_Declare_Begin()
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(), OV_AttributeId_Box_FlagIsUnstable.toUInteger());
	
	OVP_Declare_New(OpenViBEPlugins::Evaluation::CBoxAlgorithmStatisticGeneratorDesc);

#if defined(TARGET_HAS_ThirdPartyGTK)
	OVP_Declare_New(OpenViBEPlugins::Evaluation::CBoxAlgorithmKappaCoefficientDesc);
	OVP_Declare_New(OpenViBEPlugins::Evaluation::CBoxAlgorithmROCCurveDesc);
#endif

	OVP_Declare_New(OpenViBEPlugins::Evaluation::CAlgorithmConfusionMatrixDesc);
	OVP_Declare_New(OpenViBEPlugins::Evaluation::CBoxAlgorithmConfusionMatrixDesc);
#if defined(TARGET_HAS_ThirdPartyGTK)
	OVP_Declare_New(OpenViBEPlugins::Evaluation::CBoxAlgorithmClassifierAccuracyMeasureDesc);
#endif
OVP_Declare_End()
