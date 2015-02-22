#include "ovasCDriverOpenBCI.h"
#include "ovasCConfigurationOpenBCI.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>
#include <system/ovCMemory.h>
#include <cmath>
#include <iostream>

#if defined TARGET_OS_Windows
 #include <windows.h>
 #include <winbase.h>
 #include <cstdio>
 #include <cstdlib>
 #include <commctrl.h>
 #define TERM_SPEED 57600
#elif defined TARGET_OS_Linux
 #include <cstdio>
 #include <unistd.h>
 #include <fcntl.h>
 #include <termios.h>
 #include <sys/select.h>
 #define TERM_SPEED B57600
#else
#endif

#define boolean OpenViBE::boolean
using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;

//___________________________________________________________________//
//                                                                   //

CDriverOpenBCI::CDriverOpenBCI(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_OpenBCI", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32ChannelCount(6)
	,m_ui32DeviceIdentifier(uint32(-1))
	,m_pSample(NULL)
{
	m_oHeader.setSamplingFrequency(256);
	m_oHeader.setChannelCount(m_ui32ChannelCount);

	m_oSettings.add("Header", &m_oHeader);
	m_oSettings.add("DeviceIdentifier", &m_ui32DeviceIdentifier);
	m_oSettings.load();

}

void CDriverOpenBCI::release(void)
{
	delete this;
}

const char* CDriverOpenBCI::getName(void)
{
	return "OpenBCI";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverOpenBCI::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	if(m_rDriverContext.isConnected()) { return false; }

	if(!m_oHeader.isChannelCountSet()
	 ||!m_oHeader.isSamplingFrequencySet())
	{
		return false;
	}

	m_ui16Readstate=0;
	m_ui16ExtractPosition=0;

	if(!this->initTTY(&m_i32FileDescriptor, m_ui32DeviceIdentifier!=uint32(-1)?m_ui32DeviceIdentifier:1))
	{
		return false;
	}

	m_pSample=new float32[m_oHeader.getChannelCount()];
	if(!m_pSample)
	{
		delete [] m_pSample;
		m_pSample=NULL;
		return false;
	}
	// in OpenBCI protocol, a sample consists in EEG + Accelerometer data
	m_vChannelBuffer2.resize(EEGNbValuesPerSample+AccNbValuesPerSample);

	m_pCallback=&rCallback;
	m_ui32ChannelCount=m_oHeader.getChannelCount();
	m_ui8LastPacketNumber=0;

	m_rDriverContext.getLogManager() << LogLevel_Debug << CString(this->getName()) << " driver initialized.\n";
	
	// init buffer for EEG value and accel values
	m_vEEGValueBuffer.resize(EEGValueBufferSize);
	m_vAccValueBuffer.resize(AccValueBufferSize);
	return true;
}

boolean CDriverOpenBCI::start(void)
{
	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }
	m_rDriverContext.getLogManager() << LogLevel_Debug << CString(this->getName()) << " driver started.\n";
	return true;
}

boolean CDriverOpenBCI::loop(void)
{
	if(!m_rDriverContext.isConnected()) { return false; }

	if(this->readPacketFromTTY(m_i32FileDescriptor)<0)
	{
		m_rDriverContext.getLogManager() << LogLevel_ImportantWarning << "Could not receive data from " << m_sTTYName << "\n";
		return false;
	}

	if(m_vChannelBuffer.size()!=0)
	{
		if(m_rDriverContext.isStarted())
		{
			for(uint32 i=0; i<m_vChannelBuffer.size(); i++)
			{
				for(uint32 j=0; j<m_ui32ChannelCount; j++)
				{
					m_pSample[j]=(float32)m_vChannelBuffer[i][j]-512.f;
				}
				m_pCallback->setSamples(m_pSample, 1);
			}
			m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());
		}
		m_vChannelBuffer.clear();
	}

	return true;
}

boolean CDriverOpenBCI::stop(void)
{
	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return false; }
	m_rDriverContext.getLogManager() << LogLevel_Debug << CString(this->getName()) << " driver stopped.\n";
	return true;
}

