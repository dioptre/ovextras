#ifdef TARGET_OS_Windows

#include "ovasCDriverMensiaAcquisition.h"
#include "ovasCConfigurationDriverMensiaAcquisition.h"

#include <toolkit/ovtk_all.h>

#include <windows.h>

#include <cmath>
#include <string>

#include <fstream>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;

static OpenViBE::CString s_sMensiaDLL = "openvibe-driver-mensia-acquisition.dll";
HINSTANCE m_oLibMensiaAcquisition; // Library Handle


template<typename T>
void CDriverMensiaAcquisition::loadDLLfunct(T* functionPointer, const char* functionName)
{
	*functionPointer = (T)::GetProcAddress(m_oLibMensiaAcquisition, functionName);
	if (!*functionPointer)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Load method " << functionName << "\n";
		m_bValid=false;
	}
}

typedef int32 (* MACQ_InitializeAcquisitionDriver) (const char* sDeviceIdentifier, IDriverContext& rDriverContexts);

typedef const char* (* MACQ_GetName) (size_t);

typedef const char* (* MACQ_GetDeviceURL) (size_t);

typedef boolean (* MACQ_Preconfigure) (size_t, const char*);
typedef boolean (* MACQ_Configure) (size_t, const char*);
typedef uint32 (* MACQ_GetSamplingRate) (size_t);
typedef uint32 (* MACQ_GetChannelCount) (size_t);
typedef const char* (* MACQ_GetChannelName) (size_t, size_t);
typedef uint32 (* MACQ_GetExperimentIdentifier) (size_t);
typedef uint32 (* MACQ_SetExperimentIdentifier) (size_t, uint32);
typedef uint32 (* MACQ_GetSubjectAge) (size_t);
typedef uint32 (* MACQ_SetSubjectAge) (size_t, uint32);
typedef uint32 (* MACQ_GetSubjectGender) (size_t);
typedef uint32 (* MACQ_SetSubjectGender) (size_t, uint32);
typedef boolean (* MACQ_Initialize) (size_t, IDriverCallback*, uint32, const char*);
typedef boolean (* MACQ_Start) (size_t);
typedef boolean (* MACQ_Stop) (size_t);
typedef boolean (* MACQ_Uninitialize) (size_t);
typedef boolean (* MACQ_Loop) (size_t);

MACQ_InitializeAcquisitionDriver m_fpInitializeAcquisitionDriver;
MACQ_GetName m_fpGetName;
MACQ_GetDeviceURL m_fpGetDeviceURL;

MACQ_Preconfigure m_fpPreconfigure;
MACQ_Configure m_fpConfigure;
MACQ_GetSamplingRate m_fpGetSamplingRate;
MACQ_GetChannelCount m_fpGetChannelCount;
MACQ_GetChannelName m_fpGetChannelName;
MACQ_GetExperimentIdentifier m_fpGetExperimentIdentifier;
MACQ_SetExperimentIdentifier m_fpSetExperimentIdentifier;
MACQ_GetSubjectAge m_fpGetSubjectAge;
MACQ_SetSubjectAge m_fpSetSubjectAge;
MACQ_GetSubjectGender m_fpGetSubjectGender;
MACQ_SetSubjectGender m_fpSetSubjectGender;
MACQ_Initialize m_fpInitialize;
MACQ_Start m_fpStart;
MACQ_Stop m_fpStop;
MACQ_Uninitialize m_fpUninitialize;
MACQ_Loop m_fpLoop;


