/*
* NeuroServo driver for OpenViBE
*
* \author (NeuroServo)
* \date Wed Nov 23 00:24:00 2016
*
* \note This driver will not compile with VS2010 due to missing HID library. Use VS2013.
*
*/

#if defined TARGET_OS_Windows
#if defined TARGET_HAS_ThirdPartyNeuroServo

#include "ovasCDriverNeuroServoHid.h"
#include "ovasCConfigurationNeuroServoHid.h"

#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>
#include <system/ovCTime.h>
#include <Windows.h>
#include <functional>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

/*
  NeuroServo general infos
*/
#define NEUROSERVO_VID				OpenViBE::uint16(0xC1C4)
#define NEUROSERVO_PID				OpenViBE::uint16(0x8B25)
#define NEUROSERVO_DATA_SIZE		OpenViBE::uint16(65)
#define NEUROSERVO_DRIVER_NAME		"NeuroServo"
#define NEUROSERVO_SENDDATA_BLOCK	OpenViBE::uint32(1024)

/*
General define
*/
#define DRIFTSTABILISATION_MAXNBSWITCH	10
#define DRIFTSTABILISATION_MINNBSWITCH	2
#define DRIFTSTABILISATION_MULTFACTOR	1.2

//___________________________________________________________________//
//                                                                   //

CDriverNeuroServoHid::CDriverNeuroServoHid(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_NeuroServoHid", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_pSample(NULL)
	,m_bAutomaticShutdown(false)
	,m_bShutdownOnDriverDisconnect(true)
	,m_bDeviceLightEnable(false)
	,m_bIsDeviceInitialized(false)
{
	m_oHeader.setSamplingFrequency(2048);
	m_oHeader.setChannelCount(1);

	// Set the Device basic infos
	m_ui16VendorId = NEUROSERVO_VID;
	m_ui16ProductId = NEUROSERVO_PID;
	m_ui16DataSize = NEUROSERVO_DATA_SIZE;
	m_sDriverName = NEUROSERVO_DRIVER_NAME;
	m_bIsDeviceConnected = false;
	
	m_oSettings.add("Header", &m_oHeader);	
	m_oSettings.add("Vid", &m_ui16VendorId);
	m_oSettings.add("Pid", &m_ui16ProductId);
	m_oSettings.add("DataSize", &m_ui16DataSize);
	m_oSettings.add("AutomaticShutdown", &m_bAutomaticShutdown);
	m_oSettings.add("ShutdownOnDrvDisconnect", &m_bShutdownOnDriverDisconnect);
	m_oSettings.add("DeviceLightEnable", &m_bDeviceLightEnable);

	m_oSettings.load();	
}

CDriverNeuroServoHid::~CDriverNeuroServoHid(void)
{
}

const char* CDriverNeuroServoHid::getName(void)
{
	return m_sDriverName;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverNeuroServoHid::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	if(m_rDriverContext.isConnected()) return false;
	if (!m_oHeader.isChannelCountSet() || !m_oHeader.isSamplingFrequencySet()) return false;
	
	m_ui32SampleCountPerSentBlock = ui32SampleCountPerSentBlock;
	
	// Set the specific infos of the device
	m_oHidDevice.setHidDeviceInfos(m_ui16VendorId, m_ui16ProductId, m_ui16DataSize);
	
	// Connect to the device
	m_rDriverContext.getLogManager() << LogLevel_Info << m_sDriverName << ": Connecting to the device.\n";
	if (!m_oHidDevice.connect())
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_sDriverName << ": Connection failed.\n";
		return false;
	}

	// Device connection state
	m_bIsDeviceConnected = true;

	// Take into account the configuration of "Automatic Shutdown" and "Device Light Enable"
	deviceShutdownAndLightConfiguration();

	// Bind the callback methods
	m_oHidDevice.dataReceived = std::bind(&CDriverNeuroServoHid::processDataReceived, this, std::placeholders::_1);
	m_oHidDevice.deviceDetached = std::bind(&CDriverNeuroServoHid::deviceDetached, this);
	m_oHidDevice.deviceAttached = std::bind(&CDriverNeuroServoHid::deviceAttached, this);

	m_rDriverContext.getLogManager() << LogLevel_Info << m_sDriverName << ": Connection succeded.\n";

	m_ui64SendBlockRequiredTime = ITimeArithmetics::sampleCountToTime(m_oHeader.getSamplingFrequency(), (ui32SampleCountPerSentBlock));
	m_ui64SendSampleRequiredTime = ITimeArithmetics::sampleCountToTime(m_oHeader.getSamplingFrequency(), 1);

	m_pSample = new float32[ui32SampleCountPerSentBlock];
	if (!m_pSample)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not allocate memory for sample array\n";
		delete[] m_pSample;
		m_pSample = NULL;
		return false;
	}
	m_bDeviceEpochDetected = false;
	m_bIsDeviceInitialized = true;

	// Saves parameters
	m_pCallback = &rCallback;
	m_ui32SampleCountPerSentBlock = ui32SampleCountPerSentBlock;

	return true;
}

