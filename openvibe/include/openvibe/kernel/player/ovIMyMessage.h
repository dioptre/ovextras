#ifndef __OpenViBE_Kernel_Player_IMyMessage_H__
#define __OpenViBE_Kernel_Player_IMyMessage_H__

#include "../ovIKernelObject.h"

namespace OpenViBE
{
	namespace Kernel
	{

        class OV_API IMyMessage : public OpenViBE::Kernel::IKernelObject
		{
        public:
            // Returned references are invalid after processMessage().
                OpenViBE::uint64 getValueUint64(OpenViBE::CString &key, bool &success) const;
                OpenViBE::float64 getValueFloat64(CString &key, bool &success) const;
                const OpenViBE::CString* getValueCString(CString &key, bool &success) const;
                const OpenViBE::IMatrix* getValueCMatrix(CString &key, bool &success) const;
                bool setValueUint64(CString key, uint64 valueIn);
                bool setValueFloat64(CString key, float64 valueIn);
                bool setValueCString(CString key, const CString &valueIn);
                bool setValueCMatrix(CString key, const CMatrix &valueIn);

            _IsDerivedFromClass_(OpenViBE::Kernel::IKernelObject, OV_ClassId_Kernel_Player_MyMessage)

		};
	};
};

#endif // __OpenViBE_Kernel_Player_IMyMessage_H__
