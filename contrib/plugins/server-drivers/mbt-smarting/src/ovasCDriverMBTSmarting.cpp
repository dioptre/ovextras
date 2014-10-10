
#if defined(TARGET_OS_Windows)

#include "ovasCDriverMBTSmarting.h"
#include "ovasCConfigurationMBTSmarting.h"

#include <iostream>
#include <stdio.h>
#include <winbase.h>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <commctrl.h>
#define boolean OpenViBE::boolean
using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

#define PACKETSIZE 83

CDriverMBTSmarting::CDriverMBTSmarting(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_pCallback(NULL)
	,m_ui32ChannelCount(30)
	,m_ui32DeviceIdentifier(uint32(-1))
	,m_pSample(NULL)
	,m_oSettings("AcquisitionServer_Driver_mBTsmarting", m_rDriverContext.getConfigurationManager())
{
	
	m_oHeader.setSamplingFrequency(500);
	m_oHeader.setChannelCount(m_ui32ChannelCount);

	m_oSettings.add("Header", &m_oHeader);
	m_oSettings.load();	


}
bool CDriverMBTSmarting::WriteABuffer(char * lpBuf, DWORD dwToWrite, HANDLE m_i32FileDescriptor)
{
	OVERLAPPED osWrite = {0};
	DWORD dwWritten;
	DWORD dwRes;
	BOOL fRes;
	
	// Create this write operation's OVERLAPPED structure's hEvent.
	osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osWrite.hEvent == NULL)
	{
		std::cout<< "error creating overlapped event handle \n";
		// error creating overlapped event handle
		return FALSE;
	}
	// Issue write.
	if (!::WriteFile(m_i32FileDescriptor, lpBuf, dwToWrite, &dwWritten, &osWrite)) {
		if (GetLastError() != ERROR_IO_PENDING) { 
			std::cout<< "WriteFile failed, but isn't delayed. Report error and abort \n";
			// WriteFile failed, but isn't delayed. Report error and abort.
			fRes = FALSE;
		}
		else

			// Write is pending.
			dwRes = WaitForSingleObject(osWrite.hEvent, INFINITE);
		switch(dwRes)
		{
			// OVERLAPPED structure's event has been signaled. 
		case WAIT_OBJECT_0:
			if (!::GetOverlappedResult(m_i32FileDescriptor, &osWrite, &dwWritten, FALSE))
				fRes = FALSE;
			else
			{
				std::cout<< "Write operation completed successfully. \n";
				// Write operation completed successfully.
				fRes = TRUE;
			}
			break;

		default:
			std::cout<< "An error has occurred in WaitForSingleObject. \n";
			// An error has occurred in WaitForSingleObject.
			// This usually indicates a problem with the
			// OVERLAPPED structure's event handle.
			fRes = FALSE;
			break;
		}
	}

   else
	   // WriteFile completed immediately.
	   fRes = TRUE;
	   CloseHandle(osWrite.hEvent);
	   return fRes;
}
CDriverMBTSmarting::~CDriverMBTSmarting(void)
{
}

void CDriverMBTSmarting::release(void)
{
	delete this;
}
const char* CDriverMBTSmarting::getName(void)
{
	return "mBrainTrain Smarting";
	
}

#define COMMAND_SIZE 4096

