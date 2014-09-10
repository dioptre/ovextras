#ifndef OVDCBITMASKSETTINGVIEW_H
#define OVDCBITMASKSETTINGVIEW_H

#include "../ovd_base.h"
#include "ovdCAbstractSettingView.h"

#include <vector>

namespace OpenViBEDesigner
{
	namespace Setting
	{
		class CBitMaskSettingView : public CAbstractSettingView
		{
		public:
			CBitMaskSettingView(OpenViBE::Kernel::IBox& rBox,
								OpenViBE::uint32 ui32Index,
								OpenViBE::CString &rBuilderName,
								const OpenViBE::Kernel::IKernelContext& rKernelContext,
								const OpenViBE::CIdentifier &rTypeIdentifier);

			virtual void getValue(OpenViBE::CString &rValue) const;
			virtual void setValue(const OpenViBE::CString &rValue);

			void onChange();

		private:
			OpenViBE::CIdentifier m_oTypeIdentifier;
			const OpenViBE::Kernel::IKernelContext& m_rKernelContext;

			OpenViBE::boolean m_bOnValueSetting;


			std::vector < ::GtkToggleButton *> m_vToggleButton;
		};
	}

}

#endif // OVDCBITMASKSETTINGVIEW_H
