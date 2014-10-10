#ifndef __OpenViBE_AcquisitionServer_CDriverMBTSmarting_H__
#define __OpenViBE_AcquisitionServer_CDriverMBTSmarting_H__

#if defined(TARGET_OS_Windows)

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>
#include <socket\IConnectionClient.h>
#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <vector>
#include <windows.h>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDriverMBTSmarting
	 * \author mBrainTrain dev team
	 * \date Tue Oct 2 14:00:37 2014
	 * \erief The CDriverMBTSmarting allows the acquisition server to acquire data from a MBTSmarting device.
	 *
	 * TODO: details
	 *
	 * \sa CConfigurationMBTSmarting
	 */
	class CDriverMBTSmarting : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverMBTSmarting(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
		virtual void release(void);//openEEG
		virtual ~CDriverMBTSmarting(void);
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
	
		HANDLE  m_i32FileDescriptor;
		
	protected:
		bool WriteABuffer(char * lpBuf, DWORD dwToWrite, HANDLE m_i32FileDescriptor);
		OpenViBE::boolean parseByteP2(HANDLE i32FileDescriptor);
		OpenViBE::boolean initTTY(HANDLE * pFileDescriptor, OpenViBE::uint32 ui32TtyNumber);
		OpenViBE::int32 readPacketFromTTY(HANDLE i32FileDescriptor);
		void closeTTY(HANDLE i32FileDescriptor);
		
		
		SettingsHelper m_oSettings;

		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;

		// Replace this generic Header with any specific header you might have written
		OpenViBEAcquisitionServer::CHeader m_oHeader;

		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;
		OpenViBE::float32* m_pSample;

		
		int m_ui8Counter;
		OpenViBE::float32 m_ui16GyroX;
		OpenViBE::float32 m_ui16GyroY;
		OpenViBE::float32 m_ui16GyroZ;
		int m_ui8Battery;
		unsigned char m_ui8CheckSum;
	
		bool testMode;
		int m_totalSamples;
		int m_failedSamples;
		int m_receivedSamples;
		int m_prevSample;

		OpenViBE::uint32 m_ui32ChannelCount;
		OpenViBE::uint32 m_ui32DeviceIdentifier;

		
		std::vector < OpenViBE::float32 > m_vChannelBuffer2;
		std::vector < unsigned char> m_byteArray;
	
		OpenViBE::CString m_sTTYName;

	private:

		/*
		 * Insert here all specific attributes, such as USB port number or device ID.
		 * Example :
		 */
		int sample_number;
	
	};
};

#endif // __OpenViBE_AcquisitionServer_CDriverMBTSmarting_H__

#endif
