#include "ovp_defines.h"

//#include "box-algorithms/classification/ovpCDynamicNaiveBayesComputeBoxAlgorithm.h"
#include "box-algorithms/classification/ovpCBoxAlgorithmAdaptiveP300Classifier.h"

#include "algorithms/classification/ovpCAlgorithmClassifierNULL.h"
#include "algorithms/classification/ovpCAlgorithmClassifierMixtureOfExperts.h"
//#include "algorithms/classification/ovpCLikelihoodComputeFunction.h"
#include "algorithms/classification/ovpCAlgorithmClassifierRelearnPLDA.h"
#include "algorithms/classification/ovpCAlgorithmClassifierPLDA.h"
#include "algorithms/classification/ovpCAlgorithmClassifierAdaptiveGradientDescentLDA.h"


#include <vector>

OVP_Declare_Begin()
	rPluginModuleContext.getTypeManager().registerEnumerationType (OVP_TypeId_ProcessType, "Process");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ProcessType, "When all inputs are decoded", OVP_TypeId_ProcessType_AllInputs.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ProcessType, "For each input", OVP_TypeId_ProcessType_EachInput.toUInteger());

	rPluginModuleContext.getTypeManager().registerEnumerationType (OVP_TypeId_ParadigmType, "Stimulation Paradigm");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ParadigmType, "SuperSplotch", OVP_TypeId_ParadigmType_SuperSplotch.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ParadigmType, "Row/Column or Splotch", OVP_TypeId_ParadigmType_RowCol_Splotch.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ParadigmType, "One Group",OVP_TypeId_ParadigmType_OneGroup.toUInteger());

	
	rPluginModuleContext.getTypeManager().registerEnumerationType (OVP_TypeId_DecisionCriterium, "Decision Criterium (Optimal Stopping selected)");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_DecisionCriterium, "Info", OVP_TypeId_DecisionCriterium_Info.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_DecisionCriterium, "Lin", OVP_TypeId_DecisionCriterium_Lin.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_DecisionCriterium, "Exp", OVP_TypeId_DecisionCriterium_Exp.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_DecisionCriterium, "Util", OVP_TypeId_DecisionCriterium_Util.toUInteger());

	rPluginModuleContext.getTypeManager().registerEnumerationType (OVP_TypeId_EvidenceAccumulationAlgorithm, "Evidence Accumulation Algorithm");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_EvidenceAccumulationAlgorithm, "None", OVP_ClassId_Algorithm_EvidenceAccumulationNone.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_EvidenceAccumulationAlgorithm, "Counter", OVP_ClassId_Algorithm_EvidenceAccumulationCounter.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_EvidenceAccumulationAlgorithm, "Bayesian", OVP_ClassId_Algorithm_EvidenceAccumulationBayesian.toUInteger());	
	
	rPluginModuleContext.getTypeManager().registerEnumerationType (OVP_TypeId_EvidenceAccumulationAlgorithmInputType, "Evidence Accumulation Input Type");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_EvidenceAccumulationAlgorithmInputType, "Distance to hyperplane", OVP_InputType_EvidenceAccumulationDistance.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_EvidenceAccumulationAlgorithmInputType, "Class probabilities", OVP_InputType_EvidenceAccumulationProbability.toUInteger());
	
	//*
	//rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Mixture of Experts", OVP_ClassId_Algorithm_ClassifierMixtureOfExperts.toUInteger());
	//rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Relearn Probabilistic LDA", OVP_ClassId_Algorithm_ClassifierRelearnPLDA.toUInteger());
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Probabilistic LDA", OVP_ClassId_Algorithm_ClassifierPLDA.toUInteger());
	//rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm,   "Adaptive Gradient Descent LDA", OVP_ClassId_Algorithm_ClassifierAdaptiveGradientDescentLDA.toUInteger());
	//*/

	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeId_ShrinkageType,"Shrinkage");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ShrinkageType,"Full covariance",FULL);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ShrinkageType,"Shrink to diagonal",SHRINK_TO_DIAG);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ShrinkageType,"Diagonal",DIAG);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeId_ShrinkageType,"Shrink to unity",SHRINK_TO_UNITY);
		
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierNULLDesc);
	//OVP_Declare_New(OpenViBEPlugins::SignalProcessingGpl::CLikelihoodComputeFunctionDesc);
	//OVP_Declare_New(OpenViBEPlugins::SignalProcessingGpl::CDynamicNaiveBayesComputeBoxAlgorithmDesc);
	//OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierMixtureOfExpertsDesc);
	OVP_Declare_New(OpenViBEPlugins::Classification::CBoxAlgorithmAdaptiveP300ClassifierDesc);
	//OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierRelearnPLDADesc);
	OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierPLDADesc);
	//OVP_Declare_New(OpenViBEPlugins::Classification::CAlgorithmClassifierAdaptiveGradientDescentLDADesc);
OVP_Declare_End()
