#include "ovp_defines.h"

#include "box-algorithms/ovpCBoxAlgorithmStatisticGenerator.h"
#include "box-algorithms/ovpCBoxAlgorithmKappaCoefficient.h"
#include "box-algorithms/ovpCBoxAlgorithmConfusionMatrix.h"
#include "box-algorithms/ovpCBoxAlgorithmROCCurve.h"
#include "box-algorithms/ovpCBoxAlgorithmClassifierAccuracyMeasure.h"

#include "algorithms/ovpCAlgorithmConfusionMatrix.h"

OVP_Declare_Begin()
	if(std::string(rPluginModuleContext.getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toUInteger()).toASCIIString()) == std::string(""))
	{
		rPluginModuleContext.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(), OV_AttributeId_Box_FlagIsUnstable.toUInteger());
	}
	OVP_Declare_New(OpenViBEPlugins::Evaluation::CBoxAlgorithmStatisticGeneratorDesc);
	OVP_Declare_New(OpenViBEPlugins::Evaluation::CBoxAlgorithmKappaCoefficientDesc);
	OVP_Declare_New(OpenViBEPlugins::Evaluation::CBoxAlgorithmROCCurveDesc);

	OVP_Declare_New(OpenViBEPlugins::Evaluation::CAlgorithmConfusionMatrixDesc);
	OVP_Declare_New(OpenViBEPlugins::Evaluation::CBoxAlgorithmConfusionMatrixDesc);
	OVP_Declare_New(OpenViBEPlugins::Evaluation::CBoxAlgorithmClassifierAccuracyMeasureDesc);
OVP_Declare_End()