CDriverMensiaAcquisition::CDriverMensiaAcquisition(IDriverContext& rDriverContext, const char* sDriverIdentifier)
	:IDriver(rDriverContext)
    // This hax only works because m_oSettings does creates a copy of the string
    ,m_oSettings(std::string( std::string("AcquisitionServer_Driver_MensiaAcquisition_") + sDriverIdentifier).c_str(), m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_pSample(NULL)
	,m_ui32TotalSampleCount(0)
	,m_ui64StartTime(0)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverMensiaAcquisition::CDriverMensiaAcquisition\n";
	m_bValid = true;

	// Load the Mensia Acquisition Library
	OpenViBE::CString l_sPath = m_rDriverContext.getConfigurationManager().expand("${Path_Bin}") + "/" + s_sMensiaDLL;
	if(!std::ifstream(l_sPath.toASCIIString()).is_open())
	{
		m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverMensiaAcquisition::CDriverMensiaAcquisition: " <<
			" dll file [" << l_sPath.toASCIIString() <<"] not openable, perhaps it was not installed.\n";
		m_bValid = false;
		return;
	}

	m_oLibMensiaAcquisition = LoadLibrary(l_sPath);
	if(!m_oLibMensiaAcquisition)
	{
		m_rDriverContext.getLogManager() << "CDriverMensiaAcquisition::CDriverMensiaAcquisition: LoadLibrary failed to load: [" << 
			l_sPath.toASCIIString() <<"] with error [" << static_cast<uint64>(GetLastError()) << "]\n";
	}

	loadDLLfunct<MACQ_InitializeAcquisitionDriver>(&m_fpInitializeAcquisitionDriver, "initializeAcquisitionDriver");
	loadDLLfunct<MACQ_GetName>(&m_fpGetName, "getName");
	loadDLLfunct<MACQ_GetDeviceURL>(&m_fpGetDeviceURL, "getDeviceURL");
	loadDLLfunct<MACQ_Preconfigure>(&m_fpPreconfigure, "preconfigure");
	loadDLLfunct<MACQ_Configure>(&m_fpConfigure, "configure");
	loadDLLfunct<MACQ_GetSamplingRate>(&m_fpGetSamplingRate, "getSamplingRate");
	loadDLLfunct<MACQ_GetChannelCount>(&m_fpGetChannelCount, "getChannelCount");
	loadDLLfunct<MACQ_GetChannelName>(&m_fpGetChannelName, "getChannelName");
	loadDLLfunct<MACQ_GetExperimentIdentifier>(&m_fpGetExperimentIdentifier, "getExperimentIdentifier");
	loadDLLfunct<MACQ_SetExperimentIdentifier>(&m_fpSetExperimentIdentifier, "setExperimentIdentifier");
	loadDLLfunct<MACQ_GetSubjectAge>(&m_fpGetSubjectAge, "getSubjectAge");
	loadDLLfunct<MACQ_SetSubjectAge>(&m_fpSetSubjectAge, "setSubjectAge");
	loadDLLfunct<MACQ_GetSubjectGender>(&m_fpGetSubjectGender, "getSubjectGender");
	loadDLLfunct<MACQ_SetSubjectGender>(&m_fpSetSubjectGender, "setSubjectGender");
	loadDLLfunct<MACQ_Initialize>(&m_fpInitialize, "initialize");
	loadDLLfunct<MACQ_Start>(&m_fpStart, "start");
	loadDLLfunct<MACQ_Stop>(&m_fpStop, "stop");
	loadDLLfunct<MACQ_Uninitialize>(&m_fpUninitialize, "uninitialize");
	loadDLLfunct<MACQ_Loop>(&m_fpLoop, "loop");

	if (!m_bValid)
	{
		m_rDriverContext.getLogManager() << LogLevel_Warning << "Could not initialize Mensia Acqsuisition Driver driver\n";
		return;
	}

	// prepare the device library

	int32 l_i32DriverId = m_fpInitializeAcquisitionDriver(sDriverIdentifier, rDriverContext);

	// Negative value is considered an error
	if (l_i32DriverId < 0)
	{
		m_rDriverContext.getLogManager() << LogLevel_Warning << "Could not initialize Mensia Acqsuisition Driver driver\n";
		return;
	}
	m_ui32DriverId = static_cast<uint32>(l_i32DriverId);


	m_rDriverContext.getLogManager() << LogLevel_Trace << "Initialized Mensia Acquisition Driver on id [" << m_ui32DriverId << "]" << "\n";

	//TODO_JL Set sampling frenquency and channel count in header?
	m_oHeader.setSamplingFrequency(128);
	m_oHeader.setChannelCount(0);
	m_oHeader.setExperimentIdentifier(0);
	m_oHeader.setSubjectAge(0);
	m_oHeader.setSubjectGender(0);

	m_oSettings.add("Header", &m_oHeader);
	m_oSettings.add("DeviceURL", &m_sDeviceURL);
	//TODO_JL Add the URL to settings so it can be saved (we should be able to expose it)
	m_oSettings.load();
	m_fpSetExperimentIdentifier(m_ui32DriverId, m_oHeader.getExperimentIdentifier());
	m_fpSetSubjectAge(m_ui32DriverId, m_oHeader.getSubjectAge());
	m_fpSetSubjectGender(m_ui32DriverId, m_oHeader.getSubjectGender());

	m_fpPreconfigure(m_ui32DriverId, m_sDeviceURL.toASCIIString());

	m_oHeader.setChannelCount(m_fpGetChannelCount(m_ui32DriverId));
	m_oHeader.setSamplingFrequency(m_fpGetSamplingRate(m_ui32DriverId));

	for (size_t l_uiChannelIndex = 0; l_uiChannelIndex < m_oHeader.getChannelCount(); l_uiChannelIndex++)
	{
		m_oHeader.setChannelName(l_uiChannelIndex, m_fpGetChannelName(m_ui32DriverId, l_uiChannelIndex));
	}
}

