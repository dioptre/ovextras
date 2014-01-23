#ifndef __OpenViBEDesigner_CBoxConfigurationDialog_H__
#define __OpenViBEDesigner_CBoxConfigurationDialog_H__

#include "ovd_base.h"

#include <string>
#include <vector>
#include "ovdCSettingCollectionHelper.h"

namespace OpenViBEDesigner
{
	typedef struct
	{
		const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		std::vector< ::GtkWidget* >& m_vSettingValue;
		CSettingCollectionHelper& m_rHelper;
		::GtkWidget* m_pSettingOverrideValue;
		OpenViBE::Kernel::IBox& m_rBox;
	} SButtonCB;

	class CBoxConfigurationDialog
	{
	public:

		CBoxConfigurationDialog(const OpenViBE::Kernel::IKernelContext& rKernelContext, OpenViBE::Kernel::IBox& rBox, const char* sGUIFilename, const char* sGUISettingsFilename, bool bMode=false);
		virtual ~CBoxConfigurationDialog(void);
		virtual OpenViBE::boolean run(bool bMode);
		virtual ::GtkWidget* getWidget();
		virtual OpenViBE::boolean update(void);
		virtual const OpenViBE::CIdentifier getBoxID() const;

	protected:

		const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		OpenViBE::Kernel::IBox& m_rBox;
		OpenViBE::CString m_sGUIFilename;
		OpenViBE::CString m_sGUISettingsFilename;
		//
		::GtkWidget* m_oWidget;
		::GtkWidget* m_oWidgetToReturn;
		::GtkWidget* m_pSettingOverrideValue;
		bool m_Mode;
		std::vector< ::GtkWidget* > m_vSettingValue;
		std::vector< OpenViBE::uint32> m_vModSettingIndex;
		CSettingCollectionHelper *m_oHelper;
		SButtonCB *m_oButton;
	};
};

#endif // __OpenViBEDesigner_CBoxConfigurationDialog_H__
