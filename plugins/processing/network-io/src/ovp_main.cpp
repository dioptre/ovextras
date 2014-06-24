
#include "ovp_defines.h"

#include "box-algorithms/ovpCBoxAlgorithmTCPWriter.h"
#include "box-algorithms/ovpCBoxAlgorithmSharedMemoryWriter.h"

OVP_Declare_Begin();
	
#ifdef TARGET_HAS_Boost
	OVP_Declare_New(OpenViBEPlugins::FileReadingAndWriting::CBoxAlgorithmSharedMemoryWriterDesc);
	OVP_Declare_New(OpenViBEPlugins::NetworkIO::CBoxAlgorithmTCPWriterDesc);

	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeID_TCPWriter_OutputStyle,"Stimulus output");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeID_TCPWriter_OutputStyle,"Raw",TCPWRITER_RAW);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeID_TCPWriter_OutputStyle,"Hex",TCPWRITER_HEX);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeID_TCPWriter_OutputStyle,"String",TCPWRITER_STRING);
#endif

OVP_Declare_End();
