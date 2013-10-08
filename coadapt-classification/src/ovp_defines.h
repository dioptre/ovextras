#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

enum { FULL=0, DIAG=1, SHRINK_TO_DIAG=2, SHRINK_TO_UNITY=3 };

//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_BoxAlgorithm_ClassifierProcessorWithUpdate OpenViBE::CIdentifier(0xADF3B87A, 0x875B6EB0)
#define OVP_ClassId_BoxAlgorithm_ClassifierProcessorWithUpdateDesc OpenViBE::CIdentifier(0xBE9EBA5C, 0xA8415D37)

#define OV_ClassId_BoxAlgorithm_PGTrainer                                    OpenViBE::CIdentifier(0x64641646, 0x378102DC)
#define OV_ClassId_BoxAlgorithm_PGTrainerDesc                                OpenViBE::CIdentifier(0x5895213D, 0x564304E2)


#define OVP_ClassId_BoxAlgorithm_SupErrP OpenViBE::CIdentifier(0x168D0871, 0x549D67FE)
#define OVP_ClassId_BoxAlgorithm_SupErrPDesc OpenViBE::CIdentifier(0x168D0871, 0x549D67FE)

#define OVP_ClassId_BoxAlgorithm_TargetArray OpenViBE::CIdentifier(0x3CE5498C, 0x40D670E8)
#define OVP_ClassId_BoxAlgorithm_TargetArrayDesc OpenViBE::CIdentifier(0x15D062F1, 0x76747FEA)

#define OVP_ClassId_BoxAlgorithm_Normalization OpenViBE::CIdentifier(0x77D80420, 0x04162E79)
#define OVP_ClassId_BoxAlgorithm_NormalizationDesc OpenViBE::CIdentifier(0x26723751, 0x3FC344EC)

#define OVP_ClassId_BoxAlgorithm_EvidenceAccumulator 			OpenViBE::CIdentifier(0x47BC41D5, 0x94D9F146)
#define OVP_ClassId_BoxAlgorithm_EvidenceAccumulatorDesc 		OpenViBE::CIdentifier(0x47BC41D5, 0x94D9F146)

#define OVP_ClassId_Algorithm_ClassifierMixtureOfExperts                                        OpenViBE::CIdentifier(0xDF828072, 0x22C0CF5F)
#define OVP_ClassId_Algorithm_ClassifierMixtureOfExpertsDesc                                    OpenViBE::CIdentifier(0x68595043, 0x6D981FC2)
//#define OVP_Algorithm_ClassifierMOE_InputParameterId_MOE_Mode	OpenViBE::CIdentifier(0x16DFF805, 0x0BC431BF)

/*                                      
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x020300BF, 0xB4BAC0A1)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0xBB1C5CFB, 0x1DD8690F)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x1575A534, 0xD7C09DD9)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x335384EA, 0x88C917D9)                                                                                                           
*/

#define OVP_ClassId_Algorithm_EvidenceAccumulationCounter	OpenViBE::CIdentifier(0x1E1065B2, 0x2CA32013)
#define OVP_ClassId_Algorithm_EvidenceAccumulationNone	OpenViBE::CIdentifier(0xE405260B, 0x59EEFAE4)
#define OVP_ClassId_Algorithm_EvidenceAccumulationBayesian	OpenViBE::CIdentifier(0xA15EAEC5, 0xAB0CE73D)   
#define OVP_TypeId_EvidenceAccumulationAlgorithm		OpenViBE::CIdentifier(0x1E1065B2, 0x2CA32013)
#define OVP_TypeId_EvidenceAccumulationAlgorithmInputType 	OpenViBE::CIdentifier(0xE5103C63, 0x08D825E0)
#define OVP_InputType_EvidenceAccumulationProbability		OpenViBE::CIdentifier(0x043A1BC4, 0x925D3CD6)
#define OVP_InputType_EvidenceAccumulationDistance		OpenViBE::CIdentifier(0x781F51CA, 0xE6E3B0B8)

#define OVP_ClassId_BoxAlgorithm_AdaptiveP300Classifier 		OpenViBE::CIdentifier(0x73C8A153, 0x6537A7E2)
#define OVP_ClassId_BoxAlgorithm_AdaptiveP300ClassifierDesc 	OpenViBE::CIdentifier(0xBA56CFE0, 0x7EBBE949)

#define OVP_ClassId_Algorithm_ClassifierRelearnPLDA                                         OpenViBE::CIdentifier(0x5CF66A73, 0xF5BBF0BF)
#define OVP_ClassId_Algorithm_ClassifierRelearnPLDADesc                                   OpenViBE::CIdentifier(0xD5C5EF91, 0xE1B1C4F4)

#define OVP_Algorithm_ClassifierPLDA_InputParameterId_Shrinkage		OpenViBE::CIdentifier(0x7FEFDCA9, 0x816ED903) 
#define OVP_Algorithm_ClassifierPLDA_InputParameterId_Lambda		OpenViBE::CIdentifier(0xEBAEB213, 0xDD4735A0)
#define OVP_TypeId_ShrinkageType									OpenViBE::CIdentifier(0x344A52F5, 0x489DB439)


#define OVP_ClassId_Algorithm_ClassifierPLDA                                         OpenViBE::CIdentifier(0x2F9ECA0B, 0x8D3CA7BD)
#define OVP_ClassId_Algorithm_ClassifierPLDADesc                                   OpenViBE::CIdentifier(0x1BD67420, 0x587600E6)

