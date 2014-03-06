#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //
#define OVP_ClassId_LDAClassifierDesc                       OpenViBE::CIdentifier(0x1AE009FE, 0xF4FB82FB)


//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                                   //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_LDAClassifier                           OpenViBE::CIdentifier(0x49F18236, 0x75AE12FD)

//___________________________________________________________________//
//                                                                   //
// Global defines                                                   //
//___________________________________________________________________//
//                                                                   //

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
 #include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines


#endif // __OpenViBEPlugins_Defines_H__
