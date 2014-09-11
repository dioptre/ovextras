#ifndef OVDCABSTRACTSETTINGVIEW_H
#define OVDCABSTRACTSETTINGVIEW_H

#include "../ovd_base.h"

namespace OpenViBEDesigner
{
	namespace Setting
	{
		class CAbstractSettingView{

		public:
			virtual OpenViBE::CString getSettingWidgetName(void);

			virtual ~CAbstractSettingView(void);

			virtual void getValue(OpenViBE::CString &rValue) const = 0;
			virtual void setValue(const OpenViBE::CString &rValue) = 0;

			virtual ::GtkWidget* getNameWidget(void);
			virtual ::GtkWidget* getEntryWidget(void);

		protected:
			CAbstractSettingView(OpenViBE::Kernel::IBox& rBox, OpenViBE::uint32 ui32Index, OpenViBE::CString &rBuilderName);

			virtual OpenViBE::Kernel::IBox& getBox(void);

			virtual OpenViBE::uint32 getSettingsIndex(void);

			virtual void setSettingWidgetName(const OpenViBE::CString & rWidgetName);

			virtual void setNameWidget(::GtkWidget* pWidget);
			virtual void setEntryWidget(::GtkWidget* pWidget);

			virtual void generateNameWidget(void);
			virtual ::GtkWidget* generateEntryWidget(void);
			virtual void initializeValue();

			virtual void extractWidget(::GtkWidget* pWidget, std::vector< ::GtkWidget * > &rVector);

		private:
			OpenViBE::Kernel::IBox& m_rBox;
			OpenViBE::uint32 m_ui32Index;
			OpenViBE::CString m_sSettingWidgetName;
			::GtkWidget* m_pNameWidget;
			::GtkWidget* m_pEntryWidget;

			//If we don't store the builder, the setting name will be free when we'll unref the builder
			::GtkBuilder *m_pBuilder;
			OpenViBE::boolean m_bOnValueSetting;
		};
	}

}

#endif // OVDCABSTRACTSETTINGVIEW_H
