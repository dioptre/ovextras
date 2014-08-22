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
			static CAbstractSettingView* getSettingView(OpenViBE::Kernel::IBox &rBox,
														OpenViBE::uint32 ui32Index,
														OpenViBE::Kernel::IKernelContext& kernelContext);
		};
	}

}

#endif // OVDCSETTINGVIEWFACTORY_H
