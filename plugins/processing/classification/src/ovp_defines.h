#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

#define OVP_Classification_BoxTrainerFormatVersion                  4
#define OVP_Classification_BoxTrainerFormatVersionRequired          4

//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //

//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                                   //
//___________________________________________________________________//
//                                                                   //

//___________________________________________________________________//
//                                                                   //
// Global defines                                                   //
//___________________________________________________________________//
//                                                                   //

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
 #include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines


#define OVP_TypeId_ClassificationPairwiseStrategy							OpenViBE::CIdentifier(0x0DD51C74, 0x3C4E74C9)


extern const char* const c_sFormatVersionAttributeName;
extern const char* const c_sIdentifierAttributeName;

extern const char* const c_sStrategyNodeName;
extern const char* const c_sAlgorithmNodeName;
extern const char* const c_sStimulationsNodeName;
extern const char* const c_sRejectedClassNodeName;
extern const char* const c_sClassStimulationNodeName;

extern const char* const c_sClassificationBoxRoot;
extern const char* const c_sClassifierRoot;

extern const char* const c_sPairwiseStrategyEnumerationName;

extern const char* const c_sMLPEvaluationFunctionName;
extern const char* const c_sMLPTransfertFunctionName;

bool ov_float_equal(double f64First, double f64Second);

#endif // __OpenViBEPlugins_Defines_H__
