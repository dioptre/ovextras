#ifndef __OpenViBEKernel_Kernel_Player_CMyMessage_H__
#define __OpenViBEKernel_Kernel_Player_CMyMessage_H__

#include "../ovkTKernelObject.h"
#include "ovkTMessage.h"

#include <map>
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

			~CMyMessage()
			{
				m_oUint64s.clear();
				m_oStrings.clear();
				m_oFloat64s.clear();
				m_oMatrices.clear();
			}
			// Returned references are invalid after processMessage().
			OpenViBE::uint64 getValueUint64(const OpenViBE::CString &key, bool &success) const;
			OpenViBE::float64 getValueFloat64(const CString &key, bool &success) const;
			const OpenViBE::CString* getValueCString(const CString &key, bool &success) const;
			const OpenViBE::IMatrix* getValueCMatrix(const CString &key, bool &success) const;
			//setters
			bool setValueUint64(CString key, uint64 valueIn);
			bool setValueFloat64(CString key, float64 valueIn);
			bool setValueCString(CString key, const CString &valueIn);
			bool setValueCMatrix(CString key, const CMatrix &valueIn);
			//get key
			virtual const OpenViBE::CString* getFirstCStringToken() const;
			virtual const OpenViBE::CString* getFirstUInt64Token() const;
			virtual const OpenViBE::CString* getFirstFloat64Token() const;
			virtual const OpenViBE::CString* getFirstIMatrixToken() const;

			virtual const OpenViBE::CString* getNextCStringToken(const OpenViBE::CString &previousToken) const;
			virtual const OpenViBE::CString* getNextUInt64Token(const OpenViBE::CString &previousToken) const;
			virtual const OpenViBE::CString* getNextFloat64Token(const OpenViBE::CString &previousToken) const;
			virtual const OpenViBE::CString* getNextIMatrixToken(const OpenViBE::CString &previousToken) const;

		private:
			// Data definitions
			std::map<CString, uint64> m_oUint64s;
			std::map<CString, CString > m_oStrings;
			std::map<CString, float64> m_oFloat64s;
			std::map<CString, CMatrix* > m_oMatrices;

			_IsDerivedFromClass_Final_(OpenViBE::Kernel::IMyMessage, OVK_ClassId_Kernel_Player_MyMessage);
		};
	};
};

#endif // __OpenViBEKernel_Kernel_Player_CMyMessage_H__
