#include <vector>

#include "ovp_defines.h"
#include "toolkit/algorithms/classification/ovtkCAlgorithmPairingStrategy.h" //For comparision mecanism

#include "algorithms/ovpCAlgorithmClassifierNULL.h"
#include "algorithms/ovpCAlgorithmClassifierSVM.h"
#include "algorithms/ovpCAlgorithmClassifierOneVsAll.h"
#include "algorithms/ovpCAlgorithmClassifierOneVsOne.h"

#include "algorithms/ovpCAlgorithmPairwiseDecision.h"
#include "algorithms/ovpCAlgorithmPairwiseStrategyPKPD.h"
#include "algorithms/ovpCAlgorithmPairwiseDecisionVoting.h"
#include "algorithms/ovpCAlgorithmPairwiseDecisionHT.h"

#include "box-algorithms/ovpCBoxAlgorithmVotingClassifier.h"
#include "box-algorithms/ovpCBoxAlgorithmClassifierTrainer.h"
#include "box-algorithms/ovpCBoxAlgorithmClassifierProcessor.h"

#include "box-algorithms/ovpCBoxAlgorithmOutlierRemoval.h"

#if defined TARGET_HAS_ThirdPartyEIGEN
#include "algorithms/ovpCAlgorithmConditionedCovariance.h"
#include "algorithms/ovpCAlgorithmClassifierLDA.h"
#include "algorithms/ovpCAlgorithmClassifierMLP.h"
#endif // TARGET_HAS_ThirdPartyEIGEN

const char* const c_sPairwiseStrategyEnumerationName = "Pairwise Decision Strategy";

OVP_Declare_Begin();

	// Register multiclass strategies
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierOneVsAllDesc);
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierOneVsOneDesc);

	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationStrategy, "Native", OV_UndefinedIdentifier.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationStrategy, "OneVsAll", OVP_ClassId_Algorithm_ClassifierOneVsAll.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationStrategy, "OneVsOne", OVP_ClassId_Algorithm_ClassifierOneVsOne.toUInteger());

	// Functions related to deciding winner in OneVsOne multiclass decision strategy
	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_ClassificationPairwiseStrategy, c_sPairwiseStrategyEnumerationName);

	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmPairwiseStrategyPKPDDesc);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ClassificationPairwiseStrategy, "PKPD", OVP_ClassId_Algorithm_PairwiseStrategy_PKPD.toUInteger());
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmPairwiseDecisionVotingDesc);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ClassificationPairwiseStrategy, "Voting", OVP_ClassId_Algorithm_PairwiseDecision_Voting.toUInteger());
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmPairwiseDecisionHTDesc);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ClassificationPairwiseStrategy, "HT", OVP_ClassId_Algorithm_PairwiseDecision_HT.toUInteger());

	// Register actual classifiers

//	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "NULL Classifier (does nothing)",OVP_ClassId_Algorithm_ClassifierNULL.toUInteger());
//	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierNULLDesc);

	// SVM related
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "Support Vector Machine (SVM)",OVP_ClassId_Algorithm_ClassifierSVM.toUInteger());
	OpenViBEToolkit::registerClassificationComparisionFunction(OVP_ClassId_Algorithm_ClassifierSVM, OpenViBEPlugins::Classification::SVMClassificationCompare);
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierSVMDesc);
	OpenViBEPlugins::Classification::registerAvailableDecisionEnumeration(OVP_ClassId_Algorithm_ClassifierSVM, OVP_TypeId_ClassificationPairwiseStrategy);

	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_SVMType,"SVM Type");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMType,"C-SVC",C_SVC);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMType,"Nu-SVC",NU_SVC);
	
	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_SVMKernelType, "SVM Kernel Type");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Linear",LINEAR);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Polinomial",POLY);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Radial basis function",RBF);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Sigmoid",SIGMOID);

#if defined TARGET_HAS_ThirdPartyEIGEN
	// LDA related
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmConditionedCovarianceDesc);

	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierLDADesc);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Linear Discrimimant Analysis (LDA)", OVP_ClassId_Algorithm_ClassifierLDA.toUInteger());
	OpenViBEToolkit::registerClassificationComparisionFunction(OVP_ClassId_Algorithm_ClassifierLDA, OpenViBEPlugins::Classification::LDAClassificationCompare);
	OpenViBEPlugins::Classification::registerAvailableDecisionEnumeration(OVP_ClassId_Algorithm_ClassifierLDA, OVP_TypeId_ClassificationPairwiseStrategy);

	//MLP section
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierMLPDesc);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Multi-layer Perceptron", OVP_ClassId_Algorithm_ClassifierMLP.toUInteger());
	OpenViBEToolkit::registerClassificationComparisionFunction(OVP_ClassId_Algorithm_ClassifierMLP, OpenViBEPlugins::Classification::MLPClassificationCompare);
	OpenViBEPlugins::Classification::registerAvailableDecisionEnumeration(OVP_ClassId_Algorithm_ClassifierMLP, OVP_TypeId_ClassificationPairwiseStrategy);
#endif // TARGET_HAS_ThirdPartyEIGEN

	// Register boxes

	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmVotingClassifierDesc);
	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmClassifierTrainerDesc);
	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmClassifierProcessorDesc);

	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmOutlierRemovalDesc);

OVP_Declare_End();

#include<cmath>
bool ov_float_equal(double f64First, double f64Second)
{
	const double c_f64Epsilon = 0.000001;
	return c_f64Epsilon > ::fabs(f64First - f64Second);
}
