#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

//___________________________________________________________________//
//                                                                   //
//                                                                   //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_BoxAlgorithm_StimulationMultiplexer       OpenViBE::CIdentifier(0x07DB4EFA, 0x472B0938)
#define OVP_ClassId_BoxAlgorithm_StimulationMultiplexerDesc   OpenViBE::CIdentifier(0x79EF4E4D, 0x178F09E6)


#define OVP_ClassId_BoxAlgorithm_StimulationVoter                      OpenViBE::CIdentifier(0x2BBD61FC, 0x041A4EDB)
#define OVP_ClassId_BoxAlgorithm_StimulationVoterDesc                  OpenViBE::CIdentifier(0x1C36287C, 0x6F143FBF)
#define OVP_TypeId_Voting_ClearVotes                                   OpenViBE::CIdentifier(0x17AE30F8, 0x40B57661)
#define OVP_TypeId_Voting_ClearVotes_AfterOutput                       OpenViBE::CIdentifier(0x7FA81A20, 0x484023F9)
#define OVP_TypeId_Voting_ClearVotes_WhenExpires                       OpenViBE::CIdentifier(0x02766639, 0x00B155B4)
#define OVP_TypeId_Voting_OutputTime                                   OpenViBE::CIdentifier(0x48583E8F, 0x47F22462)
#define OVP_TypeId_Voting_OutputTime_Vote                              OpenViBE::CIdentifier(0x2F37507F, 0x00C06761)
#define OVP_TypeId_Voting_OutputTime_Winner                            OpenViBE::CIdentifier(0x72416689, 0x17673658)
#define OVP_TypeId_Voting_OutputTime_Last                              OpenViBE::CIdentifier(0x4F2830DB, 0x716C2930)
#define OVP_TypeId_Voting_RejectClass_CanWin                                 OpenViBE::CIdentifier(0x442F2F14, 0x7A17336C)
#define OVP_TypeId_Voting_RejectClass_CanWin_Yes                             OpenViBE::CIdentifier(0x40011974, 0x54BB3C71)
#define OVP_TypeId_Voting_RejectClass_CanWin_No                              OpenViBE::CIdentifier(0x275B746A, 0x480B302C)

//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //


//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                                   //
//___________________________________________________________________//
//



//___________________________________________________________________//
//                                                                   //
// Global defines                                                   //
//___________________________________________________________________//
//                                                                   //

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
 #include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines


#endif // __OpenViBEPlugins_Defines_H__