boolean CDriverMBTSmarting::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	if(m_rDriverContext.isConnected()) { return false; }

	if(!m_oHeader.isChannelCountSet()
	 ||!m_oHeader.isSamplingFrequencySet())
	{
		return false;
	}

	if(!initTTY(&m_i32FileDescriptor, m_ui32DeviceIdentifier))
	{
		return false;
	}
	
	Sleep(2000);
	
	if (m_i32FileDescriptor == NULL)
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

	m_vChannelBuffer2.resize(24);

	m_pCallback=&rCallback;
	m_ui32ChannelCount=m_oHeader.getChannelCount();

	m_totalSamples = 0;
	m_failedSamples = 0;
	m_receivedSamples = 0;
	m_prevSample = -1;
		
	testMode = false;
	
	uint32 l_ui32ReadLength=0;
	struct _COMSTAT l_oStatus;
	::DWORD l_dwState;

	if(::ClearCommError(m_i32FileDescriptor, &l_dwState, &l_oStatus))
	{
		l_ui32ReadLength=l_oStatus.cbInQue;
	}

	if (l_ui32ReadLength > 1)
	{
		return true;
	}
	if (l_ui32ReadLength  == 1)
	{
		uint8  l_ui8ReadBuffer[100];
		uint32 l_ui32BytesProcessed=0;
		int32  l_i32PacketsProcessed=0;
		uint32 l_ui32ReadOk=0;
		::ReadFile(m_i32FileDescriptor, l_ui8ReadBuffer, 1, (LPDWORD)&l_ui32ReadOk, 0);
		if (l_ui32ReadOk != 0 && l_ui8ReadBuffer[0] == 'g')
		{
			return true;
		}			
	}

	return false;
}

boolean CDriverMBTSmarting::start(void)
{
	struct _COMSTAT l_oStatus;
	::DWORD l_dwState;
	uint8  l_ui8ReadBuffer[100];
	uint32 l_ui32BytesProcessed=0;
	int32  l_i32PacketsProcessed=0;
	uint32 l_ui32ReadLength=0;
	uint32 l_ui32ReadOk=0;

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }


	if(::ClearCommError(m_i32FileDescriptor, &l_dwState, &l_oStatus))
	{
		l_ui32ReadLength=l_oStatus.cbInQue;
	}

	for(l_ui32BytesProcessed=0; l_ui32BytesProcessed<l_ui32ReadLength; l_ui32BytesProcessed++)
	{	
		::ReadFile(m_i32FileDescriptor, l_ui8ReadBuffer, 1, (LPDWORD)&l_ui32ReadOk, 0);
	}

	WriteABuffer(">SC;ÿÿÿ<",8, m_i32FileDescriptor);
	WriteABuffer(">ON<", 4, m_i32FileDescriptor);
	sample_number = 1;
	::ClearCommError(m_i32FileDescriptor, &l_dwState, &l_oStatus);
	while(l_oStatus.cbInQue < 5*PACKETSIZE)
	{	
		::ClearCommError(m_i32FileDescriptor, &l_dwState, &l_oStatus);
	}	
	return true;
}

boolean CDriverMBTSmarting::loop(void)
{	
	if(!m_rDriverContext.isConnected()) { return false; }	

	if(m_rDriverContext.isStarted())
	{	
		readPacketFromTTY(m_i32FileDescriptor);
	}
	return true;
}

boolean CDriverMBTSmarting::stop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Debug << CString(this->getName()) << " driver stopped.\n";
	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return false; }
	WriteABuffer(">OFF<", 5, m_i32FileDescriptor);
	
	m_rDriverContext.getLogManager() << LogLevel_Debug << CString(this->getName()) << " driver stopped.\n";


	std::cout<< "Failed: " << m_failedSamples << "\n";
	std::cout<< "Success: " << m_receivedSamples << "\n";

	return true;
}
boolean CDriverMBTSmarting::uninitialize(void)
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

boolean CDriverMBTSmarting::isConfigurable(void)
{
	return true; // change to false if your device is not configurable
}

