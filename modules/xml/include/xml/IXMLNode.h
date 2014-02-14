#ifndef __XML_IXMLNODE_H_
#define __XML_IXMLNODE_H_

#include "defines.h"
#include <string>

namespace XML
{
	class OV_API IXMLNode
	{
	public:
		virtual void release(void)=0;

		virtual std::string getName()=0;

		//Attribute
		virtual XML::boolean addAttribute(const char* sAttributeName, const char* sAttributeValue)=0;
		virtual XML::boolean hasAttribute(const char* sAttributeName)=0;
		virtual std::string getAttribute(const char* sAttributeName)=0;

		//PCDATA
		virtual void setPCData(const char* childData)=0;
		virtual std::string &getPCData(void)=0;

		//Child
		virtual void addChild(XML::IXMLNode* ChildNode)=0;
		virtual XML::IXMLNode* getChild(XML::uint32 iChildIndex)=0;
		virtual XML::IXMLNode* getChildByName(const char* sName)=0;
		virtual XML::uint32 getChildCount(void)=0;

		//XML generation
		virtual std::string getXML(XML::uint32 depth=0)=0;

	protected:
		virtual ~IXMLNode(void) {}
	};

	extern OV_API XML::IXMLNode* createNode(const char* sName);

}

#endif // IXMLNODE_H
