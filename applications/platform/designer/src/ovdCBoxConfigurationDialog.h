#ifndef __OpenViBEDesigner_CBoxConfigurationDialog_H__
#define __OpenViBEDesigner_CBoxConfigurationDialog_H__

#include "ovd_base.h"

#include <string>
#include <vector>
#include <map>
#include "ovdCSettingCollectionHelper.h"
#include "settings/ovdCAbstractSettingView.h"
#include "settings/ovdCSettingViewFactory.h"

namespace OpenViBEDesigner
{
	class CSettingViewWrapper{
	public:
		CSettingViewWrapper(OpenViBE::uint32 ui32SettingIndex,	Setting::CAbstractSettingView *pView):
			m_ui32SettingIndex(ui32SettingIndex), m_pView(pView)
		{}

		OpenViBE::uint32 m_ui32SettingIndex;
		Setting::CAbstractSettingView *m_pView;
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

		void updateSize();
		void saveConfiguration();
		void loadConfiguration();

	protected:
		void generateSettingsTable(void);
		OpenViBE::boolean addSettingsToView(OpenViBE::uint32 ui32SettingIndex, OpenViBE::uint32 ui32TableIndex);

		void settingChange(OpenViBE::uint32 ui32SettingIndex);
		void addSetting(OpenViBE::uint32 ui32SettingIndex);

		void clearSettingWrappersVector(void);
		void removeSetting(OpenViBE::uint32 ui32SettingIndex, OpenViBE::boolean bShift = true);
		OpenViBE::uint32 getTableIndex(OpenViBE::uint32 ui32SettingIndex);
		OpenViBE::uint32 getTableSize(void);

		const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		OpenViBE::Kernel::IBox& m_rBox;
		OpenViBE::CString m_sGUIFilename;
		OpenViBE::CString m_sGUISettingsFilename;
		//
		::GtkWidget* m_pWidget;//widget with the dialog for configuration (used whole for box config when no scenario is running)
		::GtkWidget* m_pWidgetToReturn; //child of m_oWidget, if we are running a scenario, this is the widget we need, the rest can be discarded
		::GtkWidget* m_pSettingOverrideValue;

		::GtkTable *m_pSettingsTable;
		::GtkViewport *m_pViewPort;
		::GtkScrolledWindow * m_pScrolledWindow;

		bool m_bIsScenarioRunning; // true if the scenario is running, false otherwise
		std::map< OpenViBE::CString, ::GtkWidget* > m_mSettingWidget;

		Setting::CSettingViewFactory m_oSettingFactory;
		std::map< OpenViBE::CString, Setting::CAbstractSettingView* > m_mSettingViewMap;//Temporary need to be remove
		std::map< OpenViBE::CString, OpenViBE::uint32 > m_mSettingViewIndexMap;

		std::vector<CSettingViewWrapper> m_vSettingWrappers;

		CSettingCollectionHelper* m_pHelper;
		::GtkCheckButton* m_pFileOverrideCheck;
	};
};

#endif // __OpenViBEDesigner_CBoxConfigurationDialog_H__