void CDriverMensiaAcquisition::release(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverMensiaAcquisition::release\n";

	delete this;
}

const char* CDriverMensiaAcquisition::getName(void)
{
	return m_fpGetName(m_ui32DriverId);
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverMensiaAcquisition::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverMensiaAcquisition::initialize\n";

	if(m_rDriverContext.isConnected()) { return false; }

	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;
	m_pCallback=&rCallback;

	if (!m_fpInitialize(m_ui32DriverId, m_pCallback, m_ui32SampleCountPerSentBlock, m_sDeviceURL.toASCIIString()))
	{
		return false;
	}

	m_fpSetExperimentIdentifier(m_ui32DriverId, m_oHeader.getExperimentIdentifier());
	m_fpSetSubjectAge(m_ui32DriverId, m_oHeader.getSubjectAge());
	m_fpSetSubjectGender(m_ui32DriverId, m_oHeader.getSubjectGender());

	m_oHeader.setSamplingFrequency(m_fpGetSamplingRate(m_ui32DriverId));
	m_oHeader.setChannelCount(m_fpGetChannelCount(m_ui32DriverId));
	m_oHeader.setExperimentIdentifier(m_fpGetExperimentIdentifier(m_ui32DriverId));
	m_oHeader.setSubjectAge(m_fpGetSubjectAge(m_ui32DriverId));
	m_oHeader.setSubjectGender(m_fpGetSubjectGender(m_ui32DriverId));


	for (size_t uiChannelIndex = 0; uiChannelIndex < m_oHeader.getChannelCount(); uiChannelIndex++)
	{
		m_oHeader.setChannelName(uiChannelIndex, m_fpGetChannelName(m_ui32DriverId, uiChannelIndex));
		m_oHeader.setChannelUnits(uiChannelIndex, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);
	}

	return true;
}

boolean CDriverMensiaAcquisition::start(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverMensiaAcquisition::start\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	if (!m_fpStart(m_ui32DriverId))
	{
		return false;
	}

	return true;
}

