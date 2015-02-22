#ifndef __OpenViBE_AcquisitionServer_CDriverOpenBCI_H__
#define __OpenViBE_AcquisitionServer_CDriverOpenBCI_H__

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#if defined TARGET_OS_Windows
 typedef void * FD_TYPE;
#elif defined TARGET_OS_Linux
 typedef OpenViBE::int32 FD_TYPE;
#else
#endif

#include <vector>

namespace OpenViBEAcquisitionServer
{
	class CDriverOpenBCI : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverOpenBCI(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
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

	protected:

		// void logPacket(void);
		OpenViBE::int16 parseByteP2(OpenViBE::uint8 ui8Actbyte);

		OpenViBE::boolean initTTY(::FD_TYPE * pFileDescriptor, OpenViBE::uint32 ui32TtyNumber);
		OpenViBE::int32 readPacketFromTTY(::FD_TYPE i32FileDescriptor);
		void closeTTY(::FD_TYPE i32FileDescriptor);

	protected:

		SettingsHelper m_oSettings;

		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;
		OpenViBEAcquisitionServer::CHeader m_oHeader;

		OpenViBE::uint32 m_ui32ChannelCount;
		OpenViBE::uint32 m_ui32DeviceIdentifier;
		OpenViBE::float32* m_pSample;

		::FD_TYPE  m_i32FileDescriptor;
		
		// OpenBCI protocol related
		OpenViBE::int16 m_i16SampleNumber; // returned by the board
		OpenViBE::uint16 m_ui16Readstate; // position in the sample (see doc)
		OpenViBE::uint8 m_ui8SampleBufferPosition;// position in the buffer
		std::vector < OpenViBE::uint8 > m_vEEGValueBuffer; // buffer for one EEG value (float24)
		const static OpenViBE::uint8 EEGValueBufferSize = 3; // float24 == 3 bytes
		std::vector < OpenViBE::uint8 > m_vAccValueBuffer; // buffer for one accelerometer value (int16)
		const static OpenViBE::uint8 AccValueBufferSize = 2; // int16 == 2 bytes
		const static OpenViBE::uint8 EEGNbValuesPerSample = 8; // the board send EEG values 8 by 8
		const static OpenViBE::uint8 AccNbValuesPerSample = 3; // 3 accelerometer data per sample
		
		OpenViBE::uint16 m_ui16ExtractPosition; // used to situate sample reading both with EEG and accelerometer data
		
		OpenViBE::uint8  m_ui8PacketNumber;
		OpenViBE::uint8  m_ui8LastPacketNumber;
		OpenViBE::uint16 m_ui16Switches;

		std::vector < std::vector < OpenViBE::int32 > > m_vChannelBuffer;
		std::vector < OpenViBE::int32 > m_vChannelBuffer2;

		OpenViBE::CString m_sTTYName;
	};
};

#endif // __OpenViBE_AcquisitionServer_CDriverOpenBCI_H__
