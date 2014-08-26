#ifndef OVDCBOOLEANSETTINGVIEW_H
#define OVDCBOOLEANSETTINGVIEW_H

#include "../ovd_base.h"
#include "ovdCAbstractSettingView.h"

namespace OpenViBEDesigner
{
	namespace Setting
	{
		class CBooleanSettingView : public CAbstractSettingView
		{
		public:
			CBooleanSettingView(OpenViBE::Kernel::IBox& rBox, OpenViBE::uint32 ui32Index, OpenViBE::CString &rBuilderName);

			virtual void getValue(OpenViBE::CString &rValue) const;
			virtual void setValue(const OpenViBE::CString &rValue);

			void toggleButtonClick();


		private:
			::GtkTable* m_pTable;
			::GtkToggleButton* m_pToggle;
			::GtkEntry* m_pEntry;
		};
	}

}


#endif // OVDCBOOLEANSETTINGVIEW_H
