
#include <vector>
#include <openvibe/ov_all.h>
#include "ovp_defines.h"

#include "box-algorithms/ovpCMouseControl.h"							// inserm

OVP_Declare_Begin();

// @BEGIN inserm
	OVP_Declare_New(OpenViBEPlugins::Tools::CMouseControlDesc);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(), OV_AttributeId_Box_FlagIsUnstable.toUInteger());
// @END inserm

	 
OVP_Declare_End();
