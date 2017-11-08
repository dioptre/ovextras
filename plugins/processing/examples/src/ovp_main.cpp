#include "ovp_defines.h"

#include "box-algorithms/ovpCHelloWorld.h"
#include "box-algorithms/ovpCHelloWorldWithInput.h"

#include "box-algorithms/ovpCLog.h"
#include "box-algorithms/ovpCBoxAlgorithmNothing.h"
#include "box-algorithms/ovpCBoxAlgorithmClock.h"

OVP_Declare_Begin();
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(), OV_AttributeId_Box_FlagIsUnstable.toUInteger());

	OVP_Declare_New(OpenViBEPlugins::Examples::CHelloWorldDesc);
	OVP_Declare_New(OpenViBEPlugins::Examples::CHelloWorldWithInputDesc);

	OVP_Declare_New(OpenViBEPlugins::Examples::CLogDesc);
	OVP_Declare_New(OpenViBEPlugins::Examples::CBoxAlgorithmNothingDesc);
	OVP_Declare_New(OpenViBEPlugins::Examples::CBoxAlgorithmClockDesc);

OVP_Declare_End();
