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

		virtual const char* getName() const =0;

		//Attribute
		virtual XML::boolean addAttribute(const char* sAttributeName, const char* sAttributeValue)=0;
		virtual XML::boolean hasAttribute(const char* sAttributeName) const =0;
		virtual const char* getAttribute(const char* sAttributeName) const =0;

		//PCDATA
		virtual void setPCData(const char* childData)=0;
		virtual const char* getPCData(void) const =0;

		//Child
		virtual void addChild(XML::IXMLNode* ChildNode)=0;
		virtual XML::IXMLNode* getChild(const XML::uint32 iChildIndex) const =0;
		virtual XML::IXMLNode* getChildByName(const char* sName) const =0;
		virtual XML::uint32 getChildCount(void) const =0;

		//XML generation
		virtual char* getXML(const XML::uint32 depth=0) const =0;

	protected:
		virtual ~IXMLNode(void) {}
	};

	extern OV_API XML::IXMLNode* createNode(const char* sName);

}

#endif // IXMLNODE_H