boolean CDriverNeuroServoHid::start(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if (m_rDriverContext.isStarted()) return false;

	// Ensure that the device is connected
	if (!m_bIsDeviceConnected)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_sDriverName << ": Device is not connected.\n";
		return false;
	}

	m_bQueueOverflow = false;
	m_bQueueUnderflow = false;

	// Build the data to be sent to the device
	BYTE data[65];
	data[0] = 0x02; // HID Report ID
	data[1] = 0x09; // Cmd
	data[2] = 0x01; // 0x01 (Reserved)
	data[3] = 0x01; // Start acquisition

	m_rDriverContext.getLogManager() << LogLevel_Info << m_sDriverName << ": Request acquisition to be started.\n";

	if (!m_oHidDevice.writeToDevice(data, m_ui16DataSize))
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_sDriverName << ": Failed to start acquisittion.\n";
		return false;
	}

	m_rDriverContext.getLogManager() << LogLevel_Info << m_sDriverName << ": Acquisiton started.\n";
	m_ui32SampleIndexForSentBlock = 0;
	m_ui64TimeStampLastSentBlock = 0;
	m_i64LastDriftSampleCount = 0;
	m_bDeviceEpochDetected = false;
	m_i64DriftAutoCorrectionDelay = 0;
	m_ui64NbSwitchDrift = 0;
	m_fDriftAutoCorrFactor = 1.0;
	m_bIsDriftWasInEarlyDirection = false;
	m_f32SampleValue = 0;

	// Set approximate lattency in regard to hardware implementation. Queue lattency 
	// should be adjusted but write_available() member is not accessible in current 
	// boost version.
	m_rDriverContext.setInnerLatencySampleCount(-1024);

	return true;
}

boolean CDriverNeuroServoHid::loop(void)
{
	if (!m_bIsDeviceInitialized) return false;
	if (!m_rDriverContext.isConnected()) return false;
	if (!m_rDriverContext.isStarted()) return true;

	for (uint32 i = 0; i < m_ui32SampleCountPerSentBlock; i++)
	{
		if (!m_pBufferQueue.pop(m_f32SampleValue) && m_bQueueUnderflow==false && m_ui64NbSwitchDrift == DRIFTSTABILISATION_MAXNBSWITCH)
		{
			// We wait for stabilisation before warn
			m_bQueueUnderflow = true;
		}
		m_pSample[i] = m_f32SampleValue; // N.B. Last sample value is sent if queue was empty
	}

	int64 l_i64CurrentDriftSampleCount = m_rDriverContext.getDriftSampleCount();
	if (l_i64CurrentDriftSampleCount != 0)
	{
		// Drift in early direction
		if (l_i64CurrentDriftSampleCount > m_i64LastDriftSampleCount)
		{
			if (m_bIsDriftWasInEarlyDirection == false && m_ui64NbSwitchDrift < DRIFTSTABILISATION_MAXNBSWITCH)
			{
				m_ui64NbSwitchDrift++;
				m_bIsDriftWasInEarlyDirection = true;
				if (m_ui64NbSwitchDrift > DRIFTSTABILISATION_MINNBSWITCH)
				{
					m_fDriftAutoCorrFactor = m_fDriftAutoCorrFactor / (float32)(DRIFTSTABILISATION_MULTFACTOR);
				}
			}
			m_i64DriftAutoCorrectionDelay = m_i64DriftAutoCorrectionDelay + (int64)(m_fDriftAutoCorrFactor*m_ui64SendSampleRequiredTime);
		}
		// Drift in late direction
		if (l_i64CurrentDriftSampleCount < m_i64LastDriftSampleCount)
		{
			if (m_bIsDriftWasInEarlyDirection == true && m_ui64NbSwitchDrift < DRIFTSTABILISATION_MAXNBSWITCH)
			{
				m_ui64NbSwitchDrift++;
				m_bIsDriftWasInEarlyDirection = false;
				if (m_ui64NbSwitchDrift > DRIFTSTABILISATION_MINNBSWITCH)
				{
					m_fDriftAutoCorrFactor = m_fDriftAutoCorrFactor / (float32)(DRIFTSTABILISATION_MULTFACTOR);
				}
			}
			m_i64DriftAutoCorrectionDelay = m_i64DriftAutoCorrectionDelay - (int64)(m_fDriftAutoCorrFactor*m_ui64SendSampleRequiredTime);
		}
	}

	const uint64 l_ui64ElapsedTimeSinceLastSentBlock = System::Time::zgetTime() - m_ui64TimeStampLastSentBlock;
	if (m_ui64SendBlockRequiredTime>l_ui64ElapsedTimeSinceLastSentBlock)
	{
		// If we're early, sleep before sending. This code regulate the data drift
		const uint64 l_ui64SleepTime = m_ui64SendBlockRequiredTime - l_ui64ElapsedTimeSinceLastSentBlock;
		System::Time::zsleep((uint64)(l_ui64SleepTime + m_i64DriftAutoCorrectionDelay));
	}

	m_ui64TimeStampLastSentBlock = System::Time::zgetTime();
	m_pCallback->setSamples(m_pSample);
	int64 OvAutoCorrection = m_rDriverContext.getSuggestedDriftCorrectionSampleCount();
	m_rDriverContext.correctDriftSampleCount(OvAutoCorrection);
	m_pCallback->setStimulationSet(m_oStimulationSet);
	m_oStimulationSet.clear();
	if (OvAutoCorrection != 0)
	{
		// We do not apply driver time correction
		m_i64LastDriftSampleCount = 0;
	}
	else
	{
		// We will apply driver time auto correction on next loop
		m_i64LastDriftSampleCount = l_i64CurrentDriftSampleCount;
	}
	if (m_bQueueUnderflow)
	{
		m_bQueueUnderflow = false;
		m_rDriverContext.getLogManager() << LogLevel_Trace << m_sDriverName << ": Sample block has been skipped by driver!! Driver queue was empty\n";
	}
	return true;
}

