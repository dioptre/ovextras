#ifndef __OpenViBEDesigner_CBoxConfigurationDialog_H__
#define __OpenViBEDesigner_CBoxConfigurationDialog_H__

#include "ovd_base.h"

#include <string>
#include <vector>
#include <map>
#include "ovdCSettingCollectionHelper.h"
#include "settings/ovdCAbstractSettingView.h"

namespace OpenViBEDesigner
{
	struct SButtonCB
	{
		SButtonCB(const OpenViBE::Kernel::IKernelContext& rKernelContext,
			std::map< OpenViBE::CString, ::GtkWidget* >& rSettingWidget,
			CSettingCollectionHelper& rHelper,
			::GtkWidget* pSettingOverrideValue,
			OpenViBE::Kernel::IBox& rBox) : 
				m_rKernelContext(rKernelContext),
				m_rSettingWidget(rSettingWidget), 
				m_rHelper(rHelper), 
				m_pSettingOverrideValue(pSettingOverrideValue),
				m_rBox(rBox)
		{ } ;

		const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		std::map< OpenViBE::CString, ::GtkWidget* >& m_rSettingWidget;
		CSettingCollectionHelper& m_rHelper;
		::GtkWidget* m_pSettingOverrideValue;
		OpenViBE::Kernel::IBox& m_rBox;
	};

	class CBoxConfigurationDialog : public OpenViBE::IObserver
	{
	public:

		CBoxConfigurationDialog(const OpenViBE::Kernel::IKernelContext& rKernelContext, OpenViBE::Kernel::IBox& rBox, const char* sGUIFilename, const char* sGUISettingsFilename, bool bMode=false);
		virtual ~CBoxConfigurationDialog(void);
		virtual OpenViBE::boolean run(bool bMode);
		virtual ::GtkWidget* getWidget();
		virtual OpenViBE::boolean update(void);
		virtual const OpenViBE::CIdentifier getBoxID() const;

		virtual void update(OpenViBE::CObservable &o, void* data);

	protected:

		const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		OpenViBE::Kernel::IBox& m_rBox;
		OpenViBE::CString m_sGUIFilename;
		OpenViBE::CString m_sGUISettingsFilename;
		//
		::GtkWidget* m_pWidget;//widget with the dialog for configuration (used whole for box config when no scenario is running)
		::GtkWidget* m_pWidgetToReturn; //child of m_oWidget, if we are running a scenario, this is the widget we need, the rest can be discarded
		::GtkWidget* m_pSettingOverrideValue;
		bool m_bIsScenarioRunning; // true if the scenario is running, false otherwise
		std::map< OpenViBE::CString, ::GtkWidget* > m_mSettingWidget;

		std::map< OpenViBE::CString, Setting::CAbstractSettingView* > m_mSettingViewMap;//Temporary need to be remove
		CSettingCollectionHelper* m_pHelper;
		SButtonCB* m_pButtonCB;
		::GtkCheckButton* m_pFileOverrideCheck;
	};
};

#endif // __OpenViBEDesigner_CBoxConfigurationDialog_H__