boolean CDriverOpenBCI::uninitialize(void)
{
	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	this->closeTTY(m_i32FileDescriptor);

	m_rDriverContext.getLogManager() << LogLevel_Debug << CString(this->getName()) << " driver closed.\n";

	delete [] m_pSample;
	m_pSample=NULL;
	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverOpenBCI::isConfigurable(void)
{
	return true;
}

boolean CDriverOpenBCI::configure(void)
{
	CConfigurationOpenBCI m_oConfiguration(
		OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-OpenBCI.ui", 
		m_ui32DeviceIdentifier);

	if(!m_oConfiguration.configure(m_oHeader)) {
		return false;
	}

	m_oSettings.save();

	return true;
}

//___________________________________________________________________//
//                                                                   //
// return sample number once one is receivced (between 0 and 255, -1 if none)
OpenViBE::int16 CDriverOpenBCI::parseByteP2(uint8 ui8Actbyte)
{
	// finished to read sample or not
	bool l_bSampleStatus = false;
	
	switch(m_ui16Readstate)
	{
		//  Byte 1: 0xA0
		case 0:
			// if first byte is not the one expected, won't go further
			if(ui8Actbyte==160)
			{
				m_ui16Readstate++;
			        // FIXME: DEBUG, may be info about the board
				std::cout << "OpenBCI got header" << std::endl;
			}
			else
			{
				m_ui16Readstate=0;
				// FIXME: DEBUG, may be info about the board
				std::cout << "OpenBCI reads: " << (char) ui8Actbyte << std::endl;
			}
			// reset sample info
			m_i16SampleNumber  = -1;
			m_ui16ExtractPosition = 0;
			m_ui8SampleBufferPosition = 0;
			// zero EEG and accel buffers
			// zero accel buffer
			m_vEEGValueBuffer.clear();
			m_vAccValueBuffer.clear();
			break;
		// Byte 2: Sample Number
		case 1:
			m_i16SampleNumber = ui8Actbyte;
			m_ui16Readstate++;
			// FIXME: DEBUG
			std::cout << "Sample number: " << (char) ui8Actbyte << std::endl;
			break;
		// reading EEG data 
		/* 
		Note: values are 24-bit signed, MSB first

		* Bytes 3-5: Data value for EEG channel 1
		* Bytes 6-8: Data value for EEG channel 2
		* Bytes 9-11: Data value for EEG channel 3
		* Bytes 12-14: Data value for EEG channel 4
		* Bytes 15-17: Data value for EEG channel 5
		* Bytes 18-20: Data value for EEG channel 6
		* Bytes 21-23: Data value for EEG channel 6
		* Bytes 24-26: Data value for EEG channel 8
		*/
		case 2:
			if (m_ui16ExtractPosition < EEGNbValuesPerSample) {	
				// fill EEG buffer
				if (m_ui8SampleBufferPosition < EEGValueBufferSize) {
					m_vEEGValueBuffer[m_ui8SampleBufferPosition] = ui8Actbyte;
					m_ui8SampleBufferPosition++;
				}
				// we got EEG value
				else {
					m_ui8SampleBufferPosition = 0;
					m_ui16ExtractPosition++;
				}
			}
			// finished with EEG
			else {
				// TODO: fill channel buffer
				// text step: accelerometer
				m_ui16Readstate++;
				// re-use the same variable to know position inside accelerometer block (I know, I'm bad!)
				m_ui16ExtractPosition = 0;
			}
			break;
		// reading accelerometer data
		/*
		 Note: values are 16-bit signed, MSB first
		 
		* Bytes 27-28: Data value for accelerometer channel X
		* Bytes 29-30: Data value for accelerometer channel Y
		* Bytes 31-32: Data value for accelerometer channel Z
		*/
		case 3:
			if (m_ui16ExtractPosition < AccNbValuesPerSample) {	
				// fill Acc buffer
				if (m_ui8SampleBufferPosition < AccValueBufferSize) {
					m_vAccValueBuffer[m_ui8SampleBufferPosition] = ui8Actbyte;
					m_ui8SampleBufferPosition++;
				}
				// we got Acc value
				else {
					m_ui8SampleBufferPosition = 0;
					m_ui16ExtractPosition++;
				}
			}
			// finished with acc
			else {
				// TODO: fill acc buffer
				m_ui16Readstate++;
			}
			break;
		// footer: Byte 33: 0xC0
		case 4:
			// expected footer: perfect, returns sample number
			if(ui8Actbyte==192)
			{
			        // FIXME: DEBUG, may be info about the board
				std::cout << "OpenBCI got footer" << std::endl;
				// we shall pass
				l_bSampleStatus = true;
			}
			// if last byte is not the one expected, discard whole sample
			else
			{
				// FIXME: DEBUG, may be info about the board
				std::cout << "OpenBCI reads instead of footer: " << (char) ui8Actbyte << std::endl;
			}
			// whatever happened, it'll be the end of this journey
			m_ui16Readstate++;
			break;
		// next time will be a new time
		default:
			m_ui16Readstate = 0;
			break;
	}
	// if it's a GO, returns sample number 
	if (l_bSampleStatus) {
		return m_i16SampleNumber;
	}
	// by default we're not ready
	return -1;
}

boolean CDriverOpenBCI::initTTY(::FD_TYPE* pFileDescriptor, uint32 ui32TTYNumber)
{
	char l_sTTYName[1024];

#if defined TARGET_OS_Windows

	::sprintf(l_sTTYName, "\\\\.\\COM%d", ui32TTYNumber);
	DCB dcb = {0};
	*pFileDescriptor=::CreateFile(
		(LPCSTR)l_sTTYName,
		GENERIC_READ|GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if(*pFileDescriptor == INVALID_HANDLE_VALUE)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not open port [" << CString(l_sTTYName) << "]\n";
		return false;
	}

	if(!::GetCommState(*pFileDescriptor, &dcb))
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not get comm state on port [" << CString(l_sTTYName) << "]\n";
		return false;
	}

	// update DCB rate, byte size, parity, and stop bits size
	dcb.DCBlength = sizeof(dcb);
	dcb.BaudRate  = CBR_56000;
	dcb.ByteSize  = 8;
	dcb.Parity    = NOPARITY;
	dcb.StopBits  = ONESTOPBIT;
	dcb.EvtChar   = '\0';

	// update flow control settings
	dcb.fDtrControl       = DTR_CONTROL_ENABLE;
	dcb.fRtsControl       = RTS_CONTROL_ENABLE;
	dcb.fOutxCtsFlow      = FALSE;
	dcb.fOutxDsrFlow      = FALSE;
	dcb.fDsrSensitivity   = FALSE;;
	dcb.fOutX             = FALSE;
	dcb.fInX              = FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.XonChar           = 0;
	dcb.XoffChar          = 0;
	dcb.XonLim            = 0;
	dcb.XoffLim           = 0;
	dcb.fParity           = FALSE;

	::SetCommState(*pFileDescriptor, &dcb);
	::SetupComm(*pFileDescriptor, 64/*1024*/, 64/*1024*/);
	::EscapeCommFunction(*pFileDescriptor, SETDTR);
	::SetCommMask (*pFileDescriptor, EV_RXCHAR | EV_CTS | EV_DSR | EV_RLSD | EV_RING);

#elif defined TARGET_OS_Linux

	struct termios l_oTerminalAttributes;

	// open ttyS<i> for i < 10, else open ttyUSB<i-10>
	if(ui32TTYNumber<10)
	{
		::sprintf(l_sTTYName, "/dev/ttyS%d", ui32TTYNumber);
	}
	else
	{
		::sprintf(l_sTTYName, "/dev/ttyUSB%d", ui32TTYNumber-10);
	}

	if((*pFileDescriptor=::open(l_sTTYName, O_RDWR))==-1)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not open port [" << CString(l_sTTYName) << "]\n";
		return false;
	}

	if(::tcgetattr(*pFileDescriptor, &l_oTerminalAttributes)!=0)
	{
		::close(*pFileDescriptor);
		*pFileDescriptor=-1;
		m_rDriverContext.getLogManager() << LogLevel_Error << "terminal: tcgetattr() failed - did you use the right port [" << CString(l_sTTYName) << "] ?\n";
		return false;
	}

	/* l_oTerminalAttributes.c_cflag = TERM_SPEED | CS8 | CRTSCTS | CLOCAL | CREAD; */
	l_oTerminalAttributes.c_cflag = TERM_SPEED | CS8 | CLOCAL | CREAD;
	l_oTerminalAttributes.c_iflag = 0;
	l_oTerminalAttributes.c_oflag = OPOST | ONLCR;
	l_oTerminalAttributes.c_lflag = 0;
	if(::tcsetattr(*pFileDescriptor, TCSAFLUSH, &l_oTerminalAttributes)!=0)
	{
		::close(*pFileDescriptor);
		*pFileDescriptor=-1;
		m_rDriverContext.getLogManager() << LogLevel_Error << "terminal: tcsetattr() failed - did you use the right port [" << CString(l_sTTYName) << "] ?\n";
		return false;
	}

