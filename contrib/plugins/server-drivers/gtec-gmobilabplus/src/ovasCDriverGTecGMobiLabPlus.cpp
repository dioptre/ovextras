/**
 * The gMobilab Linux driver was contributed by Lucie Daubigney from Supelec Metz
 *
 * Windows compatibility + gusbamp coexistence added by Jussi T. Lindgren / Inria
 *
 */

#include "ovasCDriverGTecGMobiLabPlus.h"
#include "ovasCConfigurationGTecGMobiLabPlus.h"
#include "../ovasCConfigurationBuilder.h"

#if defined TARGET_HAS_ThirdPartyGMobiLabPlusAPI

#include "ovasCDriverGTecGMobiLabPlusPrivate.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>

#if defined(TARGET_OS_Linux)
#include <dlfcn.h>
#endif

#define boolean OpenViBE::boolean


using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

static const uint32 g_ui32AcquiredChannelCount=8;

//___________________________________________________________________//
//                                                                   //

//constructor
CDriverGTecGMobiLabPlus::CDriverGTecGMobiLabPlus(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_GTecMobiLabPlus", m_rDriverContext.getConfigurationManager())
	,m_pHeader(NULL)
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_pSample(NULL)
	,m_bTestMode(false)
	,m_pLibrary(NULL)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGTecGMobiLabPlus::CDriverGTecGMobiLabPlus\n";

	m_pGtec = new OpenViBEAcquisitionServer::CDriverGTecGMobiLabPlusPrivate();

	m_pHeader = new CHeader();
	m_pHeader->setSamplingFrequency(256);
	m_pHeader->setChannelCount(8);

	m_pGtec->m_oBuffer.pBuffer = NULL;
	m_pGtec->m_oBuffer.size = 0;
	m_pGtec->m_oBuffer.validPoints = 0;
#if defined(TARGET_OS_Windows)
	m_oPortName="//./COM1";
#else
	m_oPortName="/dev/rfcomm0";
#endif

	//initialisation of the analog channels of the gTec module : by default no analog exchange are allowed
	m_pGtec->m_oAnalogIn.ain1 = false;
	m_pGtec->m_oAnalogIn.ain2 = false;
	m_pGtec->m_oAnalogIn.ain3 = false;
	m_pGtec->m_oAnalogIn.ain4 = false;
	m_pGtec->m_oAnalogIn.ain5 = false;
	m_pGtec->m_oAnalogIn.ain6 = false;
	m_pGtec->m_oAnalogIn.ain7 = false;
	m_pGtec->m_oAnalogIn.ain8 = false;

	m_oSettings.add("Header", m_pHeader);
	m_oSettings.add("PortName", &m_oPortName);
	m_oSettings.add("TestMode", &m_bTestMode);
	m_oSettings.load();
}

CDriverGTecGMobiLabPlus::~CDriverGTecGMobiLabPlus(void)
{
	delete m_pHeader;
	delete m_pGtec;
}

void CDriverGTecGMobiLabPlus::release(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGTecGMobiLabPlus::release\n";

	delete this;
}

const char* CDriverGTecGMobiLabPlus::getName(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGTecGMobiLabPlus::getName\n";
	return "g.Tec gMOBIlab+";
}

//___________________________________________________________________//
//                                                                   //

/*
 * configuration
 */

boolean CDriverGTecGMobiLabPlus::isConfigurable(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGTecGMobiLabPlus::isConfigurable\n";
	return true;
}

boolean CDriverGTecGMobiLabPlus::configure(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGTecGMobiLabPlus::configure\n";

	// We use CConfigurationGTecMobilabPlus configuration which is a class that inheritate from the CConfigurationBuilder class
	// The difference between these two classes is the addition of a member of class. This member allows to change the port where is connected the device.
	CConfigurationGTecGMobiLabPlus m_oConfiguration(OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-GTec-GMobiLabPlus.ui",
		m_oPortName, m_bTestMode);

	// We configure the Header with it...
	if(!m_oConfiguration.configure(*m_pHeader))
	{
		return false;
	}

	if(m_pHeader->getChannelCount()>g_ui32AcquiredChannelCount) 
	{
		m_pHeader->setChannelCount(g_ui32AcquiredChannelCount);
	}

	m_oSettings.save();

	m_rDriverContext.getLogManager() << LogLevel_Debug << "Port name after configuration " << CString(m_oPortName.c_str()) << " \n";
	return true;
}

//___________________________________________________________________//
//                                                                   //

