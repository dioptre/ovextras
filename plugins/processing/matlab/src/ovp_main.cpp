#include <openvibe/ov_all.h>

#include "ovp_defines.h"

#include "box-algorithms/ovpCBoxAlgorithmMatlabScripting.h"

#include <vector>

OVP_Declare_Begin()

#if defined TARGET_HAS_ThirdPartyMatlab

	OVP_Declare_New(OpenViBEPlugins::Matlab::CBoxAlgorithmMatlabScriptingDesc);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(), OV_AttributeId_Box_FlagIsUnstable.toUInteger());

#endif // TARGET_HAS_ThirdPartyMatlab

OVP_Declare_End()
