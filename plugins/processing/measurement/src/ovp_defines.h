 
#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

//___________________________________________________________________//
//                                                                   //
// Box algorithms                                                    //
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

namespace{
	const char* const c_sStatisticRootNodeName = "Statistic";
	const char* const c_sStimulationListNodeName = "Stimulations-list";
	const char* const c_sStimulationNodeName = "Stimulation";
	const char* const c_sIdentifierNodeName = "Identifier";
	const char* const c_sAmountNodeName = "Amount";
}


#endif // __OpenViBEPlugins_Defines_H__
