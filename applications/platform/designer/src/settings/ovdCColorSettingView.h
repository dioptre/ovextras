#ifndef OVDCCOLORSETTINGVIEW_H
#define OVDCCOLORSETTINGVIEW_H

#include "../ovd_base.h"
#include "ovdCAbstractSettingView.h"

namespace OpenViBEDesigner
{
	namespace Setting
	{
		class CColorSettingView : public CAbstractSettingView
		{
		public:
			CColorSettingView(OpenViBE::Kernel::IBox& rBox,
								OpenViBE::uint32 ui32Index,
								OpenViBE::CString &rBuilderName,
								const OpenViBE::Kernel::IKernelContext& rKernelContext);

			virtual void getValue(OpenViBE::CString &rValue) const;
			virtual void setValue(const OpenViBE::CString &rValue);

			void selectColor();
			void onChange();

		private:
			::GtkEntry* m_pEntry;
			::GtkColorButton *m_pButton;

			const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		};
	}

}

#endif // OVDCCOLORETTINGVIEW_H
