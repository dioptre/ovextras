#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //


#define OVP_ClassId_BoxAlgorithm_TwoSampleTTestDesc		OpenViBE::CIdentifier(0x020300BF, 0xB4BAC0A1)
#define OVP_ClassId_BoxAlgorithm_LikelinessDistributorDesc      OpenViBE::CIdentifier(0xE5103C63, 0x08D825E0)
#define OVP_ClassId_BoxAlgorithm_EpochVarianceDesc              OpenViBE::CIdentifier(0xA15EAEC5, 0xAB0CE73D)
#define OVP_ClassId_Algorithm_MatrixVarianceDesc 		OpenViBE::CIdentifier(0xE405260B, 0x59EEFAE4)

/*#define OVP_ClassId_                                        OpenViBE::CIdentifier(0xE405260B, 0x59EEFAE4)                                    
*/

//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                                   //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_BoxAlgorithm_TwoSampleTTest			OpenViBE::CIdentifier(0x16DFF805, 0x0BC431BF)
#define OVP_ClassId_BoxAlgorithm_LikelinessDistributor		OpenViBE::CIdentifier(0x781F51CA, 0xE6E3B0B8)
#define OVP_ClassId_BoxAlgorithm_EpochVariance                  OpenViBE::CIdentifier(0x335384EA, 0x88C917D9)
#define OVP_ClassId_Algorithm_MatrixVariance 			OpenViBE::CIdentifier(0x7FEFDCA9, 0x816ED903)


#define OVP_TypeId_EpochAverageMethod                                                  OpenViBE::CIdentifier(0x5E8D9B97, 0xF82F92D7)
#define OVP_TypeId_EpochAverageMethod_MovingAverage                                    OpenViBE::CIdentifier(0xD6E1AC79, 0xBE50C28F)
#define OVP_TypeId_EpochAverageMethod_MovingAverageImmediate                           OpenViBE::CIdentifier(0x2397E74F, 0xCAC8F95C)
#define OVP_TypeId_EpochAverageMethod_BlockAverage                                     OpenViBE::CIdentifier(0x7D13B924, 0xF194DA09)
#define OVP_TypeId_EpochAverageMethod_CumulativeAverage                                OpenViBE::CIdentifier(0xB083614E, 0x26C6B4BD)

#define OVP_Algorithm_MatrixVariance_InputParameterId_Matrix			OpenViBE::CIdentifier(0x781F51CA, 0xE6E3B0B8)
#define OVP_Algorithm_MatrixVariance_InputParameterId_MatrixCount		OpenViBE::CIdentifier(0xE5103C63, 0x08D825E0)
#define OVP_Algorithm_MatrixVariance_InputParameterId_AveragingMethod		OpenViBE::CIdentifier(0x043A1BC4, 0x925D3CD6)
#define OVP_Algorithm_MatrixVariance_InputParameterId_SignificanceLevel		OpenViBE::CIdentifier(0x1E1065B2, 0x2CA32013)
#define OVP_Algorithm_MatrixVariance_OutputParameterId_AveragedMatrix		OpenViBE::CIdentifier(0x5CF66A73, 0xF5BBF0BF)
#define OVP_Algorithm_MatrixVariance_OutputParameterId_Variance			OpenViBE::CIdentifier(0x1BD67420, 0x587600E6)
#define OVP_Algorithm_MatrixVariance_OutputParameterId_ConfidenceBound		OpenViBE::CIdentifier(0x1E1065B2, 0x2CA32013)
#define OVP_Algorithm_MatrixVariance_InputTriggerId_Reset			OpenViBE::CIdentifier(0xD5C5EF91, 0xE1B1C4F4)
#define OVP_Algorithm_MatrixVariance_InputTriggerId_FeedMatrix			OpenViBE::CIdentifier(0xEBAEB213, 0xDD4735A0)
#define OVP_Algorithm_MatrixVariance_InputTriggerId_ForceAverage		OpenViBE::CIdentifier(0x344A52F5, 0x489DB439)
#define OVP_Algorithm_MatrixVariance_OutputTriggerId_AveragePerformed		OpenViBE::CIdentifier(0x2F9ECA0B, 0x8D3CA7BD)

#define OVP_ClassId_BoxAlgorithm_XDAWNSpatialFilterTrainerCoAdapt     OpenViBE::CIdentifier(0xEE31A115, 0x00B25FE1)
#define OVP_ClassId_BoxAlgorithm_XDAWNSpatialFilterTrainerCoAdaptDesc OpenViBE::CIdentifier(0xB59256B0, 0xC3389515)
#define OVP_ClassId_BoxAlgorithm_SpatialFilterWithUpdate     OpenViBE::CIdentifier(0x8E7CD4B3, 0xDAAD41C1)
#define OVP_ClassId_BoxAlgorithm_SpatialFilterWithUpdateDesc OpenViBE::CIdentifier(0xE2E6A1D6, 0xCBCEEE3A)

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
