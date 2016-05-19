#ifndef __OpenViBE_AcquisitionServer_CDrivergNautilusInterface_H__
#define __OpenViBE_AcquisitionServer_CDrivergNautilusInterface_H__

#if defined(TARGET_HAS_ThirdPartyGNEEDaccessAPI)

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <GDSClientAPI.h>
#include <GDSClientAPI_gNautilus.h>
#include <string>
#include <algorithm>
#include <Windows.h>

using namespace std;

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDrivergNautilusInterface
	 * \author g.tec medical engineering GmbH (g.tec medical engineering GmbH)
	 * \date Wed Aug 12 16:37:18 2015
	 * \brief The CDrivergNautilusInterface allows the acquisition server to acquire data from a g.NEEDaccess device.
	 *
	 * TODO: details
	 *
	 * \sa CConfigurationgNautilusInterface
	 */
	class CDrivergNautilusInterface : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDrivergNautilusInterface(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
		virtual ~CDrivergNautilusInterface(void);
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

	protected:
		
		SettingsHelper m_oSettings;
		
		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;

		// Replace this generic Header with any specific header you might have written
		OpenViBEAcquisitionServer::CHeader m_oHeader;

		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;
		OpenViBE::float32* m_pSample;
		OpenViBE::uint32 m_ui32DeviceIndex;
		OpenViBE::uint32 m_ui32ActualDeviceIndex;
		OpenViBE::uint32 m_ui32BufferSize;
		OpenViBE::uint32 m_ui32AvailableScans;
		OpenViBE::float32* m_pBuffer;

		OpenViBE::int32 m_i32NotchFilterIndex;
		OpenViBE::int32 m_i32BandPassFilterIndex;

		OpenViBE::float64 m_f64Sensitivity;
		OpenViBE::int32 m_i32InputSource;
		OpenViBE::uint32 m_ui32NetworkChannel;
		OpenViBE::boolean m_bDigitalInputEnabled;
		OpenViBE::boolean m_bNoiseReductionEnabled;
		OpenViBE::boolean m_bCAREnabled;
		OpenViBE::boolean m_bAccelerationDataEnabled;
		OpenViBE::boolean m_bCounterEnabled;
		OpenViBE::boolean m_bLinkQualityEnabled;
		OpenViBE::boolean m_bBatteryLevelEnabled;
		OpenViBE::boolean m_bValidationIndicatorEnabled;
		vector<OpenViBE::uint16> m_vSelectedChannels;
		vector<OpenViBE::int32> m_vBipolarChannels;
		vector<OpenViBE::boolean> m_vNoiseReduction;
		vector<OpenViBE::boolean> m_vCAR;
		OpenViBE::uint32 m_ui32AcquiredChannelCount;

		GDS_HANDLE m_pDevice;
		GDS_RESULT m_oGdsResult;
		GDS_GNAUTILUS_CONFIGURATION m_oNautilusDeviceCfg;
		std::string m_sDeviceSerial;
		OpenViBE::uint32 m_ui32DeviceCount;

	private:

		/*
		 * Insert here all specific attributes, such as USB port number or device ID.
		 * Example :
		 */
		// OpenViBE::uint32 m_ui32ConnectionID;
	};
};

#endif // TARGET_HAS_ThirdPartyGNEEDaccessAPI

#endif // __OpenViBE_AcquisitionServer_CDrivergNautilusInterface_H__
