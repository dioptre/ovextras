#if defined TARGET_HAS_ThirdPartyEEGOAPI

#if defined TARGET_OS_Windows

#ifndef __OpenViBE_AcquisitionServer_CHeaderEEGO_H__
#define __OpenViBE_AcquisitionServer_CHeaderEEGO_H__

#include "../ovasCHeader.h"

namespace OpenViBEAcquisitionServer
{
	class CHeaderEEGO : public CHeader
	{
	public:

		CHeaderEEGO();

		// EEG, referential channels
		// range
		OpenViBE::uint32	getEEGRange() const { return m_iEEGRange; };
		void				setEEGRange(OpenViBE::uint32 range) { m_iEEGRange = range; m_bEEGRangeSet = true; };
		OpenViBE::boolean	isEEGRangeSet() const { return m_bEEGRangeSet; };

		// mask
		OpenViBE::CString	getEEGMask() const { return m_sEEGMask; };
		OpenViBE::uint32	getEEGMaskInt() const; // Same as method above. Only string parsing has been done
		void				setEEGMask(const OpenViBE::CString mask) { m_sEEGMask = mask; m_bEEGMaskSet = true; };
		OpenViBE::boolean	isEEGMaskSet() const { return m_bEEGMaskSet; };

		// Bipolar channels
		// range
		OpenViBE::uint32	getBIPRange() const { return m_iBIPRange; };
		OpenViBE::uint32	getBIPMaskInt() const; // Same as method above. Only string parsing has been done
		void				setBIPRange(OpenViBE::uint32 range) { m_iBIPRange = range; m_bBIPRangeSet = true; };
		OpenViBE::boolean	isBIPRangeSet() const { return m_bBIPRangeSet; };

		// mask
		OpenViBE::CString	getBIPMask() const { return m_sBIPMask; };
		void				setBIPMask(const OpenViBE::CString mask) { m_sBIPMask = mask; m_bBIPMaskSet = true; };
		OpenViBE::boolean	isBIPMaskSet() const { return m_bBIPMaskSet; };

	// static helper methods. Reason to put it in here is to provide the service to configuration, header and driver.
	// Not nice, should go to generic utilities or just go when VS10 is gone.
	public:
		static OpenViBE::uint64 strtoull(char const* str, char** str_end, int base);

	protected:
		OpenViBE::uint32	m_iEEGRange;
		OpenViBE::uint32	m_iBIPRange;
		OpenViBE::CString	m_sEEGMask;
		OpenViBE::CString	m_sBIPMask;
		OpenViBE::boolean	m_bEEGRangeSet;
		OpenViBE::boolean	m_bBIPRangeSet;
		OpenViBE::boolean	m_bEEGMaskSet;
		OpenViBE::boolean	m_bBIPMaskSet;
	};
}


#endif // Header Guard
#endif // TARGET_OS_Windows

#endif
