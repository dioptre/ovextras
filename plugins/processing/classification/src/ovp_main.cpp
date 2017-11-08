#include <vector>

#include "ovp_defines.h"
#include "toolkit/algorithms/classification/ovtkCAlgorithmPairingStrategy.h" //For comparision mecanism

#include "algorithms/ovpCAlgorithmClassifierSVM.h"

#include "box-algorithms/ovpCBoxAlgorithmOutlierRemoval.h"

#if defined TARGET_HAS_ThirdPartyEIGEN
#include "algorithms/ovpCAlgorithmClassifierMLP.h"
#endif // TARGET_HAS_ThirdPartyEIGEN

const char* const c_sPairwiseStrategyEnumerationName = "Pairwise Decision Strategy";

OVP_Declare_Begin();
	// SVM related
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "Support Vector Machine (SVM)",OVP_ClassId_Algorithm_ClassifierSVM.toUInteger());
	OpenViBEToolkit::registerClassificationComparisonFunction(OVP_ClassId_Algorithm_ClassifierSVM, OpenViBEPlugins::Classification::SVMClassificationCompare);
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierSVMDesc);

	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_SVMType,"SVM Type");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMType,"C-SVC",C_SVC);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMType,"Nu-SVC",NU_SVC);
	
	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_SVMKernelType, "SVM Kernel Type");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Linear",LINEAR);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Polinomial",POLY);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Radial basis function",RBF);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Sigmoid",SIGMOID);


	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_ClassificationPairwiseStrategy, c_sPairwiseStrategyEnumerationName);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ClassificationPairwiseStrategy, "Support Vector Machine (SVM)", OVP_ClassId_Algorithm_ClassifierSVM.toUInteger());

	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_OneVsOne_DecisionAlgorithms , "One vs One Decision Algorithms");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_OneVsOne_DecisionAlgorithms, "SVM Kernel Type", OVP_TypeId_SVMType.toUInteger());

#if defined TARGET_HAS_ThirdPartyEIGEN
	//MLP section
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierMLPDesc);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Multi-layer Perceptron", OVP_ClassId_Algorithm_ClassifierMLP.toUInteger());
	OpenViBEToolkit::registerClassificationComparisonFunction(OVP_ClassId_Algorithm_ClassifierMLP, OpenViBEPlugins::Classification::MLPClassificationCompare);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ClassificationPairwiseStrategy, "Multi-layer Perceptron", OVP_ClassId_Algorithm_ClassifierMLP.toUInteger());

	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_OneVsOne_DecisionAlgorithms, "Multi-layer Perceptron", OVP_ClassId_Algorithm_ClassifierMLP.toUInteger());

#endif // TARGET_HAS_ThirdPartyEIGEN

	// Register boxes
	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmOutlierRemovalDesc);

OVP_Declare_End();

#include<cmath>
bool ov_float_equal(double f64First, double f64Second)
{
	const double c_f64Epsilon = 0.000001;
	return c_f64Epsilon > ::fabs(f64First - f64Second);
}
