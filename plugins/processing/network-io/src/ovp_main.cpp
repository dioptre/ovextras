
#include "ovp_defines.h"

#include "box-algorithms/ovpCBoxAlgorithmStimulusTCPWriter.h"

OVP_Declare_Begin();

	OVP_Declare_New(OpenViBEPlugins::NetworkIO::CBoxAlgorithmStimulusTCPWriterDesc);

	rPluginModuleContext.getTypeManager().registerEnumerationType(OVP_TypeID_StimulusTCPWriter_OutputStyle,"Output style");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeID_StimulusTCPWriter_OutputStyle,"Raw",TCPWRITER_RAW);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeID_StimulusTCPWriter_OutputStyle,"Hex",TCPWRITER_HEX);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OVP_TypeID_StimulusTCPWriter_OutputStyle,"String",TCPWRITER_STRING);

OVP_Declare_End();