boolean CDriverMBTSmarting::configure(void)
{
	CConfigurationMBTSmarting m_oConfiguration(OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-MBTSmarting.ui", m_ui32DeviceIdentifier);
	if(!m_oConfiguration.configure(m_oHeader))
	{
		return false;
	}
	m_oSettings.save();

	return true;
}

boolean CDriverMBTSmarting::parseByteP2(HANDLE i32FileDescriptor)
{
	unsigned char b[2];
	unsigned char l_ui8ReadBufferTest[4];
	int id3;
	uint32 l_ui32ReadOk=0;
	uint32 l_ui32ReadLength=0;
	
	for(int receivedBytes = 1; receivedBytes<82; receivedBytes++)
	{
		l_ui8ReadBufferTest[(receivedBytes-1)%3] = m_byteArray[receivedBytes];
		if((receivedBytes)%3 == 0 && receivedBytes!=0 && receivedBytes<73)
		{				
			if(l_ui8ReadBufferTest[0] & 0x80)
				id3 =   (0xff << 24) | (l_ui8ReadBufferTest[0] << 16) | (l_ui8ReadBufferTest[1] << 8) | (l_ui8ReadBufferTest[2]<< 0);
			else
				id3 =     (l_ui8ReadBufferTest[0] << 16) | (l_ui8ReadBufferTest[1] << 8) | (l_ui8ReadBufferTest[2]);
			float chValue = float(id3) * 2.235174445530706e-2;
			
			m_vChannelBuffer2.push_back(chValue);
		}

		switch(receivedBytes){
			case 73:
				m_ui8Counter = 0;
				memcpy(&m_ui8Counter, &m_byteArray[73], 1);
				m_vChannelBuffer2.push_back(m_ui8Counter);
				break;
			case 75:
				m_ui16GyroX = 0;
				b[0] = m_byteArray[74];
				b[1] = m_byteArray[75];
				if(m_byteArray[74] & 0x80)
					m_ui16GyroX =   (0xff << 24) | (0xff << 16) | (b[0] << 8) | (b[1]<< 0);
				else
					m_ui16GyroX =     (b[0] << 8) | (b[1]<< 0);
				m_vChannelBuffer2.push_back(m_ui16GyroX);
				
				break;
			case 77:
				b[0] = m_byteArray[76];
				b[1] = m_byteArray[77];
				if(m_byteArray[76] & 0x80)
					m_ui16GyroY =   (0xff << 24) | (0xff << 16) | (b[0] << 8) | (b[1]<< 0);
				else
					m_ui16GyroY =     (b[0] << 8) | (b[1]<< 0);
				
				m_vChannelBuffer2.push_back(m_ui16GyroY);
				
				break;
			case 79:
				b[0] = m_byteArray[78];
				b[1] = m_byteArray[79];
				if(m_byteArray[78] & 0x80)
					m_ui16GyroZ =   (0xff << 24) | (0xff << 16) | (b[0] << 8) | (b[1]<< 0);
				else
					m_ui16GyroZ =     (b[0] << 8) | (b[1]<< 0);
				
				m_vChannelBuffer2.push_back(m_ui16GyroZ);
				
				break;
			case 80:
				m_ui8Battery = m_byteArray[80] & 0x7F;
				m_vChannelBuffer2.push_back(m_ui8Battery);
				break;
			case 81:
				m_ui8CheckSum = 0;
				memcpy(&m_ui8CheckSum, &m_byteArray[81], 1);
				m_vChannelBuffer2.push_back(m_ui8CheckSum);
				break;
		}
	}

	// ::DWORD l_dwState;
	// struct _COMSTAT l_oStatus;
	// if(::ClearCommError(i32FileDescriptor, &l_dwState, &l_oStatus))
	// {
		// l_ui32ReadLength=l_oStatus.cbInQue;
	// }
	
	// in case channel count is 24 or 30 send data in a raw
	if (m_ui32ChannelCount == 24 || m_ui32ChannelCount == 30)
	{	
		for(uint32 j = 0; j < m_ui32ChannelCount; j++)
		{
			m_pSample[j]=(float32)m_vChannelBuffer2[j];
		}
	}
	else
	{
		// channel count is 27 so we need to skip counter since it's 25th
		for(uint32 j = 0; j < 24; j++)
		{
			m_pSample[j]=(float32)m_vChannelBuffer2[j];
		}
		
		// gyro channels
		m_pSample[24]=(float32)m_vChannelBuffer2[25];
		m_pSample[25]=(float32)m_vChannelBuffer2[26];
		m_pSample[26]=(float32)m_vChannelBuffer2[27];
	}
			
	if (sample_number % 5000 == 0)
	{
		sample_number = 1;
        
		if (m_rDriverContext.getDriftSampleCount() > 2)
		{
			
		}
		else
		{
			sample_number++;
			m_pCallback->setSamples(m_pSample, 1);

			if(!m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount()))
			{
				std::cout << "ERROR while correcting drift." << std::endl;
			}
		}
	}
	else
	{
		sample_number++;
		m_pCallback->setSamples(m_pSample, 1);

		if(!m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount()))
	  	{
	    	std::cout << "ERROR while correcting drift." << std::endl;
	  	}
	}
	m_byteArray.clear();
	m_vChannelBuffer2.clear();
	return true;
}