boolean CDriverMensiaAcquisition::loop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Debug << "CDriverMensiaAcquisition::loop\n";

	if(!m_rDriverContext.isConnected()) { return false; }

	if(m_rDriverContext.isStarted())
	{
		if (!m_fpLoop(m_ui32DriverId))
		{
			return false;
		}
	}
	else
	{
		if (!m_fpLoop(m_ui32DriverId))
		{
			return false;
		}
		// TODO_JL impedance check here
/*		if(m_rDriverContext.isImpedanceCheckRequested())
		{
			for(uint32 j=0; j<m_oHeader.getChannelCount(); j++)
			{
				m_rDriverContext.updateImpedance(j, 1);
			}
		}
		*/
	}

	return true;
}

boolean CDriverMensiaAcquisition::stop(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverMensiaAcquisition::stop\n";


	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return false; }

	return m_fpStop(m_ui32DriverId);
}

boolean CDriverMensiaAcquisition::uninitialize(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverMensiaAcquisition::uninitialize\n";

	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	m_fpUninitialize(m_ui32DriverId);

	delete [] m_pSample;
	m_pSample=NULL;
	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverMensiaAcquisition::isConfigurable(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverMensiaAcquisition::isConfigurable\n";

	return true;
}

boolean CDriverMensiaAcquisition::configure(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverMensiaAcquisition::configure\n";
	m_fpSetExperimentIdentifier(m_ui32DriverId, m_oHeader.getExperimentIdentifier());
	m_fpSetSubjectAge(m_ui32DriverId, m_oHeader.getSubjectAge());
	m_fpSetSubjectGender(m_ui32DriverId, m_oHeader.getSubjectGender());

	/*CConfigurationDriverMensiaAcquisition m_oConfiguration(m_rDriverContext, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-Generic-Oscillator.ui" );
*/
	if(m_fpConfigure(m_ui32DriverId, m_sDeviceURL.toASCIIString()))
	{

		m_sDeviceURL = CString(m_fpGetDeviceURL(m_ui32DriverId));

		// We need to escape the URL before we save the setting to the file because
		// of the way OpenViBE handles strings
		CString m_sDeviceTempURL = m_sDeviceURL;


		std::string m_sEscapedURL = m_sDeviceURL.toASCIIString();
		size_t pos = 0;
		while((pos = m_sEscapedURL.find("{", pos)) != std::string::npos)
		{
			m_sEscapedURL.replace(pos, 1, "\\{");
			pos += 2;
		}

		pos = 0;
		while((pos = m_sEscapedURL.find("}", pos)) != std::string::npos)
		{
			m_sEscapedURL.replace(pos, 1, "\\}");
			pos += 2;
		}

		pos = 0;
		while((pos = m_sEscapedURL.find("$", pos)) != std::string::npos)
		{
			m_sEscapedURL.replace(pos, 1, "\\$");
			pos += 2;
		}

		m_sDeviceURL = CString(m_sEscapedURL.c_str());
		m_oHeader.setSamplingFrequency(m_fpGetSamplingRate(m_ui32DriverId));
		m_oHeader.setChannelCount(m_fpGetChannelCount(m_ui32DriverId));
		m_oHeader.setExperimentIdentifier(m_fpGetExperimentIdentifier(m_ui32DriverId));
		m_oHeader.setSubjectAge(m_fpGetSubjectAge(m_ui32DriverId));
		m_oHeader.setSubjectGender(m_fpGetSubjectGender(m_ui32DriverId));
		/*
		m_fpSetExperimentIdentifier(m_ui32DriverId, m_oHeader.getExperimentIdentifier());
		m_fpSetSubjectAge(m_ui32DriverId, m_oHeader.getSubjectAge());
		m_fpSetSubjectGender(m_ui32DriverId, m_oHeader.getSubjectGender());
*/



		/*
		for (size_t l_uiChannelIndex = 0; l_uiChannelIndex < m_oHeader.getChannelCount(); l_uiChannelIndex++)
		{
			m_oHeader.setChannelName(l_uiChannelIndex, "");
		}
		*/

		m_oSettings.save();

		m_sDeviceURL = m_sDeviceTempURL;



		return true;
	}


	return false;
}

#endif // TARGET_OS_Windows
