#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_GenericStreamReaderDesc                                    OpenViBE::CIdentifier(0x1E8AAB1A, 0x085D72F6)
#define OVP_ClassId_GenericStreamWriterDesc                                    OpenViBE::CIdentifier(0x02817C77, 0x77FE3D6A)
#define OVP_ClassId_BoxAlgorithm_CSVFileWriterDesc 							   OpenViBE::CIdentifier(0x65075FF7, 0x2B555E97)
#define OVP_ClassId_BoxAlgorithm_CSVFileReaderDesc 							   OpenViBE::CIdentifier(0x193F22E9, 0x26A67233)




//-------------------------------------------------------------------//

//-------------------------------------------------------------------//
// OV Matrix File Reader
#define OVP_ClassId_Algorithm_OVMatrixFileReader                               OpenViBE::CIdentifier(0x10661A33, 0x0B0F44A7)
#define OVP_ClassId_Algorithm_OVMatrixFileReaderDesc                           OpenViBE::CIdentifier(0x0E873B5E, 0x0A287FCB)

#define OVP_Algorithm_OVMatrixFileReader_InputParameterId_Filename             OpenViBE::CIdentifier(0x28F87B29, 0x0B09737E)

#define OVP_Algorithm_OVMatrixFileReader_OutputParameterId_Matrix              OpenViBE::CIdentifier(0x2F9521E0, 0x027D789F)

#define OVP_Algorithm_OVMatrixFileReader_InputTriggerId_Open                   OpenViBE::CIdentifier(0x2F996376, 0x2A942485)
#define OVP_Algorithm_OVMatrixFileReader_InputTriggerId_Load                   OpenViBE::CIdentifier(0x22841807, 0x102D681C)
#define OVP_Algorithm_OVMatrixFileReader_InputTriggerId_Close                  OpenViBE::CIdentifier(0x7FDE77DA, 0x384A0B3D)

#define OVP_Algorithm_OVMatrixFileReader_OutputTriggerId_Error                 OpenViBE::CIdentifier(0x6D4F2F4B, 0x05EC6CB9)
#define OVP_Algorithm_OVMatrixFileReader_OutputTriggerId_DataProduced          OpenViBE::CIdentifier(0x76F46051, 0x003B6FE8)

//-------------------------------------------------------------------//

//-------------------------------------------------------------------//
// OV Matrix File Writer
#define OVP_ClassId_Algorithm_OVMatrixFileWriter                               OpenViBE::CIdentifier(0x739158FC, 0x1E8240CC)
#define OVP_ClassId_Algorithm_OVMatrixFileWriterDesc                           OpenViBE::CIdentifier(0x44CF6DD0, 0x329D47F9)

#define OVP_Algorithm_OVMatrixFileWriter_InputParameterId_Filename             OpenViBE::CIdentifier(0x330D2D0B, 0x175271E6)
#define OVP_Algorithm_OVMatrixFileWriter_InputParameterId_Matrix               OpenViBE::CIdentifier(0x6F6402EE, 0x493044F3)
//-------------------------------------------------------------------//

//-------------------------------------------------------------------//
// ElectrodeLocalisationFileReader
#define OVP_ClassId_BoxAlgorithm_ElectrodeLocalisationFileReader               OpenViBE::CIdentifier(0x40704155, 0x19C50E8F)
#define OVP_ClassId_BoxAlgorithm_ElectrodeLocalisationFileReaderDesc           OpenViBE::CIdentifier(0x4796613F, 0x653A48D5)
//-------------------------------------------------------------------//

//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                                   //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_GenericStreamReader                                        OpenViBE::CIdentifier(0x0B1D880D, 0x02A17229)
#define OVP_ClassId_GenericStreamWriter                                        OpenViBE::CIdentifier(0x78EA86B0, 0x2933E255)
#define OVP_ClassId_BoxAlgorithm_CSVFileWriter     							   OpenViBE::CIdentifier(0x2C9312F1, 0x2D6613E5)
#define OVP_ClassId_BoxAlgorithm_CSVFileReader     							   OpenViBE::CIdentifier(0x641D0717, 0x02884107)

//___________________________________________________________________//
//                                                                   //
//                                                                   //
//___________________________________________________________________//
//                                                                   //

#define OVP_NodeId_OpenViBEStream_Header              EBML::CIdentifier(0xF59505AB, 0x3684C8D8)
#define OVP_NodeId_OpenViBEStream_Header_Compression  EBML::CIdentifier(0x40358769, 0x166380D1)
#define OVP_NodeId_OpenViBEStream_Header_StreamType   EBML::CIdentifier(0x732EC1D1, 0xFE904087)
#define OVP_NodeId_OpenViBEStream_Header_ChannelType  OVP_NodeId_OpenViBEStream_Header_StreamType  // deprecated old name
#define OVP_NodeId_OpenViBEStream_Buffer              EBML::CIdentifier(0x2E60AD18, 0x87A29BDF)
#define OVP_NodeId_OpenViBEStream_Buffer_StreamIndex  EBML::CIdentifier(0x30A56D8A, 0xB9C12238) 
#define OVP_NodeId_OpenViBEStream_Buffer_ChannelIndex OVP_NodeId_OpenViBEStream_Buffer_StreamIndex // deprecated old name
#define OVP_NodeId_OpenViBEStream_Buffer_StartTime    EBML::CIdentifier(0x093E6A0A, 0xC5A9467B)
#define OVP_NodeId_OpenViBEStream_Buffer_EndTime      EBML::CIdentifier(0x8B5CCCD9, 0xC5024F29)
#define OVP_NodeId_OpenViBEStream_Buffer_Content      EBML::CIdentifier(0x8D4B0BE8, 0x7051265C)

//___________________________________________________________________//
//                                                                   //
// Global defines                                                   //
//___________________________________________________________________//
//                                                                   //

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
 #include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines



#endif // __OpenViBEPlugins_Defines_H__
