#ifndef __OpenViBE_AcquisitionServer_CHeaderEEGO_H__
#define __OpenViBE_AcquisitionServer_CHeaderEEGO_H__

#if defined TARGET_HAS_ThirdPartyEEGOAPI

#include "../ovasCHeader.h"

namespace OpenViBEAcquisitionServer
{
	class CHeaderEEGO : public CHeader
	{
	public:

		CHeaderEEGO();

		// EEG, referential channels
		// range
		OpenViBE::uint32 getEEGRange() const;

		void setEEGRange(OpenViBE::uint32 range);
		OpenViBE::boolean isEEGRangeSet() const;

		// mask
		OpenViBE::CString getEEGMask() const;
		OpenViBE::uint64 getEEGMaskInt() const; // Same as method above. Only string parsing has been done
		void setEEGMask(const OpenViBE::CString mask);
		OpenViBE::boolean isEEGMaskSet() const;

		// Bipolar channels
		// range
		OpenViBE::uint32 getBIPRange() const;
		OpenViBE::uint64 getBIPMaskInt() const; // Same as method above. Only string parsing has been done
		void setBIPRange(OpenViBE::uint32 range);
		OpenViBE::boolean isBIPRangeSet() const;

		// mask
		OpenViBE::CString getBIPMask() const;
		void setBIPMask(const OpenViBE::CString mask);
		OpenViBE::boolean isBIPMaskSet() const;

	public:
		// Converts a string representing a number to this number as unsigned 64 bit value.
		// Accepts 0x, 0b and 0 notation for hexadecimal, binary and octal notation.
		// Otherwise it is interpreted as decimal.
		// Returns true if the conversion was successfull, false on error.
		// Please note that the error checking goes beyond the parsing  strtoull etc.:
		// The strto* methods stop parsing at the first character which could not be interpreted.
		// Here the string is checked against all invalid chars and an error will be returned.
		static OpenViBE::boolean convertMask(char const* str, OpenViBE::uint64& r_oOutValue);

		// data
	protected:
		OpenViBE::uint32 m_iEEGRange;
		OpenViBE::uint32 m_iBIPRange;
		OpenViBE::CString m_sEEGMask;
		OpenViBE::CString m_sBIPMask;
		OpenViBE::boolean m_bEEGRangeSet;
		OpenViBE::boolean m_bBIPRangeSet;
		OpenViBE::boolean m_bEEGMaskSet;
		OpenViBE::boolean m_bBIPMaskSet;
	};
}

#endif

#endif // Header Guard
