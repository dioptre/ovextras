#if defined TARGET_HAS_ThirdPartyEmotivAPI

#include "ovasCDriverEmotivEPOC.h"
#include "ovasCConfigurationEmotivEPOC.h"

#include <system/ovCTime.h>

#include <system/ovCMemory.h>

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <vector>

#if defined(TARGET_OS_Windows)
#include <delayimp.h>
#endif

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

#define boolean OpenViBE::boolean

#if defined(TARGET_HAS_ThirdPartyEmotivResearchAPI3x)
static const IEE_DataChannel_t g_ChannelList[] = 
{
	IED_AF3, IED_F7, IED_F3, IED_FC5, IED_T7, IED_P7, IED_O1, IED_O2, IED_P8, IED_T8, IED_FC6, IED_F4, IED_F8, IED_AF4, 
	IED_GYROX, IED_GYROY,
	IED_COUNTER,
	IED_TIMESTAMP, 
	IED_FUNC_ID, IED_FUNC_VALUE, 
	IED_MARKER, 
	IED_SYNC_SIGNAL
};
#else
// Old API
static const EE_DataChannel_t g_ChannelList[] = 
{
	ED_AF3, ED_F7, ED_F3, ED_FC5, ED_T7, ED_P7, ED_O1, ED_O2, ED_P8, ED_T8, ED_FC6, ED_F4, ED_F8, ED_AF4, 
	ED_GYROX, ED_GYROY,
	ED_COUNTER,
	ED_TIMESTAMP, 
	ED_FUNC_ID, ED_FUNC_VALUE, 
	ED_MARKER, 
	ED_SYNC_SIGNAL
};
#endif

CDriverEmotivEPOC::CDriverEmotivEPOC(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_EmotivEPOC", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_ui32TotalSampleCount(0)
	,m_pSample(NULL)
{
	m_bUseGyroscope = false;
	m_sPathToEmotivSDK = "";
	m_sCommandForPathModification = "";

	m_ui32UserID = 0;
	m_bReadyToCollect = false;
	m_bFirstStart = true;

	m_oSettings.add("Header", &m_oHeader);
	m_oSettings.add("UseGyroscope", &m_bUseGyroscope);
	m_oSettings.add("PathToEmotivSDK", &m_sPathToEmotivSDK);
	m_oSettings.add("UserID", &m_ui32UserID);
	m_oSettings.load();
}

CDriverEmotivEPOC::~CDriverEmotivEPOC(void)
{
}

const char* CDriverEmotivEPOC::getName(void)
{
	return "Emotiv EPOC";
}

boolean CDriverEmotivEPOC::restoreState(void)
{
	// Restore the previous path
	if(m_sOldPath.length() > 0) {
		_putenv_s("PATH",m_sOldPath);
	}

#if defined TARGET_OS_Windows
	__FUnloadDelayLoadedDLL2("edk.dll");
#endif

	return true;
}

boolean CDriverEmotivEPOC::buildPath(void)
{
	char * l_sPath = getenv("PATH");
	if(l_sPath == NULL)
	{
		return false;
	}
	
	m_sOldPath = l_sPath;

	string l_sStrPath = string(l_sPath);
	size_t l_pFound = l_sStrPath.find((const char *)m_sPathToEmotivSDK);

	if( l_pFound == string::npos )
	{
		// If the emotiv component is not part of the path, we add it.
		m_sCommandForPathModification = l_sPath + CString(";") + m_sPathToEmotivSDK;
		m_rDriverContext.getLogManager() << LogLevel_Trace << "[INIT] Emotiv Driver: Building new Windows PATH.\n";
	} 
	else 
	{
		// If we found the emotiv component in path already, keep the path as-is.
		m_sCommandForPathModification = CString(l_sPath);
		m_rDriverContext.getLogManager() << LogLevel_Trace << "[INIT] Emotiv Driver: Using the existing PATH.\n";
	}



	return true;
}

//___________________________________________________________________//
//                                                                   //
boolean CDriverEmotivEPOC::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	m_tEEEventHandle = NULL;

	//we need to add the path to Emotiv SDK to PATH: done in external function 
	//because SEH (__try/__except) does not allow the use of local variables with destructor.
	if(!this->buildPath())
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Emotiv Driver: Failed to get the ENV variable PATH.\n";
		return false;
	}

