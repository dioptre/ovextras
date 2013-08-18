#ifndef __OpenViBEKernel_Kernel_Player_CMyMessage_H__
#define __OpenViBEKernel_Kernel_Player_CMyMessage_H__

#include "../ovkTKernelObject.h"
#include "ovkTMessage.h"

#include <map>
#include <iostream>

#include <openvibe/ov_all.h>

namespace OpenViBE
{
	namespace Kernel
	{
        class CMyMessage : public OpenViBE::Kernel::IMyMessage
		{
		public:

            CMyMessage()
			{
			}

            // Returned references are invalid after processMessage().

                OpenViBE::uint64 getValueUint64(OpenViBE::CString &key, bool &success) const;
                OpenViBE::float64 getValueFloat64(CString &key, bool &success) const;
                const OpenViBE::CString* getValueCString(CString &key, bool &success) const;

                const OpenViBE::IMatrix* getValueCMatrix(CString &key, bool &success) const;

                bool setValueUint64(CString key, uint64 valueIn);
                bool setValueFloat64(CString key, float64 valueIn);
                bool setValueCString(CString key, const CString &valueIn);
                bool setValueCMatrix(CString key, const CMatrix &valueIn);
                //*/



        private:
            // Data definitions
                std::map<CString, uint64>			m_oUint64s;
                std::map<CString, const CString* >		m_oStrings;
                std::map<CString, float64>			m_oFloat64s;
                std::map<CString, const CMatrix* >	      m_oMatrices;

            _IsDerivedFromClass_Final_(OpenViBE::Kernel::IMyMessage, OVK_ClassId_Kernel_Player_MyMessage);
		};
	};
};

#endif // __OpenViBEKernel_Kernel_Player_CMyMessage_H__
