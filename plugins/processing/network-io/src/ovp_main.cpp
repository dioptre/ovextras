
#include "ovp_defines.h"

#include "box-algorithms/ovpCBoxAlgorithmTCPWriter.h"

OVP_Declare_Begin();
	
	OVP_Declare_New(OpenViBEPlugins::NetworkIO::CBoxAlgorithmTCPWriterDesc);

	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeID_TCPWriter_OutputStyle,"Stimulus output");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeID_TCPWriter_OutputStyle,"Raw",TCPWRITER_RAW);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeID_TCPWriter_OutputStyle,"Hex",TCPWRITER_HEX);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeID_TCPWriter_OutputStyle,"String",TCPWRITER_STRING);

OVP_Declare_End();