boolean CDriverNeuroServoHid::stop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return false;

	// Ensure that the device is connected
	if (!m_bIsDeviceConnected)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_sDriverName << ": Device is not connected.\n";
		return false;
	}

	// Build the data to be sent to the device
	BYTE data[65];
	data[0] = 0x02; // HID Report ID
	data[1] = 0x09; // Cmd
	data[2] = 0x01; // 0x01 (Reserved)
	data[3] = 0x00; // Stop acquisition

	m_rDriverContext.getLogManager() << LogLevel_Info << m_sDriverName << ": Request acquisition to be stopped.\n";

	if (!m_oHidDevice.writeToDevice(data, m_ui16DataSize))
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_sDriverName << ": Failed to stop acquisittion.\n";
		return false;
	}

	m_rDriverContext.getLogManager() << LogLevel_Info << m_sDriverName << ": Acquisiton stopped.\n";

	return true;
}

boolean CDriverNeuroServoHid::uninitialize(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) return false;

	if (m_bIsDeviceInitialized && m_bIsDeviceConnected)
	{
		if (m_bShutdownOnDriverDisconnect)
		{
			BYTE data[65];
			data[0] = 0x02; // HID Report ID
			data[1] = 0x16; // Cmd
			data[2] = 0x01; // 0x01 (Reserved)
			data[3] = 0x01; // Shutdown the device
			m_oHidDevice.writeToDevice(data, m_ui16DataSize);
		}
		else
		{
			// Set device to normal user mode		

			BYTE data[65];
			data[0] = 0x02; // HID Report ID
			data[1] = 0x17; // Cmd
			data[2] = 0x01; // 0x01 (Reserved)
			data[3] = 0x01; // ask the device to switch off automatically
			m_oHidDevice.writeToDevice(data, m_ui16DataSize);

			data[0] = 0x02; // HID Report ID
			data[1] = 0x18; // Cmd
			data[2] = 0x01; // 0x01 (Reserved)
			data[3] = 0x01; // Enable the device light
			m_oHidDevice.writeToDevice(data, m_ui16DataSize);
		}
	}
	m_bIsDeviceConnected = false;
	m_bIsDeviceInitialized = false;
	delete[] m_pSample;
	m_pSample = NULL;
	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //
boolean CDriverNeuroServoHid::isConfigurable(void)
{
	return true;
}

