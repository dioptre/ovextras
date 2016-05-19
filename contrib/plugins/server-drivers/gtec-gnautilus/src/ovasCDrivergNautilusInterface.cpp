
#if defined(TARGET_HAS_ThirdPartyGNEEDaccessAPI)

#include "ovasCDrivergNautilusInterface.h"
#include "ovasCConfigurationgNautilusInterface.h"

#include <toolkit/ovtk_all.h>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

//___________________________________________________________________//
//                                                                   //

// global functions, event handles and variables
void OnDataReadyEventHandler(GDS_HANDLE connection_handle, void *usr_data);
HANDLE g_oDataReadyEventHandle;

CDrivergNautilusInterface::CDrivergNautilusInterface(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_gNautilusInterface", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(4) //default value, has to correspond to nautilus_device_cfg.NumberOfScans!!
	,m_pSample(NULL)
	,m_ui32DeviceIndex(uint32(-1))
	,m_ui32BufferSize(0)
	,m_pBuffer(NULL)
	,m_pDevice(NULL)
	,m_i32InputSource(0)
	,m_ui32NetworkChannel(11)
	,m_i32NotchFilterIndex(-1)
	,m_i32BandPassFilterIndex(-1)
	,m_bDigitalInputEnabled(true)
	,m_bNoiseReductionEnabled(false)
	,m_bCAREnabled(false)
	,m_bAccelerationDataEnabled(true)
	,m_bCounterEnabled(true)
	,m_bLinkQualityEnabled(true)
	,m_bBatteryLevelEnabled(true)
	,m_bValidationIndicatorEnabled(true)
	,m_ui32AcquiredChannelCount(32)
{
	m_oHeader.setSamplingFrequency(250);
	m_oHeader.setChannelCount(32);

	// The following class allows saving and loading driver settings from the acquisition server .conf file
	m_oSettings.add("Header", &m_oHeader);
	// To save your custom driver settings, register each variable to the SettingsHelper
	//m_oSettings.add("SettingName", &variable);
	m_oSettings.add("DeviceIndex", &m_ui32DeviceIndex);
	m_oSettings.add("InputSource", &m_i32InputSource);
	m_oSettings.add("NetworkChannel", &m_ui32NetworkChannel);
	m_oSettings.add("Sensitivity", &m_f64Sensitivity);
	m_oSettings.add("NotchFilterIndex", &m_i32NotchFilterIndex);
	m_oSettings.add("BandPassFilterIndex", &m_i32BandPassFilterIndex);
	m_oSettings.add("DigitalInputEnabled", &m_bDigitalInputEnabled);
	m_oSettings.add("NoiseReductionEnabled", &m_bNoiseReductionEnabled);
	m_oSettings.add("CAREnabled", &m_bCAREnabled);
	m_oSettings.add("AccelerationEnabled", &m_bAccelerationDataEnabled);
	m_oSettings.add("CounterEnabled", &m_bCounterEnabled);
	m_oSettings.add("LinqQualityEnabled", &m_bLinkQualityEnabled);
	m_oSettings.add("BatteryLevelEnabled", &m_bBatteryLevelEnabled);
	m_oSettings.add("ValidationIndicatorEnabled", &m_bValidationIndicatorEnabled);
	
	m_oSettings.load();
}

CDrivergNautilusInterface::~CDrivergNautilusInterface(void)
{
}

const char* CDrivergNautilusInterface::getName(void)
{
	return "g.tec g.Nautilus using g.NEEDaccess";
}

//___________________________________________________________________//
//                                                                   //

boolean CDrivergNautilusInterface::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	unsigned __int32 i;

	if(m_rDriverContext.isConnected())
	{
		return false;
	}
	if(!m_oHeader.isChannelCountSet()||!m_oHeader.isSamplingFrequencySet())
	{
		return false;
	}
	
	if (ui32SampleCountPerSentBlock > 250)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Sample count per sent block cannot be higher than 250 samples for g.Nautilus\n";
		return false;
	}

	// initialize basic GDS functions before the first GDS function itself is called
	GDS_Initialize();

	// get number of channels actually acquired
	m_ui32AcquiredChannelCount = 0;
	for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
	{
		if (find(m_vSelectedChannels.begin(),m_vSelectedChannels.end(),(i + 1)) != m_vSelectedChannels.end())
		{
			m_oHeader.setChannelUnits(m_ui32AcquiredChannelCount, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);
			m_ui32AcquiredChannelCount += 1;
		}
	}

	m_oHeader.setChannelCount(m_ui32AcquiredChannelCount);

	// g.Nautilus provides some extra channel, add them to the channels count
	// and provide corresponding names
	unsigned __int32 l_ui32AcquiredChannelCount = m_ui32AcquiredChannelCount;
	if (m_bAccelerationDataEnabled)
	{
		m_oHeader.setChannelCount(l_ui32AcquiredChannelCount+3);
		m_oHeader.setChannelName(l_ui32AcquiredChannelCount, "CH_Accel_x");
		m_oHeader.setChannelName(l_ui32AcquiredChannelCount + 1, "CH_Accel_y");
		m_oHeader.setChannelName(l_ui32AcquiredChannelCount + 2, "CH_Accel_z");
		m_oHeader.setChannelUnits(l_ui32AcquiredChannelCount, OVTK_UNIT_Meter_Per_Second_Squared, OVTK_FACTOR_Base);
		m_oHeader.setChannelUnits(l_ui32AcquiredChannelCount + 1, OVTK_UNIT_Meter_Per_Second_Squared, OVTK_FACTOR_Base);
		m_oHeader.setChannelUnits(l_ui32AcquiredChannelCount + 2, OVTK_UNIT_Meter_Per_Second_Squared, OVTK_FACTOR_Base);
		l_ui32AcquiredChannelCount = m_oHeader.getChannelCount();
	}
	if(m_bCounterEnabled)
	{
		m_oHeader.setChannelCount(l_ui32AcquiredChannelCount+1);
		m_oHeader.setChannelName(l_ui32AcquiredChannelCount, "CH_Counter");
		m_oHeader.setChannelUnits(l_ui32AcquiredChannelCount,OVTK_UNIT_Dimensionless,OVTK_FACTOR_Base);
		l_ui32AcquiredChannelCount = m_oHeader.getChannelCount();
	}
	if(m_bLinkQualityEnabled)
	{
		m_oHeader.setChannelCount(l_ui32AcquiredChannelCount+1);
		m_oHeader.setChannelName(l_ui32AcquiredChannelCount, "CH_LQ");
		m_oHeader.setChannelUnits(l_ui32AcquiredChannelCount,OVTK_UNIT_10_2_Percent,OVTK_FACTOR_Base);
		l_ui32AcquiredChannelCount = m_oHeader.getChannelCount();
	}
	if (m_bBatteryLevelEnabled)
	{
		m_oHeader.setChannelCount(l_ui32AcquiredChannelCount+1);
		m_oHeader.setChannelName(l_ui32AcquiredChannelCount, "CH_Battery");
		m_oHeader.setChannelUnits(l_ui32AcquiredChannelCount,OVTK_UNIT_10_2_Percent,OVTK_FACTOR_Base);
		l_ui32AcquiredChannelCount = m_oHeader.getChannelCount();
	}
	if(m_bDigitalInputEnabled)
	{
		m_oHeader.setChannelCount(l_ui32AcquiredChannelCount+1);
		m_oHeader.setChannelName(l_ui32AcquiredChannelCount, "CH_Event");
		m_oHeader.setChannelUnits(l_ui32AcquiredChannelCount,OVTK_UNIT_Dimensionless,OVTK_FACTOR_Base);
		l_ui32AcquiredChannelCount = m_oHeader.getChannelCount();
	}
	if(m_bValidationIndicatorEnabled)
	{
		m_oHeader.setChannelCount(l_ui32AcquiredChannelCount+1);
		m_oHeader.setChannelName(l_ui32AcquiredChannelCount, "CH_Valid");
		m_oHeader.setChannelUnits(l_ui32AcquiredChannelCount,OVTK_UNIT_Dimensionless,OVTK_FACTOR_Base);
		l_ui32AcquiredChannelCount = m_oHeader.getChannelCount();
	}

	// initialize connection and open connection handle and get serial of connected g.Nautilus
	GDS_DEVICE_CONNECTION_INFO *l_pConnectedDevicesInfo;
	OpenViBE::uint32 l_ui32ConnectedDevicesCount = 0;
	GDS_ENDPOINT l_epHostEp;
	GDS_ENDPOINT l_epLocalEp;
	OpenViBE::uint8 l_bByteIP[4];
	char l_sTempIP[16];
	
	l_bByteIP[0] = 1;
	l_bByteIP[1] = 0;
	l_bByteIP[2] = 0;
	l_bByteIP[3] = 127;

	_snprintf_s(l_sTempIP,IP_ADDRESS_LENGTH_MAX,"%d.%d.%d.%d",l_bByteIP[3],l_bByteIP[2],l_bByteIP[1],l_bByteIP[0]);

	for(i = 0; i < IP_ADDRESS_LENGTH_MAX; i++)
	{
		l_epHostEp.IpAddress[i] = l_sTempIP[i];
		l_epLocalEp.IpAddress[i] = l_sTempIP[i];
	}

	l_epHostEp.Port = 50223;
	l_epLocalEp.Port = 50224;

	OpenViBE::boolean l_bDeviceFound = false;
	m_oGdsResult = GDS_GetConnectedDevices(l_epHostEp, l_epLocalEp, &l_pConnectedDevicesInfo, &l_ui32ConnectedDevicesCount);
	if(m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	for(i = 0; i < l_ui32ConnectedDevicesCount; i++)
	{
		// if devices are in use they cannot be used for a new acquisition
		if((l_pConnectedDevicesInfo[i].InUse) && (i < l_ui32ConnectedDevicesCount))
		{
			continue;
		}
		// only one device can be used for data acquisition, as g.Nautilus cannot be synchronized
		if ((l_pConnectedDevicesInfo[i].InUse) && (l_pConnectedDevicesInfo[i].ConnectedDevicesLength > 1))
		{
			continue;
		}
		GDS_DEVICE_INFO *l_pDeviceInfo = l_pConnectedDevicesInfo[i].ConnectedDevices;
		if(l_pDeviceInfo[0].DeviceType == GDS_DEVICE_TYPE_GNAUTILUS)
		{
			// check if device used for configuration is still available
			if (!strcmp(l_pDeviceInfo[0].Name,m_sDeviceSerial.c_str()))
			{
				m_ui32DeviceCount = 1;
				l_bDeviceFound = true;
				break;
			}
		}
	}
	if(l_bDeviceFound == false)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "No g.Nautilus device found\n";
		return false;
	}
	
	char (*l_sDeviceNames)[DEVICE_NAME_LENGTH_MAX] = new char[1][DEVICE_NAME_LENGTH_MAX];
	bool l_bOpenExclusively = true;
	BOOL l_bIsCreator;

	strncpy_s(l_sDeviceNames[0], m_sDeviceSerial.c_str(), DEVICE_NAME_LENGTH_MAX);

	// connect to device
	m_oGdsResult = GDS_Connect(l_epHostEp, l_epLocalEp, l_sDeviceNames, m_ui32DeviceCount, l_bOpenExclusively, &m_pDevice, &l_bIsCreator);
	if(m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	m_rDriverContext.getLogManager() << LogLevel_Info << "Connected to device : " << m_sDeviceSerial.c_str() << "\n";

	m_oNautilusDeviceCfg.SamplingRate = m_oHeader.getSamplingFrequency();
	m_oNautilusDeviceCfg.NumberOfScans = ui32SampleCountPerSentBlock;
	m_oNautilusDeviceCfg.NetworkChannel = m_ui32NetworkChannel;
	m_oNautilusDeviceCfg.AccelerationData = m_bAccelerationDataEnabled;
	m_oNautilusDeviceCfg.BatteryLevel = m_bBatteryLevelEnabled;
	m_oNautilusDeviceCfg.CAR = m_bCAREnabled;
	m_oNautilusDeviceCfg.Counter = m_bCounterEnabled;
	m_oNautilusDeviceCfg.DigitalIOs = m_bDigitalInputEnabled;
	m_oNautilusDeviceCfg.LinkQualityInformation = m_bLinkQualityEnabled;
	m_oNautilusDeviceCfg.NoiseReduction = m_bNoiseReductionEnabled;
	m_oNautilusDeviceCfg.Slave = 0;
	m_oNautilusDeviceCfg.ValidationIndicator = m_bValidationIndicatorEnabled;
	m_oNautilusDeviceCfg.InputSignal = GDS_GNAUTILUS_INPUT_SIGNAL(m_i32InputSource);

	for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
	{
		if (find(m_vSelectedChannels.begin(),m_vSelectedChannels.end(),(i + 1)) != m_vSelectedChannels.end())
		{
			m_oNautilusDeviceCfg.Channels[i].Enabled = true;
		}
		else
		{
			m_oNautilusDeviceCfg.Channels[i].Enabled = false;
		}
		m_oNautilusDeviceCfg.Channels[i].Sensitivity = m_f64Sensitivity;
		if (i < m_vCAR.size())
		{
			m_oNautilusDeviceCfg.Channels[i].UsedForCar = m_vCAR[i];
		}
		else
		{
			m_oNautilusDeviceCfg.Channels[i].UsedForCar = false;
		}
		if (i < m_vNoiseReduction.size())
		{
			m_oNautilusDeviceCfg.Channels[i].UsedForNoiseReduction = m_vNoiseReduction[i];
		}
		else
		{
			m_oNautilusDeviceCfg.Channels[i].UsedForNoiseReduction = false;
		}
		m_oNautilusDeviceCfg.Channels[i].BandpassFilterIndex = m_i32BandPassFilterIndex;
		m_oNautilusDeviceCfg.Channels[i].NotchFilterIndex = m_i32NotchFilterIndex;
		if (i < m_vBipolarChannels.size())
		{
			m_oNautilusDeviceCfg.Channels[i].BipolarChannel = m_vBipolarChannels[i];
		}
		else
		{
			m_oNautilusDeviceCfg.Channels[i].BipolarChannel = false;
		}
	}

	// Free memory allocated for list of connected devices by GDS_GetConnectedDevices
	m_oGdsResult = GDS_FreeConnectedDevicesList(&l_pConnectedDevicesInfo, l_ui32ConnectedDevicesCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	// Builds up a buffer to store acquired samples. This buffer
	// will be sent to the acquisition server later...
	m_pSample=new float[l_ui32AcquiredChannelCount*m_oNautilusDeviceCfg.NumberOfScans];
	if(!m_pSample)
	{
		delete [] m_pSample;
		m_pSample=NULL;
		return false;
	}
	// set up data buffer for gds getdata function
	m_pBuffer = new float[l_ui32AcquiredChannelCount*m_oNautilusDeviceCfg.NumberOfScans];
	if(!m_pBuffer)
	{
		delete [] m_pBuffer;
		m_pBuffer = NULL;
		return false;
	}
	m_ui32BufferSize = l_ui32AcquiredChannelCount*m_oNautilusDeviceCfg.NumberOfScans;

	// Saves parameters
	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;
	return true;
}

boolean CDrivergNautilusInterface::start(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) return false;

	// if no device was found return
	if (m_ui32DeviceCount != 1)
	{
		return false;
	}

	GDS_CONFIGURATION_BASE *l_oGdsDeviceConfigurations = new GDS_CONFIGURATION_BASE[m_ui32DeviceCount];
	l_oGdsDeviceConfigurations[0].Configuration = &m_oNautilusDeviceCfg;
	l_oGdsDeviceConfigurations[0].DeviceInfo.DeviceType = GDS_DEVICE_TYPE_GNAUTILUS;
	strcpy_s(l_oGdsDeviceConfigurations[0].DeviceInfo.Name,m_sDeviceSerial.c_str());
	unsigned __int32 l_ui32ScanCount = 0;
	unsigned __int32 l_ui32ChannelsPerDevice = 0;
	unsigned __int32 l_ui32BufferSizeInSamples = 0;

	// set device configuration
	m_oGdsResult = GDS_SetConfiguration(m_pDevice,l_oGdsDeviceConfigurations,m_ui32DeviceCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	// initialize data ready event and set data ready event handle
	g_oDataReadyEventHandle = NULL;
	g_oDataReadyEventHandle = CreateEvent(NULL, false, false, NULL);
	m_oGdsResult = GDS_SetDataReadyCallback(m_pDevice, (GDS_Callback)OnDataReadyEventHandler,m_oNautilusDeviceCfg.NumberOfScans,NULL);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	// set number of scans getdata function will return
	m_ui32AvailableScans = m_oNautilusDeviceCfg.NumberOfScans;

	// ...
	// request hardware to start
	// sending data
	// ...
	// start acquisition
	m_oGdsResult = GDS_StartAcquisition(m_pDevice);
	if(m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	// start data stream from server
	m_oGdsResult = GDS_StartStreaming(m_pDevice);
	if(m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	m_oGdsResult = GDS_GetDataInfo(m_pDevice, &l_ui32ScanCount, NULL, &l_ui32ChannelsPerDevice, &l_ui32BufferSizeInSamples);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	return true;
}

boolean CDrivergNautilusInterface::loop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return true;

	OpenViBE::CStimulationSet l_oStimulationSet;

	if(m_rDriverContext.isStarted())
	{
		DWORD dw_ret = WaitForSingleObject(g_oDataReadyEventHandle,5000);
		if(dw_ret == WAIT_TIMEOUT)
		{
			// if data ready event is not triggered in 5000ms add a timeout handler
			m_rDriverContext.getLogManager() << LogLevel_Error << "No data received in 5 seconds\n";
			return false;
		}
		// when data is ready call get data function with amount of scans to return
		m_oGdsResult = GDS_GetData(m_pDevice,&m_ui32AvailableScans,m_pBuffer,m_ui32BufferSize);
		if(m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
			return false;
		}
		
		// put data from receiving buffer to application buffer
		for(uint32 i=0; i<m_oHeader.getChannelCount(); i++)
		{
			for(uint32 j=0; j<m_ui32SampleCountPerSentBlock; j++)
			{
				m_pSample[i*m_ui32SampleCountPerSentBlock+j]=m_pBuffer[j*(m_oHeader.getChannelCount())+i];
			}
		}
	}
	
	// ...
	// receive samples from hardware
	// put them the correct way in the sample array
	// whether the buffer is full, send it to the acquisition server
	//...
	m_pCallback->setSamples(m_pSample);
	
	// When your sample buffer is fully loaded,
	// it is advised to ask the acquisition server
	// to correct any drift in the acquisition automatically.
	m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());

	// ...
	// receive events from hardware
	// and put them the correct way in a CStimulationSet object
	//...
	m_pCallback->setStimulationSet(l_oStimulationSet);

	return true;
}

boolean CDrivergNautilusInterface::stop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return false;

	// ...
	// request the hardware to stop
	// sending data
	// ...


	// stop streaming
	m_oGdsResult = GDS_StopStreaming(m_pDevice);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	// stop data acquisiton
	m_oGdsResult = GDS_StopAcquisition(m_pDevice);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	return true;
}

