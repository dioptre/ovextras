#ifndef OVDCABSTRACTSETTINGVIEW_H
#define OVDCABSTRACTSETTINGVIEW_H

#include "../ovd_base.h"

namespace OpenViBEDesigner
{
	namespace Setting
	{
		class CAbstractSettingView{

		public:
			virtual OpenViBE::CString OpenVigetSettingWidgetName(void);

			virtual void getValue(OpenViBE::CString &rValue) const = 0;
			virtual void setValue(const OpenViBE::CString &rValue) = 0;

		protected:
			CAbstractSettingView(OpenViBE::Kernel::IBox& rBox, OpenViBE::uint32 ui32Index);

			virtual OpenViBE::Kernel::IBox& getBox(void);

			virtual OpenViBE::uint32 getSettingsIndex(void);

			virtual void setSettingWidgetName(OpenViBE::CString & rWidgetName);

		private:
			OpenViBE::Kernel::IBox& m_rBox;
			OpenViBE::uint32 m_ui32Index;
			OpenViBE::CString m_sSettingWidgetName;
		};
	}

}

#endif // OVDCABSTRACTSETTINGVIEW_H
