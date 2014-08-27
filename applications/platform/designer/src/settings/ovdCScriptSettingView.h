#ifndef OVDCSCRIPTSETTINGVIEW_H
#define OVDCSCRIPTSETTINGVIEW_H

#include "../ovd_base.h"
#include "ovdCAbstractSettingView.h"

namespace OpenViBEDesigner
{
	namespace Setting
	{
		class CScriptSettingView : public CAbstractSettingView
		{
		public:
			CScriptSettingView(OpenViBE::Kernel::IBox& rBox,
								 OpenViBE::uint32 ui32Index,
								 OpenViBE::CString &rBuilderName,
								 const OpenViBE::Kernel::IKernelContext& rKernelContext);

			virtual void getValue(OpenViBE::CString &rValue) const;
			virtual void setValue(const OpenViBE::CString &rValue);

			void browse();
			void edit();

		private:
			::GtkEntry* m_pEntry;

			const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		};
	}

}

#endif // OVDCSCRIPTSETTINGVIEW_H