boolean CDriverMBTSmarting::initTTY(HANDLE* pFileDescriptor, uint32 ui32TTYNumber)
{
	char l_sTTYName[1024];

	*pFileDescriptor = NULL;
	::sprintf(l_sTTYName, "\\\\.\\COM%i", ui32TTYNumber);
	 DCB dcb = {0};
	*pFileDescriptor=::CreateFile(
		(LPCSTR)l_sTTYName,
		GENERIC_READ|GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (*pFileDescriptor == NULL)
		return false;

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
	
	
	dcb.DCBlength = sizeof(dcb);
	dcb.BaudRate  = 921600;
	dcb.ByteSize  = 8;
	dcb.Parity    = NOPARITY;
	dcb.StopBits  = ONESTOPBIT;


	// update flow control settings
	dcb.fOutX = true;
    dcb.fInX = true;
    dcb.fOutxCtsFlow = false;
    dcb.fOutxDsrFlow = false;
    dcb.fDsrSensitivity = false;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
	::SetCommState(*pFileDescriptor, &dcb);
	::SetupComm(*pFileDescriptor, 10000, 10000);
	::EscapeCommFunction(*pFileDescriptor, SETDTR);
	::SetCommMask (*pFileDescriptor, EV_RXCHAR | EV_CTS | EV_DSR | EV_RLSD | EV_RING);
	m_sTTYName = l_sTTYName;

	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 1;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.ReadTotalTimeoutConstant = 1;
	timeouts.WriteTotalTimeoutMultiplier = 1;
	timeouts.WriteTotalTimeoutConstant = 1;
	if (!SetCommTimeouts(m_i32FileDescriptor, &timeouts))
	{
		std::cout<< "Error timeout"<< endl;
	}

	return true;
 }

void CDriverMBTSmarting::closeTTY(HANDLE i32FileDescriptor)
{
	::CloseHandle(i32FileDescriptor);
}

int32 CDriverMBTSmarting::readPacketFromTTY(HANDLE i32FileDescriptor)
{
	uint8  l_ui8ReadBuffer[100];
	uint32 l_ui32BytesProcessed=0;
	int32  l_i32PacketsProcessed=0;
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
			if(m_byteArray.size() > 0)
			{
				m_byteArray.push_back(l_ui8ReadBuffer[0]);
				if(m_byteArray.size() == 83)
				{
					if(m_byteArray[82] == '<')
					{
						if(::ClearCommError(i32FileDescriptor, &l_dwState, &l_oStatus))
						{
						}
						
						parseByteP2(i32FileDescriptor);
						m_byteArray.clear();
						m_receivedSamples++;
					}
					else
					{
						m_failedSamples++;
						m_byteArray.clear();
					}
				}
			}

			if(m_byteArray.size() == 0 && l_ui8ReadBuffer[0] == '>')
			{
				m_byteArray.push_back(l_ui8ReadBuffer[0]);
			}
			
		}
		
	 }


	return l_i32PacketsProcessed;
 }

#endif