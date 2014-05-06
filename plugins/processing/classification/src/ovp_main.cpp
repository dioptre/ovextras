
#include <vector>

#include "ovp_defines.h"
#include "toolkit/algorithms/classification/ovtkCAlgorithmPairingStrategy.h" //For comparision mecanism

#include "algorithms/ovpCAlgorithmClassifierNULL.h"
#include "algorithms/ovpCAlgorithmClassifierSVM.h"
#include "algorithms/ovpCAlgorithmClassifierOneVsAll.h"
#include "algorithms/ovpCAlgorithmClassifierOneVsOne.h"
#include "algorithms/ovpCAlgorithmConfusionMatrix.h"

#include "algorithms/ovpCAlgorithmPairwiseDecision.h"
#include "algorithms/ovpCAlgorithmPairwiseStrategyPKPD.h"
#include "algorithms/ovpCAlgorithmPairwiseDecisionVoting.h"
#include "algorithms/ovpCAlgorithmPairwiseDecisionHT.h"

#include "box-algorithms/ovpCBoxAlgorithmVotingClassifier.h"
#include "box-algorithms/ovpCBoxAlgorithmClassifierTrainer.h"
#include "box-algorithms/ovpCBoxAlgorithmClassifierProcessor.h"
#include "box-algorithms/ovpCBoxAlgorithmConfusionMatrix.h"

#include "box-algorithms/deprecated/ovpCBoxAlgorithmClassifierProcessorDeprecated.h"
#include "box-algorithms/deprecated/ovpCBoxAlgorithmClassifierTrainerDeprecated.h"

#if defined TARGET_HAS_ThirdPartyBLiFF
#include "algorithms/classification/ovpCAlgorithmClassifierBliffLDA.h"
#include "algorithms/classification/ovpCAlgorithmClassifierBliffCFIS.h"
#endif // TARGET_HAS_ThirdPartyBLiFF

#if defined TARGET_HAS_ThirdPartyITPP
#include "algorithms/ovpCAlgorithmClassifierLDA.h"
#include "algorithms/ovpCAlgorithmClassifierPLDA.h"
#endif // TARGET_HAS_ThirdPartyITPP

#if defined TARGET_HAS_ThirdPartyEIGEN
#include "algorithms/ovpCAlgorithmConditionedCovariance.h"
#include "algorithms/ovpCAlgorithmClassifierShrinkageLDA.h"
#include "algorithms/ovpCAlgorithmClassifierPShrinkageLDA.h"
#endif // TARGET_HAS_ThirdPartyEIGEN

OVP_Declare_Begin();
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationStrategy, "Native", OV_UndefinedIdentifier.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationStrategy, "OneVsAll", OVP_ClassId_Algorithm_ClassifierOneVsAll.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationStrategy, "OneVsOne", OVP_ClassId_Algorithm_ClassifierOneVsOne.toUInteger());

	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "Support Vector Machine (SVM)",OVP_ClassId_Algorithm_ClassifierSVM.toUInteger());

//	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "NULL Classifier (does nothing)",OVP_ClassId_Algorithm_ClassifierNULL.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "Support Vector Machine (SVM)",OVP_ClassId_Algorithm_ClassifierSVM.toUInteger());
	OpenViBEToolkit::registerClassificationComparisionFunction(OVP_ClassId_Algorithm_ClassifierSVM, OpenViBEPlugins::Classification::getSVMBestClassification);


	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_SVMType,"SVM Type");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMType,"C-SVC",C_SVC);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMType,"Nu-SVC",NU_SVC);
	
	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_SVMKernelType, "SVM Kernel Type");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Linear",LINEAR);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Polinomial",POLY);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Radial basis function",RBF);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType,"Sigmoid",SIGMOID);

	//OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierNULLDesc);
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierSVMDesc);

	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmConfusionMatrixDesc);

	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmVotingClassifierDesc);
	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmClassifierTrainerDesc);
	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmClassifierProcessorDesc);

	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmConfusionMatrixDesc);

	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierOneVsAllDesc);
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierOneVsOneDesc);

	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_ClassificationPairwiseStrategy, "Pairwise Decision Strategy");

	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmPairwiseStrategyPKPDDesc);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ClassificationPairwiseStrategy, "PKPD", OVP_ClassId_Algorithm_PairwiseStrategy_PKPD.toUInteger());
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmPairwiseDecisionVotingDesc);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ClassificationPairwiseStrategy, "Voting", OVP_ClassId_Algorithm_PairwiseDecision_Voting.toUInteger());
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmPairwiseDecisionHTDesc);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ClassificationPairwiseStrategy, "HT", OVP_ClassId_Algorithm_PairwiseDecision_HT.toUInteger());


	//Deprecated trainer/processos boxes
	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmClassifierProcessorDeprecatedDesc);
	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmClassifierTrainerDeprecatedDesc);

