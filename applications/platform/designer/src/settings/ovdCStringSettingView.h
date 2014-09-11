#ifndef OVDCSTRINGSETTINGVIEW_H
#define OVDCSTRINGSETTINGVIEW_H

#include "../ovd_base.h"
#include "ovdCAbstractSettingView.h"

namespace OpenViBEDesigner
{
	namespace Setting
	{
		class CStringSettingView : public CAbstractSettingView
		{
		public:
			CStringSettingView(OpenViBE::Kernel::IBox& rBox, OpenViBE::uint32 ui32Index, OpenViBE::CString &rBuilderName);

			virtual void getValue(OpenViBE::CString &rValue) const;
			virtual void setValue(const OpenViBE::CString &rValue);

			void onChange();

		private:
			::GtkEntry* m_pEntry;
			OpenViBE::boolean m_bOnValueSetting;
		};
	}

}

#endif // OVDCSTRINGSETTINGVIEW_H
