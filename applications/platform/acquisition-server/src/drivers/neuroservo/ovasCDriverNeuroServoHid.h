/*
* NeuroServo driver for OpenViBE
*
* \author (NeuroServo)
* \date Wed Nov 23 00:24:00 2016
*
*/

#ifndef __OpenViBE_AcquisitionServer_CDriverNeuroServoHid_H__
#define __OpenViBE_AcquisitionServer_CDriverNeuroServoHid_H__

#if defined TARGET_OS_Windows
#if defined TARGET_HAS_ThirdPartyNeuroServo

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

// Provicde necessary methods to allow connection with HID device
#include "HidDeviceApi\HidDevice.h"

#include <boost/lockfree/spsc_queue.hpp>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDriverNeuroServoHid
	 * \author  (NeuroServo)
	 * \date Wed Nov 23 00:24:00 2016
	 * \brief The CDriverNeuroServoHid allows the acquisition server to acquire data from a NeuroServo device.
	 *
	 * \sa CConfigurationNeuroServoHid
	 */
	class CDriverNeuroServoHid : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverNeuroServoHid(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
		virtual ~CDriverNeuroServoHid(void);
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

	/* Methods implemented for the specific needs of NeuroServo.
	   Execute the callback from the HID device services
	*/
	public:
		void processDataReceived(BYTE data[]);
		void deviceDetached();
		void deviceAttached();

	private:
		// Control "Automatic Shutdown" and "Device Light Enable" based on user configuration
		void deviceShutdownAndLightConfiguration();


	protected:
		
		SettingsHelper m_oSettings;
		
		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;

		OpenViBEAcquisitionServer::CHeader m_oHeader;

		OpenViBE::float32* m_pSample;
		OpenViBE::CStimulationSet m_oStimulationSet;
	
	private:
		// Create a buffer queue for 1 sec data
		boost::lockfree::spsc_queue<OpenViBE::float32, boost::lockfree::capacity<2048> > m_pBufferQueue;

		// Device related infos
		HidDevice m_oHidDevice;
		OpenViBE::uint16 m_ui16ProductId;
		OpenViBE::uint16 m_ui16VendorId;
		OpenViBE::uint16 m_ui16DataSize;
		OpenViBE::CString m_sDriverName;

		// Data processing related infos
		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;
		OpenViBE::uint32 m_ui32SampleIndexForSentBlock;
		OpenViBE::uint32 m_ui32NbSamplesReceived;

		OpenViBE::uint64 m_ui64TimeStampLastSentBlock;
		OpenViBE::uint64 m_ui64SendBlockRequiredTime;
		OpenViBE::uint64 m_ui64SendSampleRequiredTime;
		OpenViBE::uint64 m_ui64NbSwitchDrift;

		OpenViBE::int64 m_i64LastDriftSampleCount;
		OpenViBE::int64 m_i64DriftAutoCorrectionDelay;

		OpenViBE::float32 m_f32SampleValue;
		OpenViBE::float32 m_fDriftAutoCorrFactor;

		OpenViBE::boolean m_bIsDriftWasInEarlyDirection;
		OpenViBE::boolean m_bQueueOverflow;
		OpenViBE::boolean m_bQueueUnderflow;
		OpenViBE::boolean m_bDeviceEpochDetected;

		// Configuration
		OpenViBE::boolean m_bAutomaticShutdown;
		OpenViBE::boolean m_bShutdownOnDriverDisconnect;
		OpenViBE::boolean m_bDeviceLightEnable;
		OpenViBE::boolean m_bIsDeviceInitialized;

		// Device connection state
		OpenViBE::boolean m_bIsDeviceConnected;
	};
};

#endif
#endif // TARGET_OS_Windows

#endif // __OpenViBE_AcquisitionServer_CDriverNeuroServoHid_H__