#if defined(TARGET_OS_Linux)
#define GetProcAddress dlsym
#define FreeLibrary dlclose
#endif

boolean CDriverGTecGMobiLabPlus::registerLibraryFunctions(void)
{
	// Lets open the DLL
#if defined(TARGET_OS_Windows)
	m_pLibrary = LoadLibrary("gMOBIlabplus.dll");
#else
	m_pLibrary = dlopen("libgmobilabplusapi.so", RTLD_LAZY);
#endif
	if (!m_pLibrary)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "CDriverGTecGMobiLabPlus:: Unable to open gMOBIlabplus.dll\n";
		return false;
	}

	m_pGtec->m_fOpenDevice = (CDriverGTecGMobiLabPlusPrivate::OV_GT_OpenDevice)GetProcAddress(m_pLibrary, "GT_OpenDevice");
	m_pGtec->m_fCloseDevice = (CDriverGTecGMobiLabPlusPrivate::OV_GT_CloseDevice)GetProcAddress(m_pLibrary, "GT_CloseDevice");
	m_pGtec->m_fSetTestmode = (CDriverGTecGMobiLabPlusPrivate::OV_GT_SetTestmode)GetProcAddress(m_pLibrary, "GT_SetTestmode");
	m_pGtec->m_fStartAcquisition = (CDriverGTecGMobiLabPlusPrivate::OV_GT_StartAcquisition)GetProcAddress(m_pLibrary, "GT_StartAcquisition");
	m_pGtec->m_fGetData = (CDriverGTecGMobiLabPlusPrivate::OV_GT_GetData)GetProcAddress(m_pLibrary, "GT_GetData");
	m_pGtec->m_fInitChannels = (CDriverGTecGMobiLabPlusPrivate::OV_GT_InitChannels)GetProcAddress(m_pLibrary, "GT_InitChannels");
	m_pGtec->m_fStopAcquisition = (CDriverGTecGMobiLabPlusPrivate::OV_GT_StopAcquisition)GetProcAddress(m_pLibrary, "GT_StopAcquisition");
	m_pGtec->m_fGetLastError = (CDriverGTecGMobiLabPlusPrivate::OV_GT_GetLastError)GetProcAddress(m_pLibrary, "GT_GetLastError");
	m_pGtec->m_fTranslateErrorCode = (CDriverGTecGMobiLabPlusPrivate::OV_GT_TranslateErrorCode)GetProcAddress(m_pLibrary, "GT_TranslateErrorCode");
	
	if (!m_pGtec->m_fOpenDevice || !m_pGtec->m_fCloseDevice || !m_pGtec->m_fSetTestmode 
		|| !m_pGtec->m_fStartAcquisition || !m_pGtec->m_fGetData || !m_pGtec->m_fInitChannels 
		|| !m_pGtec->m_fStopAcquisition || !m_pGtec->m_fGetLastError || !m_pGtec->m_fTranslateErrorCode)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "CDriverGTecGMobiLabPlus:: Unable to find all the required functions from the gMOBIlabplus.dll\n";
		return false;
	}

	return true;
}


/*
 * initialisation
 */
boolean CDriverGTecGMobiLabPlus::initialize(const uint32 ui32SampleCountPerSentBlock, IDriverCallback& rCallback)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGTecGMobiLabPlus::initialize\n";
	m_rDriverContext.getLogManager() << LogLevel_Debug << "Port name after initialisation " << CString(m_oPortName.c_str()) << "\n";

	if(m_rDriverContext.isConnected())
	{
		return false;
	}

	if(!m_pHeader->isChannelCountSet() || !m_pHeader->isSamplingFrequencySet())
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Either channel count or sampling frequency is not set\n";
		return false;
	}

	if (!registerLibraryFunctions())
	{
		return false;
	}

	uint32 l_ui32ChannelCount = m_pHeader->getChannelCount();

	// analog exchanges allowed on the first "l_ui32CHannelCount" channels:
	for(uint32 i=1; i<=l_ui32ChannelCount; i++)
	{
		CDriverGTecGMobiLabPlus::allowAnalogInputs(i);
	}

	// then buffer of type _BUFFER_ST built to store acquired samples.
	m_pGtec->m_oBuffer.pBuffer=new short int[l_ui32ChannelCount];//allocate enough space for the buffer m_oBuffer.pBuffer ; only one set of mesures is acquired (channel 1 to 8) in a row
	m_pGtec->m_oBuffer.size=l_ui32ChannelCount*sizeof(short int);
	m_pGtec->m_oBuffer.validPoints=0;

