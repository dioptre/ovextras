#include "ovasCDriverOpenBCI.h"
#include "ovasCConfigurationOpenBCI.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>
#include <system/ovCMemory.h>
#include <cmath>
#include <iostream>

#include <time.h>
#include <cstring>

#if defined TARGET_OS_Windows
 #include <windows.h>
 #include <winbase.h>
 #include <cstdio>
 #include <cstdlib>
 #include <commctrl.h>
 #include <winsock2.h> // htons and co.
 //#define TERM_SPEED 57600
 #define TERM_SPEED CBR_115200
#elif defined TARGET_OS_Linux
 #include <cstdio>
 #include <unistd.h>
 #include <fcntl.h>
 #include <termios.h>
 #include <sys/select.h>
 #include <netinet/in.h> // htons and co.
 #include <unistd.h>
 #define TERM_SPEED B115200
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

	// init state
	m_ui16Readstate=0;
	m_ui16ExtractPosition=0;
	m_i16SampleNumber  = -1;
	m_ui16ExtractPosition = 0;

	if(!this->initTTY(&m_i32FileDescriptor, m_ui32DeviceIdentifier!=uint32(-1)?m_ui32DeviceIdentifier:1))
	{
		return false;
	}
	
	// check board status and print response
	if(!this->initBoard(m_i32FileDescriptor))
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
	
	// init scale factor
	ScaleFacuVoltsPerCount = ADS1299_VREF/(pow(2.,23)-1)/ADS1299_GAIN*1000000.;
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
					// convert value to microvolt
					// TODO: check scale + depend of gain
					m_pSample[j]= (float32)m_vChannelBuffer[i][j] * ScaleFacuVoltsPerCount;
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

// Convert EEG value format from int24 MSB (network order) to int32 host
int32 CDriverOpenBCI::interpret24bitAsInt32(std::vector < uint8 > byteBuffer) {
	int32 newInt = (byteBuffer[2] << 16) | (byteBuffer[1] << 8) | byteBuffer[0];
	// negative number if most significant > 128
	if (byteBuffer[0] > 127) {
		newInt |= 0xFF << 24; 
	}
	// converts to host endianness on the fly
	return ntohl(newInt);
}

