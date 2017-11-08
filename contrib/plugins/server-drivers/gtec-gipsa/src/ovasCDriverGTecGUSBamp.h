#ifndef __OpenViBE_AcquisitionServer_CDriverGTecGUSBamp_H__
#define __OpenViBE_AcquisitionServer_CDriverGTecGUSBamp_H__

#if defined TARGET_HAS_ThirdPartyGUSBampCAPI

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <Windows.h>

#include "ringbuffer.h"

#include <gtk/gtk.h>
#include <vector>

//threading
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory> // unique_ptr

#include <deque>
using namespace std;

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDriverGTecGUSBamp
	 * \author Anton Andreev, Gipsa-lab, VIBS team
	 * \date 19/07/2012
	 * \brief GTEC driver
	 *
	 * This driver was rewritten to match the code provided by Guger as much as possible. There are several things
	 * that all must work together so that higher frequencies are supported and no hardware triggers are lost.
	 *
	 * This driver supports several buffers so that the more than one GT_GetData can be executed in the beginning (QUEUE_SIZE)
	 * and then calls to GT_GetData are queued. This allows data to be processed by OpenVibe while waiting for the next result of
	 * a previously issued GT_GetData. The extra thread is added to support this and to allow for async IO. 
	 *
	 * Hardware triggers on the parallel port are supported.
	 *
	 * The driver supports several g.tec devices working with the provided async cables. There are several requirements for async
	 * acquisition to work properly and these are checked in verifySyncMode().
	 */

	struct GDevice
	{
		HANDLE handle;
		std::string   serial;
	};

	class CDriverGTecGUSBamp : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverGTecGUSBamp(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);

		virtual void release(void);
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

		OpenViBE::boolean CDriverGTecGUSBamp::acquire(void);

		void ConfigFiltering(HANDLE o_pDevice);

	protected:

		static const int BUFFER_SIZE_SECONDS = 2;         //the size of the GTEC ring buffer in seconds
		static const int GTEC_NUM_CHANNELS = 16;          //the number of channels without countig the trigger channel
		static const int QUEUE_SIZE = 8;//4 default		  //the number of GT_GetData calls that will be queued during acquisition to avoid loss of data
		static const int NUMBER_OF_SCANS = 32;            //the number of scans that should be received simultaneously (depending on the _sampleRate; see C-API documentation for this value!)
		
		OpenViBE::uint32 NumDevices();

		static const OpenViBE::uint32 nPoints = NUMBER_OF_SCANS * (GTEC_NUM_CHANNELS + 1);
		int validPoints;
		static const DWORD bufferSizeBytes;

		SettingsHelper m_oSettings;

		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;
		OpenViBEAcquisitionServer::CHeader m_oHeader;

		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;

		OpenViBE::float32* m_pSample;

		OpenViBE::uint32 m_ui32GlobalImpedanceIndex;

		OpenViBE::uint8 m_ui8CommonGndAndRefBitmap;

		OpenViBE::int32 m_i32NotchFilterIndex;
		OpenViBE::int32 m_i32BandPassFilterIndex;

		OpenViBE::boolean m_bTriggerInputEnabled;
		OpenViBE::boolean m_bBipolarEnabled; //electrodes are substracted in sepecific sequence 1-2=1, ... 15-16=15 which results in 8 instead of 16 electrodes - used for EMG
		OpenViBE::boolean m_bCalibrationSignalEnabled;
		OpenViBE::boolean m_bShowDeviceName; //adds the amplifier serial number to the name of the channel 

		OpenViBE::boolean m_bReconfigurationRequired; // After some gt calls, we may need reconfig

		OpenViBE::uint32 m_ui32AcquiredChannelCount;      //number of channels specified by the user, never counts the event channels

		OpenViBE::uint32 m_ui32TotalHardwareStimulations; //since start button clicked
		OpenViBE::uint32 m_ui32TotalDriverChunksLost;     //since start button clicked
		OpenViBE::uint32 m_ui32TotalRingBufferOverruns;
		OpenViBE::uint32 m_ui32TotalDataUnavailable; 

		//contains buffer per device and then QUEUE_SIZE buffers so that several calls to GT_GetData can be supported
		BYTE*** m_buffers;
	    OVERLAPPED** m_overlapped;

		OpenViBE::boolean m_flagIsFirstLoop;
		OpenViBE::boolean m_bufferOverrun;

		//ring buffer provided by Guger
		CRingBuffer<OpenViBE::float32> m_RingBuffer;	

		OpenViBE::uint32 m_ui32CurrentQueueIndex;

		std::unique_ptr<std::thread> m_ThreadPtr;
		OpenViBE::boolean m_bIsThreadRunning;

		std::mutex m_io_mutex;

		OpenViBE::float32 *m_bufferReceivedData;
		std::condition_variable  m_itemAvailable;

		OpenViBE::boolean ConfigureDevice(OpenViBE::uint32 deviceNumber);

		OpenViBE::boolean verifySyncMode();//Checks if devices are configured correctly when acquiring data from multiple devices

		//Selects which device to become the new master, used only when more than 1 device is available
		OpenViBE::boolean setMasterDevice(string targetMasterSerial); //0 first device
		void detectDevices();

	    OpenViBE::uint32 m_mastersCnt;
	    OpenViBE::uint32 m_slavesCnt;
	    string m_masterSerial;

		void remapChannelNames(void);   // Converts channel names while appending the device name and handling event channels
		void restoreChannelNames(void); // Restores channel names without the device name

		vector<std::string> m_vOriginalChannelNames;        // Channel names without the device name inserted
		vector<GDevice> m_vGDevices;                        // List of amplifiers

		std::string CDriverGTecGUSBamp::getSerialByHandler(HANDLE o_pDevice);

		// Stores information related to each channel available in the recording system
		struct Channel
		{
			OpenViBE::int32 m_i32Index;             // Channel index in openvibe Designer, -1 is unused
			OpenViBE::int32 m_i32OldIndex;          // Channel index in the user-settable channel name list
			OpenViBE::int32 m_i32GtecDeviceIndex;   // Device index of the gtec amplifier this channel is in
			OpenViBE::int32 m_i32GtecChannelIndex;  // Channel index in the device-specific numbering
			bool m_bIsEventChannel;                 // Is this the special digital channel?
		};

		// Channel indexes are seen as a sequence [dev1chn1, dev1chn2,...,dev1chnN, dev2chn1, dev2chn2, ..., dev2chnN, ...]
		// The following vector is used to map these 'system indexes' to openvibe channels
		vector<Channel> m_vChannelMap;
	};
};

#endif // TARGET_HAS_ThirdPartyGUSBampCAPI

#endif // __OpenViBE_AcquisitionServer_CDriverGTecGUSBamp_H__