#if defined(TARGET_OS_Windows)
	m_pGtec->m_oOverlap.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pGtec->m_oOverlap.Offset = 0;
	m_pGtec->m_oOverlap.OffsetHigh = 0;
#endif

	// allocates enough space for m_pSample
	m_pSample=new float32[ui32SampleCountPerSentBlock*l_ui32ChannelCount];

	// if there is a problem while creating the two arrays
	if(!m_pGtec->m_oBuffer.pBuffer || !m_pSample)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Memory allocation problem\n";
		delete [] m_pGtec->m_oBuffer.pBuffer;
		delete [] m_pSample;
		m_pSample=NULL;
		m_pGtec->m_oBuffer.pBuffer=NULL;
		return false;
	}

	// initializes hardware and get
	// available header information
	// from it
#if defined(TARGET_OS_Windows)
	m_pGtec->m_oDevice = m_pGtec->m_fOpenDevice((LPSTR)m_oPortName.c_str());
#else
	m_pGtec->m_oDevice = m_pGtec->m_fOpenDevice(m_oPortName.c_str());
#endif
	if(m_pGtec->m_oDevice==0)
	{
#if defined(TARGET_OS_Windows)
		UINT l_uErrorCode = 0;
#else
		unsigned int l_uErrorCode = 0;
#endif
		_ERRSTR l_sErrorString;
		m_pGtec->m_fGetLastError(&l_uErrorCode);
		m_pGtec->m_fTranslateErrorCode(&l_sErrorString, l_uErrorCode);

		m_rDriverContext.getLogManager() << LogLevel_Error << "Unable to connect to [" << m_oPortName.c_str() << "], error code " << l_uErrorCode << ": '" << l_sErrorString.Error << "'\n";
		delete [] m_pGtec->m_oBuffer.pBuffer;
		delete [] m_pSample;
		return false;
	}

	// saves parameters
	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;
	return true;
}

boolean CDriverGTecGMobiLabPlus::uninitialize(void)
{
	boolean l_bOk = true;

	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGTecGMobiLabPlus::uninitialize\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	// uninitializes hardware here
	if (!m_pGtec->m_fCloseDevice(m_pGtec->m_oDevice))
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "GT_CloseDevice() failed\n";
		l_bOk = false;
	}

	// frees memory
	delete [] m_pSample;
	delete [] m_pGtec->m_oBuffer.pBuffer;

	m_pSample=NULL;
	m_pGtec->m_oBuffer.pBuffer=NULL;
	m_pCallback=NULL;

	// uninitialisation of the analog channels : set valus to default ones
	m_pGtec->m_oAnalogIn.ain1 = false;
	m_pGtec->m_oAnalogIn.ain2 = false;
	m_pGtec->m_oAnalogIn.ain3 = false;
	m_pGtec->m_oAnalogIn.ain4 = false;
	m_pGtec->m_oAnalogIn.ain5 = false;
	m_pGtec->m_oAnalogIn.ain6 = false;
	m_pGtec->m_oAnalogIn.ain7 = false;
	m_pGtec->m_oAnalogIn.ain8 = false;
	
	if (m_pLibrary)
	{
		FreeLibrary(m_pLibrary);
		m_pLibrary = NULL;
	}

	return l_bOk;
}

const IHeader* CDriverGTecGMobiLabPlus::getHeader(void)
{
	return m_pHeader;
}

//___________________________________________________________________//
//                                                                   //

/*
 * acquisition
 */

boolean CDriverGTecGMobiLabPlus::start(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGTecGMobiLabPlus::start\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	// we use none of the digital inputs/outputs
	_DIO l_oDigitalInOut;
	l_oDigitalInOut.dio1_enable = false;
	l_oDigitalInOut.dio2_enable = false;
	l_oDigitalInOut.dio3_enable = false;
	l_oDigitalInOut.dio4_enable = false;
	l_oDigitalInOut.dio5_enable = false;
	l_oDigitalInOut.dio6_enable = false;
	l_oDigitalInOut.dio7_enable = false;
	l_oDigitalInOut.dio8_enable = false;

	// channel initialisation
	if(!m_pGtec->m_fInitChannels(m_pGtec->m_oDevice, m_pGtec->m_oAnalogIn, l_oDigitalInOut))
	{
		m_rDriverContext.getLogManager() << LogLevel_Trace << "GT_InitChannels failed\n";
		return false;
	}

	// are we interested in test signal?
	m_pGtec->m_fSetTestmode(m_pGtec->m_oDevice, m_bTestMode);

	// requests hardware to start sending data
	if(!m_pGtec->m_fStartAcquisition(m_pGtec->m_oDevice))
	{
		m_rDriverContext.getLogManager() << LogLevel_Trace << "GT_StartAcquisition failed\n";
		return false;
	}
	return true;
}