#if defined TARGET_HAS_ThirdPartyBLiFF
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "BLiFF - Linear Discrimimant Analysis (LDA)", OVP_ClassId_Algorithm_ClassifierBliffLDA.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "BLiFF - Chiu's Fuzzy Inference System (CFIS)", OVP_ClassId_Algorithm_ClassifierBliffCFIS.toUInteger());


	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_CFISMode, "CFIS Mode");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_CFISMode, "Simple Gaussian", OVP_TypeId_CFISMode_SimpleGaussian.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_CFISMode, "Two Sided Gaussian", OVP_TypeId_CFISMode_TwoSidedGaussian.toUInteger());

	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_CFISOutputMode, "CFIS Output Mode");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_CFISOutputMode, "Class Membership", OVP_TypeId_CFISOutputMode_ClassMembership.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_CFISOutputMode, "Rule Fulfillment", OVP_TypeId_CFISOutputMode_RuleFulfillment.toUInteger());

	OVP_Declare_New(OpenViBEPlugins::Local::CAlgorithmClassifierBliffLDADesc);
	OVP_Declare_New(OpenViBEPlugins::Local::CAlgorithmClassifierBliffCFISDesc);

#endif // TARGET_HAS_ThirdPartyBLiFF

#if defined TARGET_HAS_ThirdPartyITPP
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Linear Discrimimant Analysis (LDA)", OVP_ClassId_Algorithm_ClassifierLDA.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Probabilistic Linear Discrimimant Analysis (PLDA)", OVP_ClassId_Algorithm_ClassifierPLDA.toUInteger());

	OpenViBEToolkit::registerClassificationComparisionFunction(OVP_ClassId_Algorithm_ClassifierLDA, OpenViBEPlugins::Classification::getLDABestClassification);
	OpenViBEToolkit::registerClassificationComparisionFunction(OVP_ClassId_Algorithm_ClassifierPLDA, OpenViBEPlugins::Classification::getPLDABestClassification);

	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierLDADesc);
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierPLDADesc);
#endif // TARGET_HAS_ThirdPartyITPP
	
#if defined TARGET_HAS_ThirdPartyEIGEN
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmConditionedCovarianceDesc);

	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Shrinkage LDA", OVP_ClassId_Algorithm_ClassifierShrinkageLDA.toUInteger());
	OpenViBEToolkit::registerClassificationComparisionFunction(OVP_ClassId_Algorithm_ClassifierShrinkageLDA, OpenViBEPlugins::Classification::getShrinkageLDABestClassification);
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierShrinkageLDADesc);

	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Probabilistic Shrinkage LDA", OVP_ClassId_Algorithm_ClassifierPShrinkageLDA.toUInteger());
	OpenViBEToolkit::registerClassificationComparisionFunction(OVP_ClassId_Algorithm_ClassifierPShrinkageLDA, OpenViBEPlugins::Classification::getPShrinkageLDABestClassification);
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierPShrinkageLDADesc);

#endif // TARGET_HAS_ThirdPartyEIGEN

OVP_Declare_End();
