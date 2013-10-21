#include "ovkCMyMessage.h"

using namespace std;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;

// Returned references are invalid after processMessage().
//---------------------------------------------------------------------------------------------------//
//------------------------------------------------- GETTERS -----------------------------------------//
//---------------------------------------------------------------------------------------------------//
uint64 CMyMessage::getValueUint64(const OpenViBE::CString &key, bool &success) const
{
	uint64 l_ui64Value = 0;//default value
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

OpenViBE::float64 CMyMessage::getValueFloat64(const CString &key, bool &success) const
{
	float64 l_f64Value = 0;//default value
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

const OpenViBE::CString* CMyMessage::getValueCString(const CString &key, bool &success) const
{
	const CString* l_sValue = NULL;
	std::map<CString, CString>::const_iterator l_oIterator = m_oStrings.find(key);
	if (l_oIterator!=m_oStrings.end())
	{
		l_sValue = &(l_oIterator->second);
		success = true;
	}
	else
	{
		success = false;
	}
	return l_sValue;
}

const OpenViBE::IMatrix* CMyMessage::getValueCMatrix(const CString &key, bool &success) const
{
	const IMatrix* l_oValue = NULL;
	std::map<CString, CMatrix*>::const_iterator l_oIterator = m_oMatrices.find(key);
	if (l_oIterator!=m_oMatrices.end())
	{
		l_oValue = (l_oIterator->second);
		success = true;
	}
	else
	{
		success = false;
	}
	return l_oValue;
}

//---------------------------------------------------------------------------------------------------//
//------------------------------------------------- SETTERS -----------------------------------------//
//---------------------------------------------------------------------------------------------------//
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
	//copy the ref inside
	m_oStrings[key] = valueIn;
	success = true;
	return success;
}

bool CMyMessage::setValueCMatrix(CString key, const CMatrix &valueIn){
	bool success = false;
	m_oMatrices[key] = new CMatrix();
	// we copy 'manually' since we do not have access to the toolkit functions
	uint32 l_ui32DimensionCount = valueIn.getDimensionCount();
	m_oMatrices[key]->setDimensionCount(l_ui32DimensionCount);
	for (uint32 i=0; i<l_ui32DimensionCount; i++)
	{
		uint32 l_ui32DimensionSize = valueIn.getDimensionSize(i);
		m_oMatrices[key]->setDimensionSize(i,l_ui32DimensionSize);
		for (uint32 j=0; j<l_ui32DimensionSize; j++)
		{
			const char* l_cLabel = valueIn.getDimensionLabel(i,j);
			m_oMatrices[key]->setDimensionLabel(i,j,l_cLabel);
		}
	}

	for (uint32 i=0;i<valueIn.getBufferElementCount();i++)
	{
		m_oMatrices[key]->getBuffer()[i] = valueIn.getBuffer()[i];
	}
	//
	success = true;
	return success;
}

//---------------------------------------------------------------------------------------------------//
//----------------------------------------------KEY GETTERS -----------------------------------------//
//---------------------------------------------------------------------------------------------------//
const OpenViBE::CString* CMyMessage::getFirstCStringToken() const
{
	const CString* l_sToken = NULL;
	if (!m_oStrings.empty())
	{
		l_sToken = &m_oStrings.begin()->first;
	}
	return l_sToken;
}

const OpenViBE::CString* CMyMessage::getFirstUInt64Token() const
{
	const CString* l_sToken = NULL;
	if (!m_oUint64s.empty())
	{
		l_sToken = &m_oUint64s.begin()->first;
	}
	return l_sToken;
}

const OpenViBE::CString* CMyMessage::getFirstFloat64Token() const
{
	const CString* l_sToken = NULL;
	if (!m_oFloat64s.empty())
	{
		l_sToken = &m_oFloat64s.begin()->first;
	}
	return l_sToken;
}

const OpenViBE::CString* CMyMessage::getFirstIMatrixToken() const
{
	const CString* l_sToken = NULL;
	if (!m_oMatrices.empty())
	{
		l_sToken = &m_oMatrices.begin()->first;
	}
	return l_sToken;
}

const OpenViBE::CString* CMyMessage::getNextCStringToken(const OpenViBE::CString &previousToken) const
{
	const CString* l_sKey = NULL;
	std::map<CString, CString >::const_iterator it = m_oStrings.find(previousToken);
	it++;
	if (it!=m_oStrings.end())
	{
		l_sKey = &it->first;
	}
	return l_sKey;
}

const OpenViBE::CString* CMyMessage::getNextUInt64Token(const OpenViBE::CString &previousToken) const
{
	const CString* l_sKey = NULL;
	std::map<CString, uint64>::const_iterator it = m_oUint64s.find(previousToken);
	it++;
	if (it!=m_oUint64s.end())
	{
		l_sKey = &it->first;
	}
	return l_sKey;
}

const OpenViBE::CString* CMyMessage::getNextFloat64Token(const OpenViBE::CString &previousToken) const
{
	const CString* l_sKey = NULL;
	std::map<CString, float64>::const_iterator it = m_oFloat64s.find(previousToken);
	it++;
	if (it!=m_oFloat64s.end())
	{
		l_sKey = &it->first;
	}
	return l_sKey;
}

const OpenViBE::CString* CMyMessage::getNextIMatrixToken(const OpenViBE::CString &previousToken) const
{
	const CString* l_sKey = NULL;
	std::map<CString, CMatrix* >::const_iterator it = m_oMatrices.find(previousToken);
	it++;
	if (it!=m_oMatrices.end())
	{
		l_sKey = &it->first;
	}
	return l_sKey;
}
