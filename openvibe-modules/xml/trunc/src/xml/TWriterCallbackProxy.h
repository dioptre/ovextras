#ifndef __XML_TWriterCallbackProxy_H__
#define __XML_TWriterCallbackProxy_H__

#include "IWriter.h"

namespace XML
{

// ________________________________________________________________________________________________________________
//

	template <class COwnerClass>
	class XML_API TWriterCallbackProxy1 : virtual public XML::IWriterCallback
	{
	public:
		TWriterCallbackProxy1(
			COwnerClass& rOwnerObject,
			void (COwnerClass::*mfpWrite)(const char* sString))
			:m_rOwnerObject(rOwnerObject)
			,m_mfpWrite(mfpWrite)
		{
		}
		virtual void write(const char* sString)=0;
		{
			if(m_mfpWrite)
			{
				m_rOwnerObject.m_mfpWrite(sString);
			}
		}
	protected:
		COwnerClass& m_rOwnerObject;
		void (COwnerClass::*m_mfpWrite)(const char* sString);
	};

// ________________________________________________________________________________________________________________
//

	template <class COwnerClass, void (COwnerClass::*mfpWrite)(const char* sString)>
	class XML_API TWriterCallbackProxy2 : virtual public XML::IWriterCallback
	{
	public:
		TWriterCallbackProxy2(COwnerClass rOwnerObject)
			:m_rOwnerObject(rOwnerObject)
			,m_mfpWrite(mfpWrite)
		{
		}
		virtual void write(const char* sString)=0;
		{
			if(mfpWrite)
			{
				m_rOwnerObject.mfpWrite(sString);
			}
		}
	protected:
		COwnerClass& m_rOwnerObject;
		void (COwnerClass::*m_mfpWrite)(const char* sString);
	};

// ________________________________________________________________________________________________________________
//

};

#endif // __XML_TWriterCallbackProxy_H__
