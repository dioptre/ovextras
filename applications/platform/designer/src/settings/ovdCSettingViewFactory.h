#ifndef OVDCSETTINGVIEWFACTORY_H
#define OVDCSETTINGVIEWFACTORY_H

#include "../ovd_base.h"
#include "ovdCAbstractSettingView.h"

namespace OpenViBEDesigner
{
	namespace Setting
	{
		class CSettingViewFactory{
		public:
			CSettingViewFactory(const OpenViBE::CString & rBuilderName);

			CAbstractSettingView* getSettingView(OpenViBE::Kernel::IBox &rBox,
												 OpenViBE::uint32 ui32Index,
												 const OpenViBE::Kernel::IKernelContext& kernelContext);

		private:
			::GtkBuilder* m_pBuilder;
			OpenViBE::CString m_sBuilderName;
		};
	}

}

#endif // OVDCSETTINGVIEWFACTORY_H
