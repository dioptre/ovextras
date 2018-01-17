#if defined TARGET_HAS_ThirdPartyEEGOAPI

#include <algorithm>
#include <locale>
#include <boost/algorithm/string.hpp>

#include "ovasCHeaderEEGO.h"

using namespace OpenViBEAcquisitionServer;

CHeaderEEGO::CHeaderEEGO()
	: m_iEEGRange(1000)
	, m_iBIPRange(1500)
	, m_sEEGMask("0xFFFFFFFFFFFFFFFF")
	, m_sBIPMask("0xFFFFFF")
	, m_bEEGRangeSet(false)
	, m_bBIPRangeSet(false)
	, m_bEEGMaskSet(false)
	, m_bBIPMaskSet(false)
{
}

OpenViBE::uint32 CHeaderEEGO::getEEGRange() const
{
	return m_iEEGRange;
}

void CHeaderEEGO::setEEGRange(OpenViBE::uint32 range)
{
	m_iEEGRange = range;
	m_bEEGRangeSet = true;
}

OpenViBE::boolean CHeaderEEGO::isEEGRangeSet() const
{
	return m_bEEGRangeSet;
}

OpenViBE::CString CHeaderEEGO::getEEGMask() const
{
	return m_sEEGMask;
}

OpenViBE::uint64 CHeaderEEGO::getEEGMaskInt() const
{
	return strtoull(m_sEEGMask, nullptr, 0);
}

void CHeaderEEGO::setEEGMask(const OpenViBE::CString mask)
{
	m_sEEGMask = mask;
	m_bEEGMaskSet = true;
}

OpenViBE::boolean CHeaderEEGO::isEEGMaskSet() const
{
	return m_bEEGMaskSet;
}

OpenViBE::uint32 CHeaderEEGO::getBIPRange() const
{
	return m_iBIPRange;
}

OpenViBE::uint64 CHeaderEEGO::getBIPMaskInt() const
{
	return strtoull(m_sBIPMask, nullptr, 0);
}

void CHeaderEEGO::setBIPRange(OpenViBE::uint32 range)
{
	m_iBIPRange = range;
	m_bBIPRangeSet = true;
}

OpenViBE::boolean CHeaderEEGO::isBIPRangeSet() const
{
	return m_bBIPRangeSet;
}

OpenViBE::CString CHeaderEEGO::getBIPMask() const
{
	return m_sBIPMask;
}

void CHeaderEEGO::setBIPMask(const OpenViBE::CString mask)
{
	m_sBIPMask = mask;
	m_bBIPMaskSet = true;
}

OpenViBE::boolean CHeaderEEGO::isBIPMaskSet() const
{
	return m_bBIPMaskSet;
}

/* static */
OpenViBE::boolean CHeaderEEGO::convertMask(char const* str, OpenViBE::uint64& r_oOutValue)
{
	OpenViBE::boolean l_bParseError = false;

	// init r_outValue anyway
	r_oOutValue = 0;

	std::string l_oString(str); //easier substring handling etc. Minor performance penalty which should not matter.
	boost::algorithm::trim(l_oString); // Make sure to handle whitespace correctly

	// check prefixes
	if (boost::algorithm::istarts_with(l_oString, "0b"))
	{
		// binary
		const auto substring = l_oString.substr(2);

		// check for valid string members
		if (!std::all_of(substring.begin(), substring.end(), [&](const char& chr) { return chr == '0' || chr == '1'; }))
		{
			l_bParseError = true;
		}
		else
		{
			// use the substring for string to number conversion as base 2
			r_oOutValue = strtoull(substring.c_str(), nullptr, 2);
		}
	}
	else if (boost::algorithm::istarts_with(l_oString, "0x"))
	{
		// hex
		const auto substring = l_oString.substr(2);
		std::locale loc;

		if (!std::all_of(substring.begin(), substring.end(), [&](const char& chr) { return std::isxdigit(chr, loc); }))
		{
			l_bParseError = true;
		}
		else
		{
			r_oOutValue = strtoull(substring.c_str(), nullptr, 16);
		}
	}
	else if (boost::algorithm::istarts_with(l_oString, "0"))
	{
		// octal
		const auto substring = l_oString.substr(1);
		std::locale loc;

		if (!std::all_of(substring.begin(), substring.end(), [&](const char& chr) { return chr >= '0' && chr < '8'; }))
		{
			l_bParseError = true;
		}
		else
		{
			r_oOutValue = strtoull(substring.c_str(), nullptr, 8);
		}
	}
	else
	{
		// decimal
		const auto substring = l_oString;
		std::locale loc;

		if (!std::all_of(substring.begin(), substring.end(), [&](const char& chr) { return std::isdigit(chr, loc); }))
		{
			l_bParseError = true;
		}
		else
		{
			r_oOutValue = strtoull(substring.c_str(), nullptr, 10);
		}
	}

	// if no special handling for the base 2 case is neccessary we can just use the std::stroull implementation and do not mess with that any further.
	return !l_bParseError;
}


#endif