//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                                   //
//___________________________________________________________________//
//                                                                   //

/*												                                                                                                                                         
#define OVP_ClassId_                                        
#define OVP_ClassId_                                        
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0xB083614E, 0x26C6B4BD)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x7D13B924, 0xF194DA09)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x2397E74F, 0xCAC8F95C)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0xD6E1AC79, 0xBE50C28F)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x5E8D9B97, 0xF82F92D7)
*/

#define OVP_TypeId_ProcessType                                        					OpenViBE::CIdentifier(0x98CF8BCC, 0xA8A5102A)
#define OVP_TypeId_ProcessType_AllInputs                             					OpenViBE::CIdentifier(0x68E5FAE6, 0x0C1A4382)
#define OVP_TypeId_ProcessType_EachInput                                        		OpenViBE::CIdentifier(0x8145A095, 0xABB3F4D4)


#define OVP_TypeId_ParadigmType                                        					OpenViBE::CIdentifier(0x9CA20220, 0x0EA5834A)
#define OVP_TypeId_ParadigmType_SuperSplotch                                        	OpenViBE::CIdentifier(0xFDBD0EFC, 0x1DCE70BD)
#define OVP_TypeId_ParadigmType_RowCol_Splotch                                        	OpenViBE::CIdentifier(0x0B71AD9D, 0xAB39CB73)
#define OVP_TypeId_ParadigmType_OneGroup	                                        	OpenViBE::CIdentifier(0xA3349626, 0x9F065080)


#define OVP_TypeId_DecisionCriterium                                        			OpenViBE::CIdentifier(0x82CF26B8, 0xC23DF239)
#define OVP_TypeId_DecisionCriterium_Info                                        		OpenViBE::CIdentifier(0x03E742CF, 0x0012F17F)
#define OVP_TypeId_DecisionCriterium_Lin                                        		OpenViBE::CIdentifier(0x2942AA1F, 0xE8452C82)
#define OVP_TypeId_DecisionCriterium_Exp                                        		OpenViBE::CIdentifier(0x6D41FFC2, 0x80B80135)
#define OVP_TypeId_DecisionCriterium_Util                                        		OpenViBE::CIdentifier(0xE59BC96E, 0xE244823D)


#define OVP_Algorithm_LikelihoodComputeFunction_InputParameterId_MatrixSignal         OpenViBE::CIdentifier(0x9A8E6688, 0x97A2E552)
#define OVP_Algorithm_LikelihoodComputeFunction_InputParameterId_MatrixMean       OpenViBE::CIdentifier(0x284DC4D0, 0x94DC62FD)
#define OVP_Algorithm_LikelihoodComputeFunction_InputParameterId_MatrixVariance       OpenViBE::CIdentifier(0x96B02362, 0x509A2DC7)
#define OVP_Algorithm_LikelihoodComputeFunction_InputParameterId_LogTerm     		OpenViBE::CIdentifier(0xD45DC509, 0x0F7BEAF7)
#define OVP_Algorithm_LikelihoodComputeFunction_InputParameterId_PriorProba      OpenViBE::CIdentifier(0x745684C7, 0x1EC96F5A)

#define OVP_Algorithm_LikelihoodComputeFunction_OutputParameterId_MatrixClassFunctional     OpenViBE::CIdentifier(0xF3E3C28D, 0xBD2AABC4)

#define OVP_Algorithm_LikelihoodComputeFunction_InputTriggerId_Initialize    		OpenViBE::CIdentifier(0xB268DC03, 0xE4F9F48D)
#define OVP_Algorithm_LikelihoodComputeFunction_InputTriggerId_Apply   			OpenViBE::CIdentifier(0xD653D8D9, 0x8C9670E1)

#define OVP_ClassId_Algorithm_LikelihoodComputeFunction        				OpenViBE::CIdentifier(0x0E721B46, 0xAD80E2CF)
#define OVP_ClassId_Algorithm_LikelihoodComputeDesc       				OpenViBE::CIdentifier(0xE04FBCFB, 0xCDC654A5)


#define  OVP_ClassId_Box_DynamicNaiveBayesComputeBoxAlgorithmDesc                                    	OpenViBE::CIdentifier(0xCB8FF0EA, 0x0587C94C)
#define  OVP_ClassId_Box_DynamicNaiveBayesComputeBoxAlgorithm                                        	OpenViBE::CIdentifier(0xFFE62355, 0x3A0A0B57)

#define OVP_Algorithm_NaiveBayesCompute_InputParameterId_Alpha                        	OpenViBE::CIdentifier(0xD0F2E543, 0xD6CA53C1)
#define OVP_Algorithm_NaiveBayesCompute_InputParameterId_NbRepet                        OpenViBE::CIdentifier(0x8203C224, 0xACF46B7C)


//___________________________________________________________________//
//                                                                   //
// Gloabal defines                                                   //
//___________________________________________________________________//
//                                                                   //

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
 #include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

//___________________________________________________________________//
//                                                                   //
// Operating System identification                                   //
//___________________________________________________________________//
//                                                                   //

// #define OVP_OS_Linux
// #define OVP_OS_Windows
// #define OVP_OS_MacOS
// #define OVP_OS_

#if defined TARGET_OS_Windows
 #define OVP_OS_Windows
#elif defined TARGET_OS_Linux
 #define OVP_OS_Linux
#elif defined TARGET_OS_MacOS
 #define OVP_OS_MacOS
#else
 #warning "No target operating system defined !"
#endif

#endif // __OpenViBEPlugins_Defines_H__
