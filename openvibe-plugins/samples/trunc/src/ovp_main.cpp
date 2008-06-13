#include "ovp_defines.h"

#include "ovpCScenarioImporterXML.h"
#include "ovpCScenarioExporterXML.h"

#include "ovpCCrashingBox.h"
#include "ovpCBoxAlgorithmProducer.h"
#include "ovpCBoxAlgorithmConsumer.h"
#include "ovpCSinusSignalGenerator.h"
#include "ovpCTimeSignalGenerator.h"
#include "ovpCIdentity.h"
#include "ovpCLog.h"
#include "ovpCTest.h"
#include "ovpCBoxAlgorithmClock.h"

#include "algorithms/ovpCAlgorithmAddition.h"
#include "ovpCBoxAlgorithmAdditionTest.h"

#include "ovpCBoxAlgorithmNothing.h"

// #define _WithTBE_

#if defined _WithTBE_
#include "ovpCTimeBasedEpoching.h"
#endif

OVP_Declare_Begin();
	rPluginModuleContext.getTypeManager().registerStreamType(OpenViBE::CIdentifier(0x1234,0x5678), "producer/consumer", OV_UndefinedIdentifier);

	rPluginModuleContext.getTypeManager().registerEnumerationType(OpenViBE::CIdentifier(0x1234,0x0000), "sample enumeration");
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OpenViBE::CIdentifier(0x1234,0x0000), "enum value 1", 1);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OpenViBE::CIdentifier(0x1234,0x0000), "enum value 2", 2);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OpenViBE::CIdentifier(0x1234,0x0000), "enum value 3", 3);
	rPluginModuleContext.getTypeManager().registerEnumerationEntry(OpenViBE::CIdentifier(0x1234,0x0000), "enum value 4", 4);

	rPluginModuleContext.getTypeManager().registerBitMaskType(OpenViBE::CIdentifier(0x0000,0x5678), "sample bitmask");
	rPluginModuleContext.getTypeManager().registerBitMaskEntry(OpenViBE::CIdentifier(0x0000,0x5678), "bitmask value 1", 1);
	rPluginModuleContext.getTypeManager().registerBitMaskEntry(OpenViBE::CIdentifier(0x0000,0x5678), "bitmask value 2", 2);
	rPluginModuleContext.getTypeManager().registerBitMaskEntry(OpenViBE::CIdentifier(0x0000,0x5678), "bitmask value 3", 3);
	rPluginModuleContext.getTypeManager().registerBitMaskEntry(OpenViBE::CIdentifier(0x0000,0x5678), "bitmask value 4", 4);

#if defined _WithTBE_
	OVP_Declare_New(OpenViBEPlugins::Samples::CTimeBasedEpochingDesc);
#endif

	OVP_Declare_New(OpenViBEPlugins::Samples::CScenarioImporterXMLDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CScenarioExporterXMLDesc);

	OVP_Declare_New(OpenViBEPlugins::Samples::CCrashingBoxDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CBoxAlgorithmProducerDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CBoxAlgorithmConsumerDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CSinusSignalGeneratorDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CTimeSignalGeneratorDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CIdentityDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CLogDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CBoxAlgorithmClockDesc);

	// OVP_Declare_New(OpenViBEPlugins::Samples::CTestDesc);

	OVP_Declare_New(OpenViBEPlugins::Samples::CBoxAlgorithmAdditionTestDesc);
	OVP_Declare_New(OpenViBEPlugins::Samples::CAlgorithmAdditionDesc);

	OVP_Declare_New(OpenViBEPlugins::Samples::CBoxAlgorithmNothingDesc);
OVP_Declare_End();
