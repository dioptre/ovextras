
#if defined(TARGET_HAS_ThirdPartyEEGOAPI)

#if defined TARGET_OS_Windows

#ifndef __OpenViBE_AcquisitionServer_CDriverEEGO_H__
#define __OpenViBE_AcquisitionServer_CDriverEEGO_H__

#include "ovasIDriver.h"
#include "ovasCHeaderEEGO.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

// forward declarations
namespace eemagine
{
	namespace sdk
	{
		class amplifier;
		class stream;
	}
}



namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDriverEEGO
	 * \author Steffen Heimes (eemagine GmbH)
	 * \date Mon Oct 20 14:40:33 2014
	 * \brief The CDriverEEGO allows the acquisition server to acquire data from an EEGO device.
	 *
	 * \sa CConfigurationEEGO
	 */
	class CDriverEEGO : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverEEGO(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
		virtual ~CDriverEEGO(void);
		virtual const char* getName(void);

		virtual OpenViBE::boolean initialize(
			const OpenViBE::uint32 ui32SampleCountPerSentBlock,
			OpenViBEAcquisitionServer::IDriverCallback& rCallback);
		virtual OpenViBE::boolean uninitialize(void);

		virtual OpenViBE::boolean start(void);
		virtual OpenViBE::boolean stop(void);
		virtual OpenViBE::boolean loop(void);

		virtual OpenViBE::boolean isConfigurable(void);
		virtual OpenViBE::boolean configure(void);
		virtual const OpenViBEAcquisitionServer::IHeader* getHeader(void) { return &m_oHeader; }
		
		virtual OpenViBE::boolean isFlagSet(
			const OpenViBEAcquisitionServer::EDriverFlag eFlag) const
		{
			return eFlag==DriverFlag_IsUnstable;
		}

	private:

		/**
		 * Check if the configuration makes sense and tries to fix it, informing the user.
		 */
		OpenViBE::boolean check_configuration(void);
		OpenViBE::uint64 getRefChannelMask() const;
		OpenViBE::uint64 getBipChannelMask() const;

	protected:

		SettingsHelper								m_oSettings;
		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;
		OpenViBEAcquisitionServer::CHeaderEEGO		m_oHeader;

		OpenViBE::uint32	m_ui32SampleCountPerSentBlock;
		OpenViBE::float32*	m_pSample;

		eemagine::sdk::amplifier* m_pAmplifier;
		eemagine::sdk::stream*	  m_pStream;

	private:

		OpenViBE::uint32			m_ui32SamplesInBuffer;
		OpenViBE::uint32			m_i32TriggerChannel;
		OpenViBE::CStimulationSet	m_oStimulationSet; // Storing the samples over time
		OpenViBE::uint32			m_ui32LastTriggerValue; // To detect flanks in the trigger signal. The last state on the trigger input.

		// For setting store/load
		OpenViBE::uint32	m_iBIPRange; // [mV]
		OpenViBE::uint32	m_iEEGRange; // [mV]
		OpenViBE::CString	m_sEEGMask; // String interpreted as value to be interpreted as bitfield
		OpenViBE::CString	m_sBIPMask; // String interpreted as value to be interpreted as bitfield
	};
};

#endif // __OpenViBE_AcquisitionServer_CDriverEEGO_H__
#endif // TARGET_OS_Windows

#endif
