#ifndef __OpenViBE_AcquisitionServer_CConfigurationgNautilusInterface_H__
#define __OpenViBE_AcquisitionServer_CConfigurationgNautilusInterface_H__

#if defined(TARGET_HAS_ThirdPartyGNEEDaccessAPI)

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>
#include <GDSClientAPI.h>
#include <GDSClientAPI_gNautilus.h>

#include <sstream>
#include <algorithm>

using namespace std;

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CConfigurationgNautilusInterface
	 * \author g.tec medical engineering GmbH (g.tec medical engineering GmbH)
	 * \date Wed Aug 12 16:37:18 2015
	 * \brief The CConfigurationgNautilusInterface handles the configuration dialog specific to the g.NEEDaccess device.
	 *
	 * TODO: details
	 *
	 * \sa CDrivergNautilusInterface
	 */
	class CConfigurationgNautilusInterface : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
	public:

		// you may have to add to your constructor some reference parameters
		// for example, a connection ID:
		//CConfigurationgNautilusInterface(OpenViBEAcquisitionServer::IDriverContext& rDriverContext, const char* sGtkBuilderFileName, OpenViBE::uint32& rConnectionId);
		CConfigurationgNautilusInterface(OpenViBEAcquisitionServer::IDriverContext& rDriverContext,
										 const char* sGtkBuilderFileName,
										 string& rDeviceSerial,
										 OpenViBE::int32& rInputSource,
										 OpenViBE::uint32& rNetworkChannel,
										 OpenViBE::int32& rBandpassFilterIndex,
										 OpenViBE::int32& rNotchFilterIndex,
										 OpenViBE::float64& rSensitivity,
										 OpenViBE::boolean& rDigitalInputEnabled,
										 OpenViBE::boolean& rNoiseReductionEnabled,
										 OpenViBE::boolean& rCAREnabled,
										 OpenViBE::boolean& rAccelerationDataEnabled,
										 OpenViBE::boolean& rCounterEnabled,
										 OpenViBE::boolean& rLinkQualityEnabled,
										 OpenViBE::boolean& rBatteryLevelEnabled,
										 OpenViBE::boolean& rValidationIndicatorEnabled,
										 vector<OpenViBE::uint16>& rSelectedChannels,
										 vector<OpenViBE::int32>& rBipolarChannels,
										 vector<OpenViBE::boolean>& rCAR,
										 vector<OpenViBE::boolean>& rNoiseReduction
										 );

		virtual OpenViBE::boolean preConfigure(void);
		virtual OpenViBE::boolean postConfigure(void);
		
		//button callback functions
		void buttonChannelSettingsPressedCB(void);
		void buttonChannelSettingsApplyPressedCB(void);
		void buttonSensitivityFiltersPressedCB(void);
		void buttonSensitivityFiltersApplyPressedCB(void);
		OpenViBE::boolean getHardwareSettings(void);
		OpenViBE::boolean getChannelNames(void);
		OpenViBE::boolean getAvailableChannels(void);
		OpenViBE::boolean getFiltersForNewSamplingRate(void);
		void comboboxSampleRateChangedCB(void);
		void checkbuttonNoiseReductionChangedCB(void);
		void checkbuttonCARChangedCB(void);

	protected:

		OpenViBEAcquisitionServer::IDriverContext& m_rDriverContext;
		string& m_rDeviceSerial;
		OpenViBE::int32& m_rInputSource;
		OpenViBE::uint32& m_rNetworkChannel;
		OpenViBE::int32& m_rBandpassFilterIndex;
		OpenViBE::int32& m_rNotchFilterIndex;
		OpenViBE::float64& m_rSensitivity;
		OpenViBE::boolean& m_rDigitalInputEnabled;
		OpenViBE::boolean& m_rNoiseReductionEnabled;
		OpenViBE::boolean& m_rCAREnabled;
		OpenViBE::boolean& m_rAccelerationDataEnabled;
		OpenViBE::boolean& m_rCounterEnabled;
		OpenViBE::boolean& m_rLinkQualityEnabled;
		OpenViBE::boolean& m_rBatteryLevelEnabled;
		OpenViBE::boolean& m_rValidationIndicatorEnabled;
		vector<OpenViBE::uint16>& m_vSelectedChannels;
		vector<OpenViBE::int32>& m_vBipolarChannels;
		vector<OpenViBE::boolean>& m_vCAR;
		vector<OpenViBE::boolean>& m_vNoiseReduction;
		vector<OpenViBE::int32> m_vComboBoxBandpassFilterIndex;
		vector<OpenViBE::int32> m_vComboBoxNotchFilterIndex;
		vector<OpenViBE::float64> m_vComboBoxSensitivityValues;
		vector<OpenViBE::int32> m_vComboBoxInputSources;
		vector<OpenViBE::uint32> m_vComboBoxNetworkChannels;

	private:

		/*
		 * Insert here all specific attributes, such as a connection ID.
		 * use references to directly modify the corresponding attribute of the driver
		 * Example:
		 */
		// OpenViBE::uint32& m_ui32ConnectionID;
		OpenViBE::boolean openDevice(void);
		OpenViBE::boolean closeDevice(void);
		GDS_HANDLE m_ui64DeviceHandle;
		GDS_RESULT m_oGdsResult;
		vector<OpenViBE::boolean> m_vAvailableChannels;
		char (*m_sDeviceNames)[DEVICE_NAME_LENGTH_MAX];
		OpenViBE::boolean m_bConnectionOpen;
	};
};

#endif // TARGET_HAS_ThirdPartyGNEEDaccessAPI

#endif // __OpenViBE_AcquisitionServer_CConfigurationgNautilusInterface_H__
