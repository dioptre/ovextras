#ifndef OVDCENUMERATIONSETTING_H
#define OVDCENUMERATIONSETTING_H

#include "../ovd_base.h"
#include "ovdCAbstractSettingView.h"

namespace OpenViBEDesigner
{
	namespace Setting
	{
		class CEnumerationSettingView : public CAbstractSettingView
		{
		public:
			CEnumerationSettingView(OpenViBE::Kernel::IBox& rBox,
								OpenViBE::uint32 ui32Index,
								OpenViBE::CString &rBuilderName,
								const OpenViBE::Kernel::IKernelContext& rKernelContext,
									const OpenViBE::CIdentifier &rTypeIdentifier);

			virtual void getValue(OpenViBE::CString &rValue) const;
			virtual void setValue(const OpenViBE::CString &rValue);


		private:
			::GtkComboBox* m_pComboBox;
			OpenViBE::CIdentifier m_oTypeIdentifier;

			const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		};
	}

}

#endif // OVDCENUMERATIONSETTING_H