boolean CDriverNeuroServoHid::configure(void)
{
	CConfigurationNeuroServoHid m_oConfiguration(m_rDriverContext, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-NeuroServoHid.ui");
	
	// Set the current state of the "Automatic Shutdown" "Shutdown on driver disconnect" and "Device Light Enable"
	m_oConfiguration.setRadioAutomaticShutdown(m_bAutomaticShutdown);
	m_oConfiguration.setRadioShutdownOnDriverDisconnect(m_bShutdownOnDriverDisconnect);
	m_oConfiguration.setRadioDeviceLightEnable(m_bDeviceLightEnable);

	if (!m_oConfiguration.configure(m_oHeader)) 
	{
		return false;
	}

	// Get the configuration from the ui
	m_bAutomaticShutdown = m_oConfiguration.getAutomaticShutdownStatus();
	m_bShutdownOnDriverDisconnect = m_oConfiguration.getShutdownOnDriverDisconnectStatus();
	m_bDeviceLightEnable = m_oConfiguration.getDeviceLightEnableStatus();

	// Save the settings
	m_oSettings.save();
	
	return true;
}

//___________________________________________________________________//
//                                                                   //
// NEUROSERVO SPECIFIC METHODS IMPLEMENTATION

void CDriverNeuroServoHid::processDataReceived(BYTE data[])
{
	// Ensure the acquisition has started
	if (m_rDriverContext.isStarted() && m_bIsDeviceInitialized)
	{
		if (data[1] == 0x03)
		{
			float32 pValue;
			uint32 ui32NbElement = (data[3] + (data[4] << 8));
			uint32 ui32ElementIndex = (data[5] + (data[6] << 8));

			if (ui32ElementIndex == 0)
			{
				// Queue lattency should be considered but write_available() member
				// is not accessible in current boost version.
				/*
				size_t NbItemInQueue = 1024 - m_pBufferQueue.write_available();
				if ((uint32)(NbItemInQueue) != m_ui32BufferQueueCount)
				{
					m_rDriverContext.setInnerLatencySampleCount((int64)(m_ui32BufferQueueCount - (uint32)(NbItemInQueue)));
					m_ui32BufferQueueCount = (uint32)(NbItemInQueue);
				}
				*/
				m_ui32NbSamplesReceived = 0;
				m_bDeviceEpochDetected = true;
			}
			if (m_bDeviceEpochDetected == true)
			{
				// Loop and process the data
				for (int LoopData = (7); LoopData < m_ui16DataSize - 1; LoopData = LoopData + 2)
				{
					if (m_ui32NbSamplesReceived < ui32NbElement)
					{
						pValue = (float)((float)(data[LoopData]) + (float)((data[LoopData + 1]) << 8));
						// Remove comp2
						if (pValue > 32767)
						{
							pValue = pValue - 65536;
						}
						if (!m_pBufferQueue.push(pValue))
						{
							m_bQueueOverflow = true;
						}
						m_ui32SampleIndexForSentBlock++;

						// If full epoch is received
						if (m_ui32SampleIndexForSentBlock == 1024)
						{
							m_bDeviceEpochDetected = false;
							m_ui32SampleIndexForSentBlock = 0;
							if (m_bQueueOverflow && m_ui64NbSwitchDrift == DRIFTSTABILISATION_MAXNBSWITCH) // We wait for stabilisation before warn
							{
								m_bQueueOverflow = false;
								m_rDriverContext.getLogManager() << LogLevel_Trace << m_sDriverName << ": Sample block has been skipped by driver!! Driver queue was full\n";
							}
						}
						m_ui32NbSamplesReceived++;
					}
				}
			}
		}
	}
}

void CDriverNeuroServoHid::deviceDetached()
{
	m_rDriverContext.getLogManager() << LogLevel_Info << m_sDriverName << ": Device detached.\n";
	m_bIsDeviceConnected = false;
	m_bIsDeviceInitialized = false;
}

void CDriverNeuroServoHid::deviceAttached()
{
	m_rDriverContext.getLogManager() << LogLevel_Info << m_sDriverName << ": Device attached.\n";
	m_bIsDeviceConnected = true;
}

void CDriverNeuroServoHid::deviceShutdownAndLightConfiguration()
{
	BYTE data[65];
	data[0] = 0x02; // HID Report ID
	data[2] = 0x01; // 0x01 (Reserved)

	data[1] = 0x17; // Cmd
	if (m_bAutomaticShutdown)
	{
		data[3] = 0x01; // ask the device to switch off automatically
		m_oHidDevice.writeToDevice(data, m_ui16DataSize);
	}
	else
	{
		data[3] = 0x00; // ask the device not to switch off automatically
		m_oHidDevice.writeToDevice(data, m_ui16DataSize);
	}

	data[1] = 0x18; // Cmd
	if (m_bDeviceLightEnable)
	{
		data[3] = 0x01; // Enable the device light
		m_oHidDevice.writeToDevice(data, m_ui16DataSize);
	}
	else
	{
		data[3] = 0x00; // Disable the device light
		m_oHidDevice.writeToDevice(data, m_ui16DataSize);
	}
}

#endif
#endif // TARGET_OS_Windows