#if defined TARGET_OS_Windows
	// m_rDriverContext.getLogManager() << LogLevel_Info << "[INIT] Emotiv Driver: Setting PATH as " << m_sCommandForPathModification << "\n";

	if(_putenv_s("PATH",m_sCommandForPathModification) != 0)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Emotiv Driver: Failed to modify the environment PATH with the Emotiv SDK path.\n";
		return false;
	}
#endif
	
	m_oHeader.setChannelCount(14);
	if(m_bUseGyroscope)
	{
		m_oHeader.setChannelCount(16); // 14 + 2 REF + 2 Gyro 
	}
	
	m_oHeader.setChannelName(0,  "AF3");
	m_oHeader.setChannelName(1,  "F7");
	m_oHeader.setChannelName(2,  "F3");
	m_oHeader.setChannelName(3,  "FC5");
	m_oHeader.setChannelName(4,  "T7");
	m_oHeader.setChannelName(5,  "P7");
	m_oHeader.setChannelName(6,  "O1");
	m_oHeader.setChannelName(7,  "O2");
	m_oHeader.setChannelName(8,  "P8");
	m_oHeader.setChannelName(9,  "T8");
	m_oHeader.setChannelName(10, "FC6");
	m_oHeader.setChannelName(11, "F4");
	m_oHeader.setChannelName(12, "F8");
	m_oHeader.setChannelName(13, "AF4");

	if(m_bUseGyroscope)
	{
		m_oHeader.setChannelName(14, "Gyro-X");
		m_oHeader.setChannelName(15, "Gyro-Y");
	}

	m_oHeader.setSamplingFrequency(128); // let's hope so...

	// Set channel units
	// Various sources (e.g. http://emotiv.com/forum/forum15/topic879/messages/?PAGEN_1=3
	// and http://www.bci2000.org/wiki/index.php/Contributions:Emotiv ) suggested
	// that the units from the device are in microvolts, but with a typical DC offset around 4000. 
	// Hard to find official source.
	for(uint32 c=0;c<14;c++) 
	{
		m_oHeader.setChannelUnits(c, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);
	}
	if(m_bUseGyroscope)
	{
		// Even less sure about the units of these, leaving as unspecified.
		m_oHeader.setChannelUnits(14, OVTK_UNIT_Unspecified, OVTK_FACTOR_Base);
		m_oHeader.setChannelUnits(15, OVTK_UNIT_Unspecified, OVTK_FACTOR_Base);
	}

	m_rDriverContext.getLogManager() << LogLevel_Trace << "INIT called.\n";
	if(m_rDriverContext.isConnected())
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Emotiv Driver: Driver already initialized.\n";
		restoreState();
		return false;
	}

	if(!m_oHeader.isChannelCountSet()
	 ||!m_oHeader.isSamplingFrequencySet())
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Emotiv Driver: Channel count or frequency not set.\n";
		restoreState();
		return false;
	}

	//---------------------------------------------------------
	// Builds up a buffer to store acquired samples. This buffer will be sent to the acquisition server later.

	m_pSample=new float32[m_oHeader.getChannelCount()];
	//m_pBuffer=new float64[m_oHeader.getChannelCount()*ui32SampleCountPerSentBlock];
	if(!m_pSample /*|| !m_pBuffer*/)
	{
		delete [] m_pSample;
		//delete [] m_pBuffer;
		m_pSample=NULL;
		//m_pBuffer=NULL;
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Emotiv Driver: Samples allocation failed.\n";
		restoreState();
		return false;
	}

	//__________________________________
	// Hardware initialization

	m_bReadyToCollect = false;
#if defined TARGET_OS_Windows
	// First call to a function from EDK.DLL: guard the call with __try/__except clauses.
	__try
#elif defined TARGET_OS_Linux
	try
#endif
	{
		m_tEEEventHandle = IEE_EmoEngineEventCreate();
		m_ui32EDK_LastErrorCode = IEE_EngineConnect();

	}
#if defined TARGET_OS_Windows
	__except(EXCEPTION_EXECUTE_HANDLER)
#elif defined TARGET_OS_Linux
	catch(...)
#endif
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Emotiv Driver: First call to 'edk.dll' failed.\n"
			<< "\tThis driver needs Emotiv SDK Research Edition (or better)\n"
			<< "\tinstalled on your computer.\n"
			<< "\tYou have configured the driver to use path [" << m_sPathToEmotivSDK << "].\n"
			<< "\tIf there is an 'edk.dll' there, its version may be incompatible.\n"
			<< "\tThis driver was built against 32-bit (x86) Emotiv SDK v "
#if defined(TARGET_HAS_ThirdPartyEmotivResearchAPI3x)
			<< "3.x.x.\n";
