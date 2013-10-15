#include "ovp_defines.h"

#include "box-algorithms/ovpCHelloWorld.h"
#include "box-algorithms/ovpCHelloWorldWithInput.h"
#include "box-algorithms/ovpCBoxAlgorithmModUI.h"

OVP_Declare_Begin();

	OVP_Declare_New(OpenViBEPlugins::Samples::CHelloWorldDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CHelloWorldWithInputDesc);
	OVP_Declare_New(OpenViBEPlugins::Examples::CBoxAlgorithmModUIDesc);

OVP_Declare_End();
