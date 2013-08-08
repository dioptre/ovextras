#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_BoxAlgorithm_P300SpellerVisualisationSplotch     	OpenViBE::CIdentifier(0x915E41D6, 0xE6684D47)
#define OVP_ClassId_BoxAlgorithm_SerialP300SpellerVisualisation      	OpenViBE::CIdentifier(0x2558CB57, 0x3A6245D5)
#define OVP_ClassId_BoxAlgorithm_ErpPlot 				OpenViBE::CIdentifier(0x10DC6917, 0x2B29B2A0)
//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                                   //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_BoxAlgorithm_P300SpellerVisualisationSplotchDesc 	OpenViBE::CIdentifier(0x13DE2B0D, 0x208202E7)
#define OVP_ClassId_BoxAlgorithm_SerialP300SpellerVisualisationDesc  	OpenViBE::CIdentifier(0x76AC40CA, 0x1556B357)
#define OVP_ClassId_BoxAlgorithm_ErpPlotDesc 			     	OpenViBE::CIdentifier(0x10DC6917, 0x2B29B2A0)

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
// Some enumerations                                                 //
//___________________________________________________________________//
//                                                                   //

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{
		enum EDisplayMode
		{
			//DisplayMode_Default,
			DisplayMode_ZoomIn,
			DisplayMode_ZoomOut,
			//DisplayMode_BestFit,
			DisplayMode_GlobalBestFit,
			//DisplayMode_Normal,
		};
	};
};

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

//___________________________________________________________________//
//                                                                   //
// Hardware Architecture identification                              //
//___________________________________________________________________//
//                                                                   //

// #define OVP_ARCHITECTURE_i386
// #define OVP_ARCHITECTURE_

#if defined TARGET_ARCHITECTURE_i386
 #define OVP_ARCHITECTURE_i386
#else
 #warning "No target architecture defined !"
#endif

//___________________________________________________________________//
//                                                                   //
// Compilator software identification                                //
//___________________________________________________________________//
//                                                                   //

// #define OVP_COMPILATOR_GCC
// #define OVP_COMPILATOR_VisualStudio
// #define OVP_COMPILATOR_

#if defined TARGET_COMPILATOR_GCC
 #define OVP_COMPILATOR_GCC
#elif defined TARGET_COMPILATOR_VisualStudio
 #define OVP_COMPILATOR_VisualStudio
#else
 #warning "No target compilator defined !"
#endif

//___________________________________________________________________//
//                                                                   //
// API Definition                                                    //
//___________________________________________________________________//
//                                                                   //

// Taken from
// - http://people.redhat.com/drepper/dsohowto.pdf
// - http://www.nedprod.com/programs/gccvisibility.html
#if defined OVP_Shared
 #if defined OVP_OS_Windows
  #define OVP_API_Export __declspec(dllexport)
  #define OVP_API_Import __declspec(dllimport)
 #elif defined OVP_OS_Linux
  #define OVP_API_Export __attribute__((visibility("default")))
  #define OVP_API_Import __attribute__((visibility("default")))
 #else
  #define OVP_API_Export
  #define OVP_API_Import
 #endif
#else
 #define OVP_API_Export
 #define OVP_API_Import
#endif

#if defined OVP_Exports
 #define OVP_API OVP_API_Export
#else
 #define OVP_API OVP_API_Import
#endif

//___________________________________________________________________//
//                                                                   //
// NULL Definition                                                   //
//___________________________________________________________________//
//                                                                   //

#ifndef NULL
#define NULL 0
#endif

#endif // __OpenViBEPlugins_Defines_H__
