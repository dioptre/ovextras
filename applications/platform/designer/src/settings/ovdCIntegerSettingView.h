#ifndef OVDCINTEGERSETTINGVIEW_H
#define OVDCINTEGERSETTINGVIEW_H

#include "../ovd_base.h"
#include "ovdCAbstractSettingView.h"

namespace OpenViBEDesigner
{
	namespace Setting
	{
		class CIntegerSettingView : public CAbstractSettingView
		{
		public:
			CIntegerSettingView(OpenViBE::Kernel::IBox& rBox,
								OpenViBE::uint32 ui32Index,
								OpenViBE::CString &rBuilderName,
								const OpenViBE::Kernel::IKernelContext& rKernelContext);

			virtual void getValue(OpenViBE::CString &rValue) const;
			virtual void setValue(const OpenViBE::CString &rValue);

			void adjustValue(int amount);
			void onInsertionCB();


		private:
			::GtkEntry* m_pEntry;

			const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		};
	}

}

#endif // OVDCINTEGERSETTINGVIEW_H