#else
			<< "1.x.x.\n";
#endif

		restoreState();
		return false;
	}

	if (m_ui32EDK_LastErrorCode != EDK_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Emotiv Driver: Can't connect to EmoEngine. EDK Error Code [" << m_ui32EDK_LastErrorCode << "]\n";
		restoreState();
		return false;
	}
	else
	{
		m_rDriverContext.getLogManager() << LogLevel_Trace << "[INIT] Emotiv Driver: Connection to EmoEngine successful.\n";
	}

#if defined(TARGET_HAS_ThirdPartyEmotivResearchAPI)
	unsigned long l_ulHwVersion = 0;
	unsigned long l_ulBuildNum = 0;
	char l_sVersion[16];

	IEE_HardwareGetVersion(m_ui32UserID, &l_ulHwVersion);
	IEE_SoftwareGetVersion(l_sVersion, 16, &l_ulBuildNum);

	m_rDriverContext.getLogManager() << LogLevel_Info << "Emotiv Driver: "
		<< "headset version " << uint64(l_ulHwVersion && 0xFFFF) << " / "  << uint64(l_ulHwVersion >> 16) 
		<< ", software " << l_sVersion << ", build " << uint64(l_ulBuildNum) << "\n";
#endif

	//__________________________________
	// Saves parameters

	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;

	return true;
}

boolean CDriverEmotivEPOC::start(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "START called.\n";
	if(!m_rDriverContext.isConnected())
	{
		return false;
	}

	if(m_rDriverContext.isStarted())
	{
		return false;
	}

	m_tDataHandle = IEE_DataCreate();
	//float l_fBufferSizeInSeconds = (float32)m_ui32SampleCountPerSentBlock/(float32)m_oHeader.getSamplingFrequency();
	float l_fBufferSizeInSeconds = 1;
	m_ui32EDK_LastErrorCode = IEE_DataSetBufferSizeInSec(l_fBufferSizeInSeconds);
	if (m_ui32EDK_LastErrorCode != EDK_OK) {
		m_rDriverContext.getLogManager() << LogLevel_Error << "[START] Emotiv Driver: Set buffer size to ["<<l_fBufferSizeInSeconds<< "] sec failed. EDK Error Code [" << m_ui32EDK_LastErrorCode << "]\n";
		return false;
	}
	m_rDriverContext.getLogManager() << LogLevel_Trace << "[START] Emotiv Driver: Data Handle created. Buffer size set to ["<<l_fBufferSizeInSeconds<< "] sec.\n";

	return true;
}

