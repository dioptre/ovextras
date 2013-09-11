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
				virtual OpenViBE::uint64 getValueUint64(const OpenViBE::CString &key, bool &success) const=0;
				virtual OpenViBE::float64 getValueFloat64(const CString &key, bool &success) const=0;
				virtual const OpenViBE::CString* getValueCString(const CString &key, bool &success) const=0;
				virtual const OpenViBE::IMatrix* getValueCMatrix(const CString &key, bool &success) const=0;
				virtual bool setValueUint64(CString key, uint64 valueIn)=0;
				virtual bool setValueFloat64(CString key, float64 valueIn)=0;
				virtual bool setValueCString(CString key, const CString &valueIn)=0;
				virtual bool setValueCMatrix(CString key, const CMatrix &valueIn)=0;
				//get key
				virtual const OpenViBE::CString* getFirstCStringToken() const=0;
				virtual const OpenViBE::CString* getFirstUInt64Token() const=0;
				virtual const OpenViBE::CString* getFirstFloat64Token() const=0;
				virtual const OpenViBE::CString* getFirstIMatrixToken() const=0;

				virtual const OpenViBE::CString* getNextCStringToken(const OpenViBE::CString &previousToken) const=0;
				virtual const OpenViBE::CString* getNextUInt64Token(const OpenViBE::CString &previousToken) const=0;
				virtual const OpenViBE::CString* getNextFloat64Token(const OpenViBE::CString &previousToken) const=0;
				virtual const OpenViBE::CString* getNextIMatrixToken(const OpenViBE::CString &previousToken) const=0;

			_IsDerivedFromClass_(OpenViBE::Kernel::IKernelObject, OV_ClassId_Kernel_Player_MyMessage)

		};
	};
};

#endif // __OpenViBE_Kernel_Player_IMyMessage_H__