//___________________________________________________________________//
//                                                                   //
// return sample number once one is received (between 0 and 255, -1 if none)
OpenViBE::int16 CDriverOpenBCI::parseByteP2(uint8 ui8Actbyte)
{
	// finished to read sample or not
	bool l_bSampleStatus = false;
	
	switch(m_ui16Readstate)
	{
		case 0:
			// if first byte is not the one expected, won't go further
			if(ui8Actbyte==0xA0)
			{
				m_ui16Readstate++;
			}
			else
			{
				m_ui16Readstate=0;
			}
			// reset sample info
			m_i16SampleNumber  = -1;
			m_ui16ExtractPosition = 0;
			m_ui8SampleBufferPosition = 0;
			break;
		// Byte 2: Sample Number
		case 1:
			m_i16SampleNumber = ui8Actbyte;
			m_ui16Readstate++;
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
				if (m_ui8SampleBufferPosition == EEGValueBufferSize)
				{
					// fill channel buffer, converting at the same time from 24 to 32 bits
					m_vChannelBuffer2[m_ui16ExtractPosition] = interpret24bitAsInt32(m_vEEGValueBuffer);
					// reset for next value
					m_ui8SampleBufferPosition = 0;
					m_ui16ExtractPosition++;
				}
			}
			// finished with EEG
			if (m_ui16ExtractPosition == EEGNbValuesPerSample) {
				// next step: accelerometer
				m_ui16Readstate++;
				// re-use the same variable to know position inside accelerometer block (I know, I'm bad!). Err... FIXME
				m_ui16ExtractPosition=0;
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
				if (m_ui8SampleBufferPosition == AccValueBufferSize) { 
					// convert 2 bytes buffer to int16 (network order)
					int16 accValue = m_vAccValueBuffer[1]<< 8;
					accValue |= m_vAccValueBuffer[0];
					accValue=ntohs(accValue);
					// fill channel buffer, positioning after EEG values
					m_vChannelBuffer2[EEGNbValuesPerSample+m_ui16ExtractPosition] = accValue;
					// reset for next value
					m_ui8SampleBufferPosition = 0;
					m_ui16ExtractPosition++;
				 }
			}
			// finished with acc
			if (m_ui16ExtractPosition == AccNbValuesPerSample) {
				// next step: footer
				m_ui16Readstate++;
			}
			break;
		// footer: Byte 33: 0xC0
		case 4:
			// expected footer: perfect, returns sample number
			if(ui8Actbyte==0xC0)
			{
				// we shall pass
				l_bSampleStatus = true;
			}
			// if last byte is not the one expected, discard whole sample
			else
			{
			}
			// whatever happened, it'll be the end of this journey
			m_ui16Readstate=0;
			break;
		// uh-oh, should not be there
		default:
			m_ui16Readstate = 0;
			break;
	}
	// if it's a GO, add channel values, returns sample number
	if (l_bSampleStatus) {
		m_vChannelBuffer.push_back(m_vChannelBuffer2);
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
	dcb.BaudRate  = TERM_SPEED;
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


// bad but portable method to sleep
// mseconds: miliseconds to sleep
void CDriverOpenBCI::BadSleep(int32 mseconds)
{
	
	clock_t goal = mseconds + clock();
	while (goal > clock());
}

// if waitForResponse, will print (and wait for) this particular sequence of character
boolean CDriverOpenBCI::boardWriteAndPrint(::FD_TYPE i32FileDescriptor, const char * cmd, const char * waitForResponse) {
	// no command: don't go further
	if (strlen(cmd) == 0) {
		return true;
	}
	uint32 cmdSize = strlen(cmd);
	std::cout << "Command size: " << cmdSize << std::endl;
	uint8  l_ui8ReadBuffer[1]; // FIXME: size...
	uint32 l_ui32BytesProcessed=0;

#if defined TARGET_OS_Windows

	uint32 l_ui32ReadLength=0;
	uint32 l_ui32ReadOk=0;
	uint32 l_ui32WriteOk=0;
	struct _COMSTAT l_oStatus;
	::DWORD l_dwState;
	
	// write
	unsigned int spot = 0;
	do {
		std::cout << "write: " << cmd[spot] << std::endl;
		::WriteFile(i32FileDescriptor, (LPCVOID) cmd[spot], 1, (LPDWORD)&l_ui32WriteOk, 0);

		// give some time to the board to register
		Sleep(2000);
	} while (spot < cmdSize && l_ui32WriteOk == 1); //traling 
	// ended before end, problem
	if (spot != cmdSize) {;
		std::cout << "stop before end" << std::endl;
		return false;
	}
	// read
	if (strlen(waitForResponse) > 0) {
	  
		if(::ClearCommError(i32FileDescriptor, &l_dwState, &l_oStatus))
		{
			l_ui32ReadLength=l_oStatus.cbInQue;
		}

		for(l_ui32BytesProcessed=0; l_ui32BytesProcessed<l_ui32ReadLength; l_ui32BytesProcessed++)
		{
			::ReadFile(i32FileDescriptor, l_ui8ReadBuffer, 1, (LPDWORD)&l_ui32ReadOk, 0);
			if(l_ui32ReadOk==1)
			{
				std::cout << l_ui8ReadBuffer[0];
			}
		}
	}
	else {
		std::cout << "Do not expect reponse." << std::endl;
	}

	
#elif defined TARGET_OS_Linux
	fd_set  l_inputFileDescriptorSet;
	struct timeval l_timeout;
	size_t l_ui32ReadLength=0;
	bool finished=false;

	l_timeout.tv_sec=0;
	l_timeout.tv_usec=0;
	
 	FD_ZERO(&l_inputFileDescriptorSet);
	FD_SET(i32FileDescriptor, &l_inputFileDescriptorSet);
	
	int n_written = 0;
	unsigned int spot = 0;
	do {
		std::cout << "write: " << cmd[spot] << std::endl;
		n_written = write(i32FileDescriptor, &cmd[spot], 1 );
		// give some time to the board to register
		sleep(2);
		spot += n_written;
	} while (spot < cmdSize && n_written > 0); //traling 
	// ended before end, problem
	if (spot != cmdSize) {;
		std::cout << "stop before end" << std::endl;
		return false;
	}
	
	if (strlen(waitForResponse) > 0) {
		std::cout << "Wait for: " << waitForResponse << std::endl;
		do
		{
			switch(::select(i32FileDescriptor+1, &l_inputFileDescriptorSet, NULL, NULL, &l_timeout))
			{
				case -1: // error or timeout
				case  0:
					finished=true;
					break;

				default:
					if(FD_ISSET(i32FileDescriptor, &l_inputFileDescriptorSet))
					{
						l_ui32ReadLength=::read(i32FileDescriptor, l_ui8ReadBuffer, sizeof(l_ui8ReadBuffer));		  
						if((l_ui32ReadLength) > 0)
						{					   
							for(l_ui32BytesProcessed=0; l_ui32BytesProcessed<l_ui32ReadLength; l_ui32BytesProcessed++)
							{
								std::cout << l_ui8ReadBuffer[l_ui32BytesProcessed];
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
		std::cout << std::endl;
		std::cout << "finish to read" << std::endl;
	}
	else {
		std::cout << "Do not expect reponse." << std::endl;
	}
#else
#endif
	// FIXME: only if waitForResponse
	return true;

}

boolean CDriverOpenBCI::initBoard(::FD_TYPE i32FileDescriptor)
{	
	// reset 32-bit board (no effect with 8bit board)
	const char * cmd = "v";
	boardWriteAndPrint(i32FileDescriptor, cmd, "$$$");
	
	// start stream
	boardWriteAndPrint(i32FileDescriptor, "b", "");
	
	// send commands...
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

	uint8  l_ui8ReadBuffer[1]; // FIXME: size...
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
			// sample number returned: complete sample/packet
			if(this->parseByteP2(l_ui8ReadBuffer[0]) != -1)
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
					l_ui32ReadLength=::read(i32FileDescriptor, l_ui8ReadBuffer, sizeof(l_ui8ReadBuffer));		  
					if((l_ui32ReadLength) > 0)
					{					   
						for(l_ui32BytesProcessed=0; l_ui32BytesProcessed<l_ui32ReadLength; l_ui32BytesProcessed++)
						{
							int l_curPacket = this->parseByteP2(l_ui8ReadBuffer[l_ui32BytesProcessed]);
							// number returned: only eif complete sample/packet
							if(l_curPacket != -1)
							{
								// check packet drop
								if ((m_ui8LastPacketNumber + 1) % 256 !=  l_curPacket) {
									// FIXME: use logging
									std::cout << "Last packet drop! Last: " << (int) m_ui8LastPacketNumber << ", current packet number: " << l_curPacket << std::endl;
								}
							        m_ui8LastPacketNumber = l_curPacket;
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