boolean CDriverEmotivEPOC::loop(void)
{
	if(!m_rDriverContext.isConnected())
	{
		return false;
	}

	if(m_rDriverContext.isStarted())
	{
		// we enable the acquisiton for every new headset (user) ever added
		if(IEE_EngineGetNextEvent(m_tEEEventHandle) == EDK_OK)
		{
			IEE_Event_t l_tEventType = IEE_EmoEngineEventGetType(m_tEEEventHandle);
			

			if (l_tEventType == IEE_UserAdded)
			{
				uint32 l_ui32NewUserID;
				IEE_EmoEngineEventGetUserId(m_tEEEventHandle, &l_ui32NewUserID);
				m_rDriverContext.getLogManager() << LogLevel_Trace << "[LOOP] Emotiv Driver: User #" << l_ui32NewUserID << " registered.\n";
				m_ui32EDK_LastErrorCode = IEE_DataAcquisitionEnable(l_ui32NewUserID, true);
				if (m_ui32EDK_LastErrorCode != EDK_OK) 
				{
					m_rDriverContext.getLogManager() << LogLevel_Error << "[LOOP] Emotiv Driver: Enabling acquisition failed. EDK Error Code [" << m_ui32EDK_LastErrorCode << "]\n";
					return false;
				}
				// but we are ready to acquire the samples only if the requested headset is detected
				m_bReadyToCollect = m_bReadyToCollect || (m_ui32UserID == l_ui32NewUserID);
			}
		}
		
		if(m_bReadyToCollect)
		{
			uint32 l_ui32nSamplesTaken=0;
			m_ui32EDK_LastErrorCode = IEE_DataUpdateHandle(m_ui32UserID, m_tDataHandle);
			if(m_ui32EDK_LastErrorCode != EDK_OK)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "[LOOP] Emotiv Driver: An error occurred while updating the DataHandle. EDK Error Code [" << m_ui32EDK_LastErrorCode << "]\n";
				return false;
			}
			m_ui32EDK_LastErrorCode = IEE_DataGetNumberOfSample(m_tDataHandle, &l_ui32nSamplesTaken);
			if(m_ui32EDK_LastErrorCode != EDK_OK)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "[LOOP] Emotiv Driver: An error occurred while getting new samples from device. EDK Error Code [" << m_ui32EDK_LastErrorCode << "]\n";
				return false;
			}
			// warning : if you connect/disconnect then reconnect, the internal buffer may be full of samples, thus maybe l_ui32nSamplesTaken > m_ui32SampleCountPerSentBlock
			m_rDriverContext.getLogManager() << LogLevel_Debug << "EMOTIV EPOC >>> received ["<< l_ui32nSamplesTaken <<"] samples per channel from device with user #" << m_ui32UserID <<".\n";
			
			
			/*for(uint32 i=0; i<m_oHeader.getChannelCount(); i++)
			{
				for(uint32 s=0;s<l_ui32nSamplesTaken;s++)
				{
					double* l_pBuffer = new double[l_ui32nSamplesTaken];
					m_ui32EDK_LastErrorCode = IEE_DataGet(m_tDataHandle, g_ChannelList[i], l_pBuffer, l_ui32nSamplesTaken);
					if(m_ui32EDK_LastErrorCode != EDK_OK)
					{
						m_rDriverContext.getLogManager() << LogLevel_Error << "[LOOP] Emotiv Driver: An error occurred while getting new samples from device. EDK Error Code [" << m_ui32EDK_LastErrorCode << "]\n";
						return false;
					}
					
					m_rDriverContext.getLogManager() << LogLevel_Debug << "EMOTIV EPOC >>> adding sample with value ["<< l_pBuffer[s] <<"]\n";
					m_pSample[i*m_ui32SampleCountPerSentBlock + s] = (float32)l_pBuffer[s];
					delete [] l_pBuffer;
				}
				m_pCallback->setSamples(m_pSample);
			}
			m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());
			*/

			double* l_pBuffer = new double[l_ui32nSamplesTaken];
			for(uint32 s=0; s<l_ui32nSamplesTaken; s++)
			{
				for(uint32 i=0; i<m_oHeader.getChannelCount(); i++)
				{
					m_ui32EDK_LastErrorCode = IEE_DataGet(m_tDataHandle, g_ChannelList[i], l_pBuffer, l_ui32nSamplesTaken);
					if(m_ui32EDK_LastErrorCode != EDK_OK)
					{
						m_rDriverContext.getLogManager() << LogLevel_Error << "[LOOP] Emotiv Driver: An error occurred while getting new samples from device. EDK Error Code [" << m_ui32EDK_LastErrorCode << "]\n";
						return false;
					}
					m_pSample[i] = (float32)l_pBuffer[s];
					if(s==0)m_rDriverContext.getLogManager() << LogLevel_Debug << "EMOTIV EPOC >>> sample received has value ["<< l_pBuffer[s] <<"]\n";
					//m_rDriverContext.getLogManager() << LogLevel_Debug << "EMOTIV EPOC >>> sample stored has value ["<< m_pSample[i] <<"]\n";
				}
				m_pCallback->setSamples(m_pSample, 1);
			}
			m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());
			delete [] l_pBuffer;
			
		}
	}

	return true;
}

boolean CDriverEmotivEPOC::stop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "STOP called.\n";
	if(!m_rDriverContext.isConnected())
	{
		return false;
	}

	if(!m_rDriverContext.isStarted())
	{
		return false;
	}

	IEE_DataFree(m_tDataHandle);

	return true;
}

boolean CDriverEmotivEPOC::uninitialize(void)
{
	IEE_EngineDisconnect();

	if(m_tEEEventHandle) {
		IEE_EmoEngineEventFree(m_tEEEventHandle);
		m_tEEEventHandle = NULL;
	}

	restoreState();

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverEmotivEPOC::isConfigurable(void)
{
	return true;
}

boolean CDriverEmotivEPOC::configure(void)
{
	CConfigurationEmotivEPOC m_oConfiguration(m_rDriverContext, 
		OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-Emotiv-EPOC.ui", 
		m_bUseGyroscope, m_sPathToEmotivSDK, m_ui32UserID);

	if(!m_oConfiguration.configure(m_oHeader)) 
	{
		return false;
	}

	m_oSettings.save();

	return true;
}

#endif // TARGET_HAS_ThirdPartyEmotivAPI
