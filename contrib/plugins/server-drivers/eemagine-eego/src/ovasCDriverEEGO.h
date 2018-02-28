#if defined(TARGET_HAS_ThirdPartyEEGOAPI)

#ifndef __OpenViBE_AcquisitionServer_CDriverEEGO_H__
#define __OpenViBE_AcquisitionServer_CDriverEEGO_H__

#include <memory>

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
		class factory;
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
	class CDriverEEGO : public IDriver
	{
	public:

		CDriverEEGO(IDriverContext& rDriverContext);
		virtual ~CDriverEEGO(void);
		const char* getName(void) override;

		OpenViBE::boolean initialize(
			const OpenViBE::uint32 ui32SampleCountPerSentBlock,
			IDriverCallback& rCallback) override;
		OpenViBE::boolean uninitialize(void) override;

		OpenViBE::boolean start(void) override;
		OpenViBE::boolean stop(void) override;
		OpenViBE::boolean loop(void) override;

		OpenViBE::boolean isConfigurable(void) override;
		OpenViBE::boolean configure(void) override;
		const IHeader* getHeader(void) override { return &m_oHeader; }

		OpenViBE::boolean isFlagSet(const EDriverFlag eFlag) const override
		{
			return false; // The only currently used flag is for checking for unstability. eego is stable now.
		}

	private:


		OpenViBE::boolean loop_wrapped(void);

		/**
		 * Check if the configuration makes sense and tries to fix it, informing the user.
		 */
		OpenViBE::boolean check_configuration(void);
		OpenViBE::uint64 getRefChannelMask() const;
		OpenViBE::uint64 getBipChannelMask() const;
		eemagine::sdk::factory& factory();

	protected:

		SettingsHelper m_oSettings;
		IDriverCallback* m_pCallback;
		CHeaderEEGO m_oHeader;

		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;
		std::unique_ptr<OpenViBE::float32[]> m_pSample;

		std::unique_ptr<eemagine::sdk::factory> m_pFactory;
		std::unique_ptr<eemagine::sdk::amplifier> m_pAmplifier;
		std::unique_ptr<eemagine::sdk::stream> m_pStream;

	private:

		OpenViBE::uint32 m_ui32SamplesInBuffer;
		OpenViBE::uint32 m_i32TriggerChannel;
		OpenViBE::CStimulationSet m_oStimulationSet; // Storing the samples over time

		// To detect flanks in the trigger signal. The last state on the trigger input.
		OpenViBE::uint32 m_ui32LastTriggerValue;

		// For setting store/load
		OpenViBE::uint32 m_iBIPRange; // [mV]
		OpenViBE::uint32 m_iEEGRange; // [mV]
		OpenViBE::CString m_sEEGMask; // String interpreted as value to be interpreted as bitfield
		OpenViBE::CString m_sBIPMask; // String interpreted as value to be interpreted as bitfield
	};
};

#endif // __OpenViBE_AcquisitionServer_CDriverEEGO_H__

#endif
