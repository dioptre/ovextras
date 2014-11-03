#ifndef __OpenViBE_Designer_Setting_CSettingViewFactory_H__
#define __OpenViBE_Designer_Setting_CSettingViewFactory_H__

#include "../ovd_base.h"
#include "ovdCAbstractSettingView.h"

namespace OpenViBEDesigner
{
	namespace Setting
	{
		class CSettingViewFactory{
		public:
			CSettingViewFactory(const OpenViBE::CString & rBuilderName,
								const OpenViBE::Kernel::IKernelContext& rKernelContext);

			CAbstractSettingView* getSettingView(OpenViBE::Kernel::IBox &rBox,
												 OpenViBE::uint32 ui32Index);

			CAbstractSettingView* getSettingView(OpenViBE::CIdentifier &rIdentifierType);

		private:
			::GtkBuilder* m_pBuilder;
			OpenViBE::CString m_sBuilderName;
			const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		};
	}

}

#endif // __OpenViBE_Designer_Setting_CSettingViewFactory_H__
