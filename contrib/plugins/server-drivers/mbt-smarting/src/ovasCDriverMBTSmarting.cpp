#include "ovasCDriverMBTSmarting.h"
#include "ovasCConfigurationMBTSmarting.h"

#include <toolkit/ovtk_all.h>

#include <string>
#include <sstream>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

#define MBT_CHANNEL_COUNT 27

//___________________________________________________________________//
//                                                                   //

CDriverMBTSmarting::CDriverMBTSmarting(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_MBTSmarting", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_ui32ConnectionID(1)
	,m_pSmartingAmp(nullptr)
{
	m_oHeader.setSamplingFrequency(500);
	m_oHeader.setChannelCount(MBT_CHANNEL_COUNT);
	
	// The following class allows saving and loading driver settings from the acquisition server .conf file
	m_oSettings.add("Header", &m_oHeader);
	// To save your custom driver settings, register each variable to the SettingsHelper
	m_oSettings.add("ConnectionID", &m_ui32ConnectionID);
	m_oSettings.load();	

}

CDriverMBTSmarting::~CDriverMBTSmarting(void)
{
}

const char* CDriverMBTSmarting::getName(void)
{
	return "mBrainTrain Smarting";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverMBTSmarting::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	if(m_rDriverContext.isConnected()) return false;
	if(!m_oHeader.isChannelCountSet()||!m_oHeader.isSamplingFrequencySet()) return false;
	
	// n.b. We force it to be 27 despite user choice
	m_oHeader.setChannelCount(MBT_CHANNEL_COUNT);

	// ...
	// initialize hardware and get
	// available header information
	// from it
	// Using for example the connection ID provided by the configuration (m_ui32ConnectionID)
	// ...
	
	m_pSmartingAmp = new SmartingAmp();
	
	stringstream port_ss;
	#ifdef TARGET_OS_Windows
		port_ss << "COM" << m_ui32ConnectionID;
	#elif defined TARGET_OS_Linux
		port_ss << "/dev/rfcomm" << m_ui32ConnectionID;
	#endif

	m_rDriverContext.getLogManager() << LogLevel_Info << "Attempting to Connect to Device at : " << port_ss.str().c_str() <<"\n";

	string port(port_ss.str().c_str());
	bool connected = m_pSmartingAmp->connect(port);
	if(connected)
	{
		// set sampling frequency
		switch(m_oHeader.getSamplingFrequency())
		{
		case 250:
			m_pSmartingAmp->send_command(FREQUENCY_250);
			m_rDriverContext.getLogManager() << LogLevel_Info << "Setting the sampling frequency at " << 250 <<"\n";
			break;
		case 500:
			m_rDriverContext.getLogManager() << LogLevel_Info << "Setting the sampling frequency at " << 500 <<"\n";
			m_pSmartingAmp->send_command(FREQUENCY_500);
			break;
		default:
			m_rDriverContext.getLogManager() << LogLevel_Error << "Only sampling frequencies 250 and 500 are supported\n";
			return false;
			break;
		}

		// Declare channel units
		for(uint32 c=0;c<24;c++) 
		{
			m_oHeader.setChannelUnits(c, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);         // signal channels
		}
		m_oHeader.setChannelUnits(24, OVTK_UNIT_Degree_Per_Second, OVTK_FACTOR_Base); // gyroscope outputs
		m_oHeader.setChannelUnits(25, OVTK_UNIT_Degree_Per_Second, OVTK_FACTOR_Base);
		m_oHeader.setChannelUnits(26, OVTK_UNIT_Degree_Per_Second, OVTK_FACTOR_Base);

		m_oHeader.setChannelName(24, "Gyro 1");
		m_oHeader.setChannelName(25, "Gyro 2");
		m_oHeader.setChannelName(26, "Gyro 3");

		// Saves parameters
		m_pCallback=&rCallback;
		m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;

		return true;
	}

	return false;
}

boolean CDriverMBTSmarting::start(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) return false;

	// ...
	// request hardware to start
	// sending data
	// ...

	m_pSmartingAmp->start();

	m_vSamples.resize(MBT_CHANNEL_COUNT * m_ui32SampleCountPerSentBlock);

	return true;
}

boolean CDriverMBTSmarting::loop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return true;

	OpenViBE::CStimulationSet l_oStimulationSet;

	// ...
	// receive samples from hardware
	// put them the correct way in the sample array
	// whether the buffer is full, send it to the acquisition server
	//...

	std::vector<float*> samples;
	while(samples.size() < m_ui32SampleCountPerSentBlock)
	{
		if(!m_pSmartingAmp->get_sample(samples, m_ui32SampleCountPerSentBlock))
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "The Smarting thread returned 0 samples, unexpected.\n";
			return false;
		}
	}

	for(uint32_t s=0;s<m_ui32SampleCountPerSentBlock;s++)
	{
		float* pSample = samples[s];
		for(size_t c=0;c<MBT_CHANNEL_COUNT;c++)
		{
			m_vSamples[c*m_ui32SampleCountPerSentBlock+s] = pSample[c];	
		}
		delete[] pSample;
	}

	m_pCallback->setSamples(&m_vSamples[0]);
	m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());

	// ...
	// receive events from hardware
	// and put them the correct way in a CStimulationSet object
	//...
	m_pCallback->setStimulationSet(l_oStimulationSet);

	return true;
}

boolean CDriverMBTSmarting::stop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return false;

	// ...
	// request the hardware to stop
	// sending data
	// ...
	m_rDriverContext.setInnerLatencySampleCount(0);

	m_pSmartingAmp->stop();

	return true;
}

boolean CDriverMBTSmarting::uninitialize(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) return false;

	// ...
	// uninitialize hardware here
	// ...

	m_pSmartingAmp->disconnect();

	delete m_pSmartingAmp;
	m_pSmartingAmp = nullptr;

	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //
boolean CDriverMBTSmarting::isConfigurable(void)
{
	return true; // change to false if your device is not configurable
}

boolean CDriverMBTSmarting::configure(void)
{
	// Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
	CConfigurationMBTSmarting m_oConfiguration(m_rDriverContext, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-MBTSmarting.ui", m_ui32ConnectionID);
	
	if(!m_oConfiguration.configure(m_oHeader))
	{
		return false;
	}
	m_oSettings.save();
	
	return true;
}