boolean CDriverGTecGMobiLabPlus::loop(void)
{
	// m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGTecGMobiLabPlus::loop\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return true; }

	uint32 i, j;
	uint32 l_ui32ChannelCount=m_pHeader->getChannelCount();

	// only "l-ui32ChannelCount" measures corresponding to one measure per channel are acquired in a row with the function GT_GetData()
	// these measures are stored in m_oBuffer.pBuffer[]
	// the acquisition is reapeted m_ui32SampleCountPerSendBlock times to fill in the array "m_pSample"
	for(i=0 ; i<m_ui32SampleCountPerSentBlock ; i++)
	{
#if defined(TARGET_OS_Windows)
		if (!m_pGtec->m_fGetData(m_pGtec->m_oDevice, &m_pGtec->m_oBuffer, &m_pGtec->m_oOverlap))// receive samples from hardware (one per channel)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "GT_GetData failed\n";
			return false;
		}
		if (WaitForSingleObject(m_pGtec->m_oOverlap.hEvent, 1000) == WAIT_TIMEOUT)
		{
			m_rDriverContext.getLogManager() << LogLevel_Warning << "Timeout in reading from the device\n";
			return false;
		}
#else
		if (!m_pGtec->m_fGetData(m_pGtec->m_oDevice, &m_pGtec->m_oBuffer))// receive samples from hardware (one per channel)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "GT_GetData failed\n";
			return false;
		}
#endif

		// here the "l_ui32ChannelCount" measures just acquired are stored in m_pSample not to be deleted by the next acquisition
		// m_rDriverContext.getLogManager() << LogLevel_Debug << "Here are the " << l_ui32ChannelCount << " measures of the " << i << " th sample\n" << LogLevel_Debug;
		for(j=0; j<l_ui32ChannelCount; j++)
		{
			// m_rDriverContext.getLogManager() << (m_oBuffer.pBuffer[j]*0.5)/32768. << " ";
			//operation made to modify the short int in a number between 0 and 500mV (in Volt)
			m_pSample[m_ui32SampleCountPerSentBlock*j+i] = static_cast<float32>((m_pGtec->m_oBuffer.pBuffer[j]*0.5)/32768.);
		}
		// m_rDriverContext.getLogManager() << "\n";
	}

	// the buffer is full : it is send to the acquisition server
	m_pCallback->setSamples(m_pSample,m_ui32SampleCountPerSentBlock);
	m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());

	return true;
}

boolean CDriverGTecGMobiLabPlus::stop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGTecGMobiLabPlus::stop\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return false; }

	// requests the hardware to stop sending data
	if(!m_pGtec->m_fStopAcquisition(m_pGtec->m_oDevice))
	{
		m_rDriverContext.getLogManager() << LogLevel_Trace << "GT_StopAcquisition failed\n";
		return false;
	}

	return true;
}

// this function allows exchanges of data on the "ui32ChannelIndex" channel
// function used to initialize the analog inputs according to the number "channelCount"
void CDriverGTecGMobiLabPlus::allowAnalogInputs(uint32 ui32ChannelIndex)
{
	switch(ui32ChannelIndex)
	{
		case 8: m_pGtec->m_oAnalogIn.ain8 = true; break;
		case 7: m_pGtec->m_oAnalogIn.ain7 = true; break;
		case 6: m_pGtec->m_oAnalogIn.ain6 = true; break;
		case 5: m_pGtec->m_oAnalogIn.ain5 = true; break;
		case 4: m_pGtec->m_oAnalogIn.ain4 = true; break;
		case 3: m_pGtec->m_oAnalogIn.ain3 = true; break;
		case 2: m_pGtec->m_oAnalogIn.ain2 = true; break;
		case 1: m_pGtec->m_oAnalogIn.ain1 = true; break;
		default:
			m_rDriverContext.getLogManager() << LogLevel_Trace << "Unexpected value " << ui32ChannelIndex << " in CDriverGTecGMobiLabPlus::allowAnalogInputs\n";
			break;
	}
}

#endif // TARGET_HAS_ThirdPartyGMobiLabPlusAPI
