#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_BoxAlgorithm_SharedMemoryWriter 		OpenViBE::CIdentifier(0xACC272DD, 0xC1BDC1B1)
#define OVP_ClassId_BoxAlgorithm_SharedMemoryWriterDesc 	OpenViBE::CIdentifier(0xACC272DD, 0xC1BDC1B1)
#define OVP_ClassId_BoxAlgorithm_StimulationBasedFileReader 	OpenViBE::CIdentifier(0xC06B2F47, 0x9942CD32)
#define OVP_ClassId_BoxAlgorithm_StimulationBasedFileReaderDesc OpenViBE::CIdentifier(0xC06B2F47, 0x9942CD32)

/*#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x335384EA, 0x88C917D9)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0xA15EAEC5, 0xAB0CE73D)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0xE405260B, 0x59EEFAE4)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x1E1065B2, 0x2CA32013)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x1E1065B2, 0x2CA32013)
*/

//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                                   //
//___________________________________________________________________//
//                                                                   //

/*
#															OpenViBE::CIdentifier(0x7FEFDCA9, 0x816ED903)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x781F51CA, 0xE6E3B0B8)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0xE5103C63, 0x08D825E0)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x043A1BC4, 0x925D3CD6)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x5CF66A73, 0xF5BBF0BF)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0xD5C5EF91, 0xE1B1C4F4)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0xEBAEB213, 0xDD4735A0)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x344A52F5, 0x489DB439)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x2F9ECA0B, 0x8D3CA7BD)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x1BD67420, 0x587600E6)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0xB083614E, 0x26C6B4BD)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x7D13B924, 0xF194DA09)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x2397E74F, 0xCAC8F95C)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0xD6E1AC79, 0xBE50C28F)
#define OVP_ClassId_                                        OpenViBE::CIdentifier(0x5E8D9B97, 0xF82F92D7)
*/

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
