/*
 * ovasCDriverBioSemiActiveTwo.h
 *
 * Copyright (c) 2012, Mensia Technologies SA. All rights reserved.
 * -- Rights transferred to Inria, contract signed 21.11.2014
 *
 */

#ifdef TARGET_HAS_ThirdPartyBioSemiAPI

#ifndef __Mensia_CBridgeBioSemiActiveTwo_H__
#define __Mensia_CBridgeBioSemiActiveTwo_H__

#include <vector>


#define BIOSEMI_ACTIVETWO_MAXCHANNELCOUNT 264

namespace Mensia
{
	enum EBioSemiError
	{
		BioSemiError_NoError,

		BioSemiError_OSOpenFailed,
		BioSemiError_OSCloseFailed,

		BioSemiError_EnableUSBHandshakeFailed,
		BioSemiError_DisableUSBHandshakeFailed,

		BioSemiError_ReadPointerFailed,

		BioSemiError_DeviceTypeChanged,
		BioSemiError_SpeedmodeChanged,
		BioSemiError_InvalidSpeedmode,

		BioSemiError_NoData,
		BioSemiError_NotEnoughDataInBuffer,
		BioSemiError_NoSync,
		BioSemiError_SyncLost,
		BioSemiError_BufferOverflow
	};


	class CBridgeBioSemiActiveTwo 
	{
	public:

		CBridgeBioSemiActiveTwo();
		virtual ~CBridgeBioSemiActiveTwo();
		
		virtual bool isDeviceConnected(void);
		virtual bool open(void);
		virtual bool start(void);
		virtual int read(void);
		virtual bool discard(void);
		virtual unsigned int getAvailableSampleCount(void);
		virtual bool consumeOneSamplePerChannel(float* pSampleBuffer, unsigned int uiBufferValueCount);
		virtual bool stop(void);
		virtual bool close(void);

		bool isSynced() const {return m_bBridgeSyncedWithDevice;}
		
		unsigned int getChannelCount() const {return m_uiChannelCount;}
		
		bool getTrigger(unsigned int uiIndex) const {return (uiIndex > m_vTriggers.size() ? false : m_vTriggers[uiIndex]);}
		bool isCMSInRange(void) const {return m_bCMSInRange;}
		bool isBatteryLow(void) const {return m_bBatteryLow;}
		bool isDeviceMarkII(void) const {return m_bActiveTwoMarkII;}
		virtual unsigned int getSamplingFrequency(void);

		unsigned int getLastError(void) {return m_uiLastError;}

	protected:
		// Handle to the device
		void * m_hDevice;
		// Ring buffer
		std::vector<char>  m_vBuffer;
		// 64bits buffer for USB_WRITE operations
		std::vector<char> m_vControlBuffer;

		/* deduced from the SYNC channel on first read */
		bool m_bFirstRead;
		bool m_bBridgeSyncedWithDevice;
		unsigned int m_uiChannelCount;
		unsigned int m_uiInitialChannelCount;
		
		/* From Status channel */
		std::vector<bool> m_vTriggers;
		bool m_bEpochStarted;
		bool m_bCMSInRange;
		bool m_bBatteryLow;
		bool m_bActiveTwoMarkII;
		bool m_bInitialActiveTwoMarkII;
		unsigned int m_uiSpeedMode;
		unsigned int m_uiInitialSpeedmode;
		virtual bool updateStatusFromValue(int iStatusChannelValue);

		// ring buffer indices
		unsigned int m_uiLastRingBufferByteIndex;    //Buffer filled up to this point in last read()
		unsigned int m_uiConsumptionByteIndex;       //Next consume() should start here, it should ALWAYS be a SYNC byte.
		virtual unsigned int getAvailableByteCount(void);
		virtual void consumeBytes(unsigned int uiByteToConsume);

		// just for stats
		unsigned int m_uiTotalByteReadCount;

		unsigned int m_uiLastError;
	};
};

#endif // __Mensia_CBridgeBioSemiActiveTwo_H__
#endif // TARGET_HAS_ThirdPartyBioSemiAPI