#else

	return false;

#endif

	m_sTTYName = l_sTTYName;

	return true;
 }

void CDriverOpenBCI::closeTTY(::FD_TYPE i32FileDescriptor)
{
#if defined TARGET_OS_Windows
	::CloseHandle(i32FileDescriptor);
#elif defined TARGET_OS_Linux
	::close(i32FileDescriptor);
#else
#endif
}

int32 CDriverOpenBCI::readPacketFromTTY(::FD_TYPE i32FileDescriptor)
{
	m_rDriverContext.getLogManager() << LogLevel_Debug << "Enters readPacketFromTTY\n";

	uint8  l_ui8ReadBuffer[100];
	uint32 l_ui32BytesProcessed=0;
	int32  l_i32PacketsProcessed=0;

#if defined TARGET_OS_Windows

	uint32 l_ui32ReadLength=0;
	uint32 l_ui32ReadOk=0;
	struct _COMSTAT l_oStatus;
	::DWORD l_dwState;

	if(::ClearCommError(i32FileDescriptor, &l_dwState, &l_oStatus))
	{
		l_ui32ReadLength=l_oStatus.cbInQue;
	}

	for(l_ui32BytesProcessed=0; l_ui32BytesProcessed<l_ui32ReadLength; l_ui32BytesProcessed++)
	{
		::ReadFile(i32FileDescriptor, l_ui8ReadBuffer, 1, (LPDWORD)&l_ui32ReadOk, 0);
		if(l_ui32ReadOk==1)
		{
			if(this->parseByteP2(l_ui8ReadBuffer[0]))
			{
				l_i32PacketsProcessed++;
			}
		}
	}

#elif defined TARGET_OS_Linux

	fd_set  l_inputFileDescriptorSet;
	struct timeval l_timeout;
	size_t l_ui32ReadLength=0;
	bool finished=false;

	l_timeout.tv_sec=0;
	l_timeout.tv_usec=0;

	do
	{
		FD_ZERO(&l_inputFileDescriptorSet);
		FD_SET(i32FileDescriptor, &l_inputFileDescriptorSet);

		switch(::select(i32FileDescriptor+1, &l_inputFileDescriptorSet, NULL, NULL, &l_timeout))
		{
			case -1: // error or timeout
			case  0:
				finished=true;
				break;

			default:
				if(FD_ISSET(i32FileDescriptor, &l_inputFileDescriptorSet))
				{
					if((l_ui32ReadLength=::read(i32FileDescriptor, l_ui8ReadBuffer, 1)) > 0)
					{
						for(l_ui32BytesProcessed=0; l_ui32BytesProcessed<l_ui32ReadLength; l_ui32BytesProcessed++)
						{
							if(this->parseByteP2(l_ui8ReadBuffer[l_ui32BytesProcessed]))
							{
								l_i32PacketsProcessed++;
							}
						}
					}
				}
				else
				{
					finished=true;
				}
				break;
		}
	}
	while(!finished);

#else

#endif

	m_rDriverContext.getLogManager() << LogLevel_Debug << "Leaves readPacketFromTTY\n";
	return l_i32PacketsProcessed;
 }


