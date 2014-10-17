#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_CrashingBoxDesc                         OpenViBE::CIdentifier(0x009F54B9, 0x2B6A4922)
#define OVP_ClassId_SinusSignalGeneratorDesc                OpenViBE::CIdentifier(0x2633AFA2, 0x6974E32F)
#define OVP_ClassId_TimeSignalGeneratorDesc                 OpenViBE::CIdentifier(0x57AD8655, 0x1966B4DC)
#define OVP_ClassId_IdentityDesc                            OpenViBE::CIdentifier(0x54743810, 0x6A1A88CC)
#define OVP_ClassId_LogDesc                                 OpenViBE::CIdentifier(0x00780136, 0x57633D46)
#define OVP_ClassId_BoxAlgorithm_ClockDesc                  OpenViBE::CIdentifier(0x754C233D, 0x37DF04A3)

#define OVP_ClassId_AlgorithmAdditionDesc                   OpenViBE::CIdentifier(0x842E0B85, 0xA59FABC1)
#define OVP_ClassId_BoxAlgorithmAdditionTestDesc            OpenViBE::CIdentifier(0xB33EC315, 0xF63BC0C5)

#define OVP_ClassId_NoiseGeneratorDesc			            OpenViBE::CIdentifier(0x7237458A, 0x1F312C4A)
#define OVP_ClassId_NoiseGenerator			                OpenViBE::CIdentifier(0x0E3929F1, 0x15AF76B9)

//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                                   //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_CrashingBox                             OpenViBE::CIdentifier(0x00DAFD60, 0x39A58819)
#define OVP_ClassId_SinusSignalGenerator                    OpenViBE::CIdentifier(0x7E33BDB8, 0x68194A4A)
#define OVP_ClassId_TimeSignalGenerator                     OpenViBE::CIdentifier(0x28A5E7FF, 0x530095DE)
#define OVP_ClassId_Identity                                OpenViBE::CIdentifier(0x5DFFE431, 0x35215C50)
#define OVP_ClassId_Log                                     OpenViBE::CIdentifier(0x00BE3E25, 0x274F2075)
#define OVP_ClassId_BoxAlgorithm_Clock                      OpenViBE::CIdentifier(0x14CB4CFC, 0x6D064CB3)

#define OVP_ClassId_AlgorithmAddition                       OpenViBE::CIdentifier(0x75FCE50E, 0x8302FA91)
#define OVP_ClassId_BoxAlgorithmAdditionTest                OpenViBE::CIdentifier(0x534EB140, 0x15F41496)

//___________________________________________________________________//
//                                                                   //
// Unstables                                                         //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_BoxAlgorithm_MeanVariance               OpenViBE::CIdentifier(0x48D24C76, 0x625769DB)
#define OVP_ClassId_BoxAlgorithm_MeanVarianceDesc           OpenViBE::CIdentifier(0x584D6C5B, 0x055A1ABC)

//___________________________________________________________________//
//                                                                   //
// Global defines                                                   //
//___________________________________________________________________//
//                                                                   //

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
 #include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines



#endif // __OpenViBEPlugins_Defines_H__
