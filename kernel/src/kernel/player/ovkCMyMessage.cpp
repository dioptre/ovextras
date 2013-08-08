

#include "ovkCMyMessage.h"


using namespace std;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;

//*
// Returned references are invalid after processMessage().
uint64 CMyMessage::getValueUint64(OpenViBE::CString &key, bool &success) const
{
    uint64 l_ui64Value;
    std::map<CString, uint64>::const_iterator l_oIterator = m_oUint64s.find(key);
    if (l_oIterator!=m_oUint64s.end())
    {
        l_ui64Value = l_oIterator->second;
        success = true;
    }
    else
    {
        success = false;
    }
    return l_ui64Value;
}

OpenViBE::float64 CMyMessage::getValueFloat64(CString &key, bool &success) const
{
    float64 l_f64Value;
    std::map<CString, float64>::const_iterator l_oIterator = m_oFloat64s.find(key);
    if (l_oIterator!=m_oFloat64s.end())
    {
        l_f64Value = l_oIterator->second;
        success = true;
    }
    else
    {
        success = false;
    }
    return l_f64Value;
}

const OpenViBE::CString* CMyMessage::getValueCString(CString &key, bool &success) const
{
    const CString* l_sValue = NULL;
    std::map<CString, const CString*>::const_iterator l_oIterator = m_oStrings.find(key);
    if (l_oIterator!=m_oStrings.end())
    {
        l_sValue = l_oIterator->second;
        success = true;
    }
    else
    {
        success = false;
    }
    return l_sValue;
}
//*/
const OpenViBE::IMatrix* CMyMessage::getValueCMatrix(CString &key, bool &success) const
{
    const IMatrix* l_oValue = NULL;
    std::map<CString, const CMatrix*>::const_iterator l_oIterator = m_oMatrices.find(key);
    if (l_oIterator!=m_oMatrices.end())
    {
        l_oValue = l_oIterator->second;
        success = true;
    }
    else
    {
        success = false;
    }
    return l_oValue;
}


bool CMyMessage::setValueUint64(CString key, uint64 valueIn)
{
    bool success = false;
    m_oUint64s[key] = valueIn;
    success = true;
    return success;
}

bool CMyMessage::setValueFloat64(CString key, float64 valueIn){
    bool success = false;
    m_oFloat64s[key] = valueIn;
    success = true;
    return success;
}

bool CMyMessage::setValueCString(CString key, const CString &valueIn){
    bool success = false;
    m_oStrings[key] = &valueIn;
    success = true;
    return success;
}

bool CMyMessage::setValueCMatrix(CString key, const CMatrix &valueIn){
    bool success = false;
    m_oMatrices[key] = &valueIn;
    success = true;
    return success;
}
//*/