boolean CDrivergNautilusInterface::uninitialize(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) return false;

	// ...
	// uninitialize hardware here
	// ...

	m_oGdsResult = GDS_Disconnect(&m_pDevice);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	delete [] m_pSample;
	m_pSample=NULL;
	m_pCallback=NULL;

	// uninitialize basic GDS functions after last GDS function is called
	GDS_Uninitialize();

	m_rDriverContext.getLogManager() << LogLevel_Info << "Disconnected from device : " << m_sDeviceSerial.c_str() << "\n";

	return true;
}

//___________________________________________________________________//
//                                                                   //
boolean CDrivergNautilusInterface::isConfigurable(void)
{
	return true; // change to false if your device is not configurable
}

boolean CDrivergNautilusInterface::configure(void)
{
	// Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
	CConfigurationgNautilusInterface m_oConfiguration(m_rDriverContext, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-gNautilusInterface.ui",
		m_sDeviceSerial,m_i32InputSource,m_ui32NetworkChannel,m_i32BandPassFilterIndex,m_i32NotchFilterIndex,m_f64Sensitivity,m_bDigitalInputEnabled,
		m_bNoiseReductionEnabled,m_bCAREnabled,m_bAccelerationDataEnabled,m_bCounterEnabled,m_bLinkQualityEnabled,m_bBatteryLevelEnabled,
		m_bValidationIndicatorEnabled,m_vSelectedChannels,m_vBipolarChannels,m_vCAR,m_vNoiseReduction);
	
	if(!m_oConfiguration.configure(m_oHeader))
	{
		return false;
	}
	m_oSettings.save();
	
	return true;
}

void OnDataReadyEventHandler(GDS_HANDLE connection_handle, void *usr_data)
{
	// signals data is ready on GDS acquisition buffer
	if(!SetEvent(g_oDataReadyEventHandle))
	{
		// insert error handling if necessary
	}
}

#endif // TARGET_HAS_ThirdPartyGNEEDaccessAPI


