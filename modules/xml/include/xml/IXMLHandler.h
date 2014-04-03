#ifndef __XML_IXMLHANDLER_H_
#define __XML_IXMLHANDLER_H_

#include "defines.h"
#include "IXMLNode.h"

namespace XML
{
	class OV_API IXMLHandler
	{
	public:
		virtual void release(void)=0;

		//Parsing
		virtual XML::IXMLNode* parseFile(const char* sPath)=0;
		virtual XML::IXMLNode* parseString(const char* sString, const uint32& uiSize)=0;

		//XML extraction
		virtual XML::boolean writeXMLInFile(IXMLNode &rNode, const char* sPath)=0;

	protected:
		virtual ~IXMLHandler() { }
	};

	extern OV_API XML::IXMLHandler* createXMLHandler(void);
}

#endif // __XML_IXMLHANDLER_H_
