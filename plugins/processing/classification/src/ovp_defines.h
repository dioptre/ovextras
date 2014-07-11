#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

#define OVP_Classification_BoxTrainerXMLVersion								2

//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //
#define OVP_ClassId_LDAClassifierDesc										OpenViBE::CIdentifier(0x1AE009FE, 0xF4FB82FB)


//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                                   //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_LDAClassifier											OpenViBE::CIdentifier(0x49F18236, 0x75AE12FD)

//___________________________________________________________________//
//                                                                   //
// Global defines                                                   //
//___________________________________________________________________//
//                                                                   //

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
 #include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines


#define OVP_TypeId_ClassificationPairwiseStrategy							OpenViBE::CIdentifier(0x0DD51C74, 0x3C4E74C9)


extern const char* const c_sXmlVersionAttributeName;
extern const char* const c_sIdentifierAttributeName;

extern const char* const c_sStrategyNodeName;
extern const char* const c_sAlgorithmNodeName;
extern const char* const c_sStimulationsNodeName;
extern const char* const c_sRejectedClassNodeName;
extern const char* const c_sClassStimulationNodeName;

extern const char* const c_sClassificationBoxRoot;
extern const char* const c_sClassifierRoot;

bool ov_float_equal(double f64First, double f64Second);

#endif // __OpenViBEPlugins_Defines_H__
