#ifndef __OpenViBEKernel_Kernel_Player_CBoxSettingModifierVisitor_H__
#define __OpenViBEKernel_Kernel_Player_CBoxSettingModifierVisitor_H__

#include "ovkCSimulatedBox.h"
#include "ovkCPlayer.h"

#include <openvibe/ovIObjectVisitor.h>
#include <openvibe/kernel/ovIObjectVisitorContext.h>

#include <xml/IReader.h>

#define OVD_AttributeId_SettingOverrideFilename             OpenViBE::CIdentifier(0x8D21FF41, 0xDF6AFE7E)

class CBoxSettingModifierVisitor : public OpenViBE::IObjectVisitor, public XML::IReaderCallback
{
public:


	CBoxSettingModifierVisitor(OpenViBE::Kernel::IConfigurationManager* pConfigurationManager = NULL) :
		OpenViBE::IObjectVisitor(),
		m_pConfigurationManager(pConfigurationManager)
	{}

	virtual void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount);

	virtual void processChildData(const char* sData);	

	virtual void closeChild(void);

	virtual OpenViBE::boolean processBegin(OpenViBE::Kernel::IObjectVisitorContext& rObjectVisitorContext, OpenViBE::Kernel::IBox& rBox);

	virtual OpenViBE::boolean processEnd(OpenViBE::Kernel::IObjectVisitorContext& rObjectVisitorContext, OpenViBE::Kernel::IBox& rBox);

	OpenViBE::Kernel::IObjectVisitorContext* m_pObjectVisitorContext;
	OpenViBE::Kernel::IBox* m_pBox;
	OpenViBE::uint32 m_ui32SettingIndex;
	OpenViBE::boolean m_bIsParsingSettingValue;
	OpenViBE::boolean m_bIsParsingSettingOverride;
	OpenViBE::Kernel::IConfigurationManager* m_pConfigurationManager;

#undef boolean
	_IsDerivedFromClass_Final_(OpenViBE::IObjectVisitor, OV_UndefinedIdentifier);
#define boolean OpenViBE::boolean
};

#endif