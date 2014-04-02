#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //
#define OVP_ClassId_BoxAlgorithm_ErpPlot 				OpenViBE::CIdentifier(0x10DC6917, 0x2B29B2A0)
//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                                   //
//___________________________________________________________________//
//                                                                   //

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

#endif // __OpenViBEPlugins_Defines_H__
