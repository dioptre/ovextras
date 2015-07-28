#if defined TARGET_OS_Windows

#include "ovasCHeaderEEGO.h"

#ifdef _MSC_VER
///// stroull hack ////
// I copied and modified this snippet from https://github.com/mloskot/soci/blob/6883da2cfef650852183339969a09406d7b80b9c/src/core/soci-platform.h
// It is Boost licensed and should be fine. Code is trivial to recreate anyway, but can introduce errors.
// Older versions of MSVC do not support stroull and this has been one way to solve this problem.
// - Steffen Heimes

// Define if you have the strtoll variants.
#if _MSC_VER >= 1300
# define HAVE_STRTOULL 1
namespace std {
	static inline long long strtoull(char const* str, char** str_end, int base)
	{
		return _strtoui64(str, str_end, base);
	}
}
#else
# undef HAVE_STRTOULL
# error "Visual C++ versions prior 1300 don't support _strtoui64"
#endif // _MSC_VER >= 1300
#endif // _MSC_VER


using namespace OpenViBEAcquisitionServer;

CHeaderEEGO::CHeaderEEGO()
	:m_iEEGRange(1000)
	,m_bEEGRangeSet(false)
	,m_iBIPRange(1500)
	,m_bBIPRangeSet(false)
	,m_sEEGMask("0xFFFFFFFFFFFFFFFF")
	,m_sBIPMask("0xFFFFFF")
	,m_bEEGMaskSet(false)
	,m_bBIPMaskSet(false)
{}

OpenViBE::uint32 CHeaderEEGO::getEEGMaskInt() const
{
	return strtoull(m_sEEGMask, NULL, 0);
}

OpenViBE::uint32 CHeaderEEGO::getBIPMaskInt() const
{
	return strtoull(m_sBIPMask, NULL, 0);
}


/* static */
OpenViBE::uint64 CHeaderEEGO::strtoull(char const* str, char** str_end, int base)
{
	return std::strtoull(str, str_end, base);
}

#endif // TARGET_OS_Windows
