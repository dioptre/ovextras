#include "IXMLNode.h"

#include <string>
#include <stack>
#include <vector>
#include <map>

namespace XML
{
	class OV_API IXMLNodeImpl: public IXMLNode
	{
	public:
		IXMLNodeImpl(const char* sName);

		virtual std::string getName();

		virtual void release(void);

		//Attribute
		virtual XML::boolean addAttribute(const char* sAttributeName, const char* sAttributeValue);
		virtual XML::boolean hasAttribute(const char* sAttributeName);
		virtual std::string getAttribute(const char* sAttributeName);

		//PCDATA
		virtual void setPCData(const char* childData);
		virtual std::string &getPCData(void);

		//Child
		virtual void addChild(XML::IXMLNode* pChildNode);
		virtual XML::IXMLNode* getChild(XML::uint32 iChildIndex);
		virtual XML::IXMLNode* getChildByName(const char* sName);
		virtual XML::uint32 getChildCount(void);

		//XMl generation
		virtual std::string getXML(XML::uint32 depth=0);

	protected:
		virtual ~IXMLNodeImpl(void);

	private:
		std::string sanitize(std::string& sString);
		void applyIndentation(std::string &sString, XML::uint32 depth);


		std::vector<XML::IXMLNode *> m_oNodeVector;
		std::map<std::string, std::string> m_mAttibuteMap;
		std::string m_sNodeName;
		std::string m_sPCData;
		XML::boolean m_bHasPCData;
	};
}

using namespace std;
using namespace XML;

IXMLNodeImpl::~IXMLNodeImpl(void)
{
}

void IXMLNodeImpl::release(void)
{
	delete this;
}

IXMLNodeImpl::IXMLNodeImpl(const char *sName):
    m_sNodeName(sName)
  ,m_sPCData("")
  ,m_bHasPCData(false)
{

}

string IXMLNodeImpl::getName()
{
	return m_sNodeName;
}

boolean IXMLNodeImpl::addAttribute(const char* sAttributeName, const char* sAttributeValue)
{
    m_mAttibuteMap[sAttributeName] = sAttributeValue;
    return true;
}

XML::boolean IXMLNodeImpl::hasAttribute(const char *sAttributeName)
{
    return m_mAttibuteMap.count(sAttributeName) != 0;
}

string IXMLNodeImpl::getAttribute(const char *sAttributeName)
{
    return m_mAttibuteMap[sAttributeName];
}

void IXMLNodeImpl::setPCData(const char *childData)
{
    m_sPCData = childData;
    m_bHasPCData = true;
}

string &IXMLNodeImpl::getPCData(void)
{
    return m_sPCData;
}

void IXMLNodeImpl::addChild(IXMLNode *pChildNode)
{
    m_oNodeVector.push_back(pChildNode);
}

IXMLNode *IXMLNodeImpl::getChild(XML::uint32 iChildIndex)
{
	return m_oNodeVector[iChildIndex];
}

IXMLNode *IXMLNodeImpl::getChildByName(const char *sName)
{
	string l_sName(sName);
	for (vector<XML::IXMLNode*>::iterator it=m_oNodeVector.begin(); it!=m_oNodeVector.end(); ++it)
	{
		IXMLNode *l_sTempNode = (IXMLNode *)(*it);
		if(!l_sTempNode->getName().compare(l_sName))
			return l_sTempNode;
	}
	return NULL;
}

XML::uint32 IXMLNodeImpl::getChildCount(void)
{
    return m_oNodeVector.size();
}

std::string IXMLNodeImpl::sanitize(string &sString)
{
    string::size_type i;
    string l_sRes(sString);
    if(l_sRes.length()!=0)
    {
        // mandatory, this one should be the first because the other ones add & symbols
        for(i=l_sRes.find("&", 0); i!=string::npos; i=l_sRes.find("&", i+1))
            l_sRes.replace(i, 1, "&amp;");
        // other escape sequences
        for(i=l_sRes.find("\"", 0); i!=string::npos; i=l_sRes.find("\"", i+1))
            l_sRes.replace(i, 1, "&quot;");
        for(i=l_sRes.find("<", 0); i!=string::npos; i=l_sRes.find("<", i+1))
            l_sRes.replace(i, 1, "&lt;");
        for(i=l_sRes.find(">", 0); i!=string::npos; i=l_sRes.find(">", i+1))
            l_sRes.replace(i, 1, "&gt;");
    }
    return l_sRes;
}

void IXMLNodeImpl::applyIndentation(string &sString, XML::uint32 depth)
{
    string l_sIndent(depth, '\t');
    sString.append(l_sIndent);
}

string IXMLNodeImpl::getXML(XML::uint32 depth)
{
    string l_sRes;
    applyIndentation(l_sRes, depth);
    l_sRes = l_sRes + "<" + m_sNodeName;

    //Add attributes if we have some
    if(!m_mAttibuteMap.empty())
    {
        for (map<string,string>::iterator it=m_mAttibuteMap.begin(); it!=m_mAttibuteMap.end(); ++it)
        {

            l_sRes = l_sRes + string(" ")+ it->first + string("=\"") + sanitize(it->second) + string("\"");
        }
    }
    //If we have nothing else to print let's close the node and return
    if(!m_bHasPCData && m_oNodeVector.empty())
    {
        l_sRes = l_sRes + string("/>\n");
        return l_sRes;
    }

	l_sRes = l_sRes + string(">");

    if(m_bHasPCData)
    {
		l_sRes = l_sRes + sanitize(m_sPCData);
    }

    for (vector<XML::IXMLNode*>::iterator it=m_oNodeVector.begin(); it!=m_oNodeVector.end(); ++it)
    {
        IXMLNode *l_sTempNode = (IXMLNode *)(*it);
		l_sRes = l_sRes + string("\n") + l_sTempNode->getXML(depth+1);
    }

	if(!m_oNodeVector.empty())
		applyIndentation(l_sRes, depth);
    l_sRes = l_sRes + "</" + m_sNodeName + ">\n";
    return l_sRes;
}

OV_API IXMLNode* XML::createNode(const char* sName)
{
    return new IXMLNodeImpl(sName);
}


