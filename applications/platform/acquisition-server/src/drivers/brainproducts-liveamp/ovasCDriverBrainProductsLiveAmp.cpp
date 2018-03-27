/*
 * Brain Products LiveAmp driver for OpenViBE
 * Copyright (C) 2017 Brain Products - Author : Ratko Petrovic
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */
/*********************************************************************
* History
* [2017-03-29] ver 1.0											          - RP
* [2017-04-04] ver 1.1 Cosmetic changes: int32/uint32, static_cast<>...   - RP
*              Function loop: optimized buffer copying. 
* [2017-04-28] ver 1.2 LiveAmp8 and LiveAmp16 channles support added.     - RP
*			   Introduced checking of Bipolar channels.
*
*********************************************************************/
#if defined TARGET_HAS_ThirdPartyLiveAmpAPI

#include "ovasCDriverBrainProductsLiveAmp.h"
#include "ovasCConfigurationBrainProductsLiveAmp.h"

#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>
#include <Amplifier_LIB.h>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;


//___________________________________________________________________//
//                                                                   //
CDriverBrainProductsLiveAmp::CDriverBrainProductsLiveAmp(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_BrainProductsLiveAmp", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_pHandle(NULL)
	,m_oHeader()
	,m_oStimulationSet()
	,m_ui32SampleCountPerSentBlock(0)
	,m_pSampleBuffer(NULL)
	,m_ui32BufferSize(0)
	,m_ui32SampleSize(0)	
	,m_ui32PhysicalSampleRate(250)
	,m_bUseAccChannels(false)
	,m_ui32CountEEG(32)
	,m_ui32CountAux(0)
	,m_ui32CountACC(0)	
	,m_ui32CountBipolar(0)
	,m_ui32BadImpedanceLimit(10000)
	,m_ui32GoodImpedanceLimit(5000)
	,m_sSerialNumber("05203-0077")
	,m_iRecordingMode(-1)
	,m_bUseBipolarChannels(false)
	,m_ui32ImpedanceChannels(0)
	,m_ui32UsedChannelsCounter(0)
	,m_ui32EnabledChannels(0)	
	,m_bSimulationMode(false)
{
	// set default sampling rates:
	m_vSamplingRatesArray.push_back(250);
	m_vSamplingRatesArray.push_back(500);
	m_vSamplingRatesArray.push_back(1000);

	m_oHeader.setSamplingFrequency(250); // init for the first time
	
	// The following class allows saving and loading driver settings from the acquisition server .conf file	
	m_oSettings.add("EEGchannels", &m_ui32CountEEG);
	m_oSettings.add("AUXchannels", &m_ui32CountAux);
	m_oSettings.add("ACCchannels", &m_ui32CountACC);
	m_oSettings.add("BipolarChannels", &m_ui32CountBipolar);
	m_oSettings.add("IncludeACC",  &m_bUseAccChannels);
	
	m_oSettings.add("SerialNr",	   &m_sSerialNumber);	
	m_oSettings.add("GoodImpedanceLimit", &m_ui32GoodImpedanceLimit);
	m_oSettings.add("BadImpedanceLimit",  &m_ui32BadImpedanceLimit);
	m_oSettings.add("Header", &m_oHeader);
	m_oSettings.add("UseBipolarChannels",  &m_bUseBipolarChannels);	//m_oSettings.add("SettingName", &variable);
	
	// To save your custom driver settings, register each variable to the SettingsHelper	
	m_oSettings.load();	

	m_ui32PhysicalSampleRate = m_oHeader.getSamplingFrequency();
	m_oHeader.setChannelCount(m_ui32CountEEG + m_ui32CountBipolar + m_ui32CountAux + m_ui32CountACC);
}

CDriverBrainProductsLiveAmp::~CDriverBrainProductsLiveAmp(void)
{
	uninitialize();
	//delete (this);
}

const char* CDriverBrainProductsLiveAmp::getName(void)
{
	return "Brain Products LiveAmp";
}

//___________________________________________________________________//
//                                                                   //
boolean CDriverBrainProductsLiveAmp::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	m_ui32SampleCountPerSentBlock = ui32SampleCountPerSentBlock;
	m_rDriverContext.getLogManager() << LogLevel_Info << "[INIT] LiveAmp initialization has been started. " << "\n";

	if(m_rDriverContext.isConnected()) return false;
	if(!m_oHeader.isChannelCountSet()||!m_oHeader.isSamplingFrequencySet()) return false;

	m_rDriverContext.getLogManager() << LogLevel_Info << "[INIT] LiveAmp initialization process is running, please wait..." << "\n";
	
	// Builds up a buffer to store
	// acquired samples. This buffer
	// will be sent to the acquisition
	// server later...
	if(m_pSampleBuffer != NULL)
	{
		delete [] m_pSampleBuffer;
		m_pSampleBuffer=NULL;
	}
	
	// ...
	// initialize hardware and get
	if(!initializeLiveAmp())
		return false;
	
	m_rDriverContext.getLogManager() << LogLevel_Info << "[INIT] LiveAmp successfully initialized! \n";

	if(!configureLiveAmp())
		return false;

	if(!checkAvailableChannels())
		return false;

	if(!disableAllAvailableChannels()) // must disable all channels, than enable only one that will be used.
		return false;

	if(!getChannelIndices())
		return false;
	
	if(!configureImpedanceMessure())
		return false;

	// check if the impedance checking is possible:
	if(m_rDriverContext.isImpedanceCheckRequested())	
	{
		if(m_ui32CountEEG > 0)
		{
			int32 l_i32Type =-1;
			int32 l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, 0, CPROP_I32_Electrode, &l_i32Type, sizeof(l_i32Type));		
			if (l_i32Res != AMP_OK)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "[configure] GetProperty type error: " << l_i32Res << "\n";
				return false;
			}

			/*if(l_i32Type == EL_ACTIVE)
			{
				m_oHeader.setImpedanceCheckRequested (false);
				m_rDriverContext.getLogManager() << LogLevel_Warning << "[configure] Cannot provide impedance checking with attached electrodes! \n";
			}*/
		}
	}
		
	if(!m_rDriverContext.isImpedanceCheckRequested())
	{
		// calculate what size has each "sample"
		m_ui32SampleSize = getLiveAmpSampleSize();
		if(m_ui32EnabledChannels != m_ui32UsedChannelsCounter)
			m_rDriverContext.getLogManager() << LogLevel_Error << "m_ui32UsedChannelsCounter: " << m_ui32UsedChannelsCounter <<  " !=  m_ui32EnabledChannels= " << m_ui32EnabledChannels << "\n";

		m_ui32BufferSize = 1000 * m_ui32SampleSize;
		m_pSampleBuffer = new OpenViBE::uint8[m_ui32BufferSize];	
	}
	else
	{
		m_ui32SampleSize = m_ui32ImpedanceChannels * sizeof(float32);

		m_ui32BufferSize = 1000 * m_ui32SampleSize;
		m_pSampleBuffer = new OpenViBE::uint8[m_ui32BufferSize];

		int32 l_i32Res = ampStartAcquisition(m_pHandle);
		if(l_i32Res != AMP_OK)
			m_rDriverContext.getLogManager() << LogLevel_Trace << " [Impedance] Device Handle = NULL.\n";

		m_rDriverContext.getLogManager() << LogLevel_Trace << "Impedance Acquisition started...\n";		
	}

	m_vSendBuffer.clear();

	m_vSample.clear();
	m_vSample.resize(m_oHeader.getChannelCount());// allocate for all analog signals / channels

	// Saves parameters
	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock = ui32SampleCountPerSentBlock;
	
	return true;
}



boolean CDriverBrainProductsLiveAmp::start(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "START called.\n";
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) 
		return false;

	if (m_pHandle == NULL)
		m_rDriverContext.getLogManager() << LogLevel_Error << "Device Handle = NULL.\n";
	
	int32 l_i32Res = ampGetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_RecordingMode, &m_iRecordingMode, sizeof(m_iRecordingMode));
	if(m_iRecordingMode != RM_NORMAL)
	{
		l_i32Res = ampStopAcquisition(m_pHandle);
		if(l_i32Res != AMP_OK)
			m_rDriverContext.getLogManager() << LogLevel_Error << " [start] ampStopAcquisition error code = " << l_i32Res << "\n";

		m_iRecordingMode = RM_NORMAL;
		l_i32Res = ampSetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_RecordingMode, &m_iRecordingMode, sizeof(m_iRecordingMode));
		if(l_i32Res != AMP_OK)
			m_rDriverContext.getLogManager() << LogLevel_Error << " [start] ampSetProperty mode error code = " << l_i32Res << "\n";
		
		// calculate what size has each "sample"
		m_ui32SampleSize = getLiveAmpSampleSize();
		if(m_ui32EnabledChannels != m_ui32UsedChannelsCounter)
			m_rDriverContext.getLogManager() << LogLevel_Error << "m_ui32UsedChannelsCounter: " << m_ui32UsedChannelsCounter <<  " !=  m_ui32EnabledChannels= " << m_ui32EnabledChannels << "\n";

		delete [] m_pSampleBuffer;
		m_pSampleBuffer = NULL;

		m_ui32BufferSize = 1000 * m_ui32SampleSize;		
		m_pSampleBuffer = new BYTE[m_ui32BufferSize];	
		m_vSendBuffer.clear();
		m_vSample.clear();
		m_vSample.resize(m_oHeader.getChannelCount());// allocate for all analog signals / channels
	}

	l_i32Res = ampStartAcquisition(m_pHandle);
	if(l_i32Res != AMP_OK)
		m_rDriverContext.getLogManager() << LogLevel_Error << " [start] ampStartAcquisition error code = " << l_i32Res << "\n";

	m_rDriverContext.getLogManager() << LogLevel_Trace << "Acquisition started...\n";

	return true;
}

boolean CDriverBrainProductsLiveAmp::loop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
		
	if (m_rDriverContext.isStarted())
	{
		OpenViBE::CStimulationSet l_oStimulationSet;
		std::vector<std::vector<float32>> l_vTemp_buffer(1, std::vector<float32>(1));
	
		while (1)
		{
			// receive samples from hardware
			// put them the correct way in the sample array
			// whether the buffer is full, send it to the acquisition server
			int32 l_i32SamplesRead = ampGetData(m_pHandle, m_pSampleBuffer, m_ui32BufferSize, 0);
			if(l_i32SamplesRead < 1)
				return true;

			uint32 l_ui32LocChannelCount = m_oHeader.getChannelCount();

			liveAmpExtractData(l_i32SamplesRead, l_vTemp_buffer);
		
			int32 l_i32SampCount = l_i32SamplesRead / m_ui32SampleSize;
			for( int32 sample = 0; sample < l_i32SampCount; sample++ )
			{
				std::vector<float32> loc_buffer (m_ui32UsedChannelsCounter);			
				for( uint32 ch = 0; ch < m_ui32UsedChannelsCounter; ch++ )			
					loc_buffer[ch] = l_vTemp_buffer[sample][ch];	// access only with available indices

				m_vSendBuffer.push_back(loc_buffer);
			}
			
			int32 l_i32ReadToSend = m_vSendBuffer.size() - m_ui32SampleCountPerSentBlock; // must check buffer size in that way !!!
			if (l_i32ReadToSend > 0)
			{
				// for debug only: m_rDriverContext.getLogManager() << LogLevel_Info << "[info] Buffer size = " << m_vSendBuffer.size() << "\n";
				m_vSample.clear();
				m_vSample.resize(l_ui32LocChannelCount*m_ui32SampleCountPerSentBlock);			 
			
				for( uint32 ch = 0, i=0; ch <  m_oHeader.getChannelCount(); ch++ )		
				{
					for( uint32 sample = 0; sample < m_ui32SampleCountPerSentBlock; sample++ )				
						m_vSample[i++] =  m_vSendBuffer[sample][ch];
				}

				// receive events from hardware
				// and put them the correct way in a CStimulationSet object
				//m_pCallback->setStimulationSet(l_oStimulationSet);
				for( uint32 sample = 0; sample < m_ui32SampleCountPerSentBlock; sample++ )	
				{	// check triggers:
					for (uint32 t=0; t < m_vTriggerIndices.size(); t++)
					{					
						uint16 trigg = static_cast<uint16>(m_vSendBuffer[sample][m_vTriggerIndices[t]]);
						if(trigg != m_vLastTriggerStates[t])
						{
							uint64 l_ui64StimulationTime = ITimeArithmetics::sampleCountToTime(m_oHeader.getSamplingFrequency(), uint64(sample));
							m_oStimulationSet.appendStimulation(OVTK_StimulationId_Label(trigg), l_ui64StimulationTime, 0); // send the same time as the 'sample'
							m_vLastTriggerStates[t] = trigg;				
						}
					}
				}

				// send acquired data...
				m_pCallback->setSamples(&m_vSample[0], m_ui32SampleCountPerSentBlock);			
				m_pCallback->setStimulationSet(m_oStimulationSet);
			
				m_vSample.clear();
				m_oStimulationSet.clear();
				// When your sample buffer is fully loaded, 
				// it is advised to ask the acquisition server 
				// to correct any drift in the acquisition automatically.
				m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());
			
				// delete sent samples
				m_vSendBuffer.erase(m_vSendBuffer.begin(), m_vSendBuffer.begin() + m_ui32SampleCountPerSentBlock);				
				break;
			}
			if (m_bSimulationMode)
				Sleep(100);
		}
	}	
	else if(m_rDriverContext.isImpedanceCheckRequested()) //impedance measurement 
	{
		if(m_iRecordingMode != RM_IMPEDANCE)
			return true; // go out of the loop
		
		std::vector<std::vector<float32>> l_vTempBuffer(1, std::vector<float32>(1));
		uint32 l_ui32SamplesRead = ampGetImpedanceData(m_pHandle, m_pSampleBuffer, m_ui32BufferSize);
		if(l_ui32SamplesRead < 1)
			return true;

		liveAmpExtractImpedanceData(l_ui32SamplesRead, l_vTempBuffer);

		uint32 l_ui32CountOfMeasuredChannles = (m_ui32ImpedanceChannels - 2)  / 2 ; // GND, REF and pairs of ch+ and ch-, countOfMeasuredChannles can be different than  m_oHeader.getChannelCount()

		uint32 l_ui32SmpCount = l_ui32SamplesRead / m_ui32SampleSize;
		for( uint32 sample = 0; sample < l_ui32SmpCount; sample++ )
		{			
			// must extract impedance values for each cEEG channel, and or Bipolar channel
			for( uint32 ch = 0;  ch < l_ui32CountOfMeasuredChannles; ch++ )	
			{
				int32 l_i32Type;
				int32 l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, ch, CPROP_I32_Type, &l_i32Type, sizeof(l_i32Type));		
				if (l_i32Res != AMP_OK)
				{
					m_rDriverContext.getLogManager() << LogLevel_Error << "[ImpMeas] GetProperty type error: " << l_i32Res << "\n";
					break;
				}

				if (l_i32Type == CT_EEG) 
				{			
					m_rDriverContext.updateImpedance(ch, l_vTempBuffer[sample][2*(ch+1)]);
				}
				else if(l_i32Type == CT_BIP)
				{
					m_rDriverContext.updateImpedance(ch, l_vTempBuffer[sample][2*(ch+1)] - l_vTempBuffer[sample][2*(ch+1) + 1]);
				}
			}
		}
	}

	return true;
}

boolean CDriverBrainProductsLiveAmp::stop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return false;

	// ...
	// request the hardware to stop
	// sending data
	// ...
	if(m_pHandle != NULL)
	{
		int32 l_i32Res = ampStopAcquisition(m_pHandle);
		m_rDriverContext.getLogManager() << LogLevel_Trace << "'stop' called. stopping the acquisition:" << l_i32Res << "\n";
	}

	return true;
}

boolean CDriverBrainProductsLiveAmp::uninitialize(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) return false;

	// ...
	// uninitialize hardware here
	// ...
	if(m_pHandle != NULL)
	{
		int32 l_i32Res = ampStopAcquisition(m_pHandle);
		if(l_i32Res != AMP_OK)
			m_rDriverContext.getLogManager() << LogLevel_Trace << "Uninitialized called. stopping acquisition:" << l_i32Res << "\n";

		l_i32Res = ampCloseDevice(m_pHandle);			
		m_rDriverContext.getLogManager() << LogLevel_Trace << "Uninitialized called. Closing the device:" << l_i32Res << "\n";
	}


	delete [] m_pSampleBuffer;
	m_pSampleBuffer=NULL;
	m_pCallback=NULL;
	m_vSample.clear();

	return true;
}

//___________________________________________________________________//
//                                                                   //
boolean CDriverBrainProductsLiveAmp::isConfigurable(void)
{
	return true; // change to false if your device is not configurable
}

boolean CDriverBrainProductsLiveAmp::configure(void)
{
	// get sampling rate index:
	uint32 sampRateIndex = -1;
	for (uint32 i=0; i < m_vSamplingRatesArray.size(); i++)
	{
		if(m_ui32PhysicalSampleRate == m_vSamplingRatesArray[i])
			sampRateIndex = i;
	}


	// Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
	CConfigurationBrainProductsLiveAmp m_oConfiguration(
		*this, 
		OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-BrainProductsLiveAmp.ui",
		sampRateIndex,
		m_ui32CountEEG,
		m_ui32CountBipolar,
		m_ui32CountAux,
		m_ui32CountACC,
		m_bUseAccChannels,
		m_ui32GoodImpedanceLimit,
		m_ui32BadImpedanceLimit,
		m_sSerialNumber,
		m_bUseBipolarChannels);

	if(!m_oConfiguration.configure(m_oHeader))
	{
		return false;
	}

	// update sampling rate
	if(sampRateIndex >= 0 && sampRateIndex <  m_vSamplingRatesArray.size() )
		m_ui32PhysicalSampleRate = m_vSamplingRatesArray[sampRateIndex];

	m_oHeader.setSamplingFrequency(m_ui32PhysicalSampleRate);
	m_oHeader.setChannelCount(m_ui32CountEEG + m_ui32CountBipolar + m_ui32CountAux + m_ui32CountACC);
	
	m_oSettings.save();
	
	return true;
}



OpenViBE::boolean CDriverBrainProductsLiveAmp::initializeLiveAmp()
{	
	HANDLE hlocDevice = NULL;
	int32 l_i32Res;
	char HWI[20];
	strcpy_s(HWI, "ANY");  // use word SIM to simulate the LiveAmp
	std::string serialN(m_sSerialNumber);

	if(strcmp(HWI, "SIM") == 0)	
	{
		m_bSimulationMode = true;
		serialN = "054201-0001";   // 32 channels
		//serialN = "054201-0010"; // 64 channels
	}

	if(m_pHandle != NULL)
	{
		ampCloseDevice(m_pHandle);
		m_pHandle = NULL;
	}
	
	// dialog window doesn't show all meassage, due to threading problems. The information will be seen in Dialog title.
	GtkWindow *window = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
	GtkWidget *dialog = gtk_message_dialog_new (window, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,GTK_BUTTONS_NONE, "  Connecting to LiveAmp, wait some seconds please ...    ");
	gtk_window_set_title (GTK_WINDOW(dialog)  , " Connecting to LiveAmp, please wait...");

	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER); 
	gtk_widget_show_all (dialog);
	
	
	l_i32Res = ampEnumerateDevices(HWI, sizeof(HWI), "LiveAmp", 0);
	if (l_i32Res == 0)			
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[InitializeLiveAmp] No LiveAmp connected! \n";
		gtk_widget_destroy (dialog);
		return false;
	}
	else if (l_i32Res < 0)			
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[InitializeLiveAmp] Error by LiveAmp initialization; error code= " << l_i32Res << "\n";
		gtk_widget_destroy (dialog);
		return false;
	}
	else
	{				
		int32 l_i32NumDevices = l_i32Res;
		for (int32 i = 0; i < l_i32NumDevices; i++)
		{
			hlocDevice = NULL;
			l_i32Res = ampOpenDevice(i, &hlocDevice);
			if(l_i32Res != AMP_OK)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "[InitializeLiveAmp] Cannot open device # " << i << "; error code = " << l_i32Res << "\n";
				gtk_widget_destroy (dialog);
				return false;
			}
				
			char sVar[20]; 
			l_i32Res = ampGetProperty(hlocDevice, PG_DEVICE, i, DPROP_CHR_SerialNumber, sVar, sizeof(sVar)); // get serial number
			if(l_i32Res != AMP_OK)		
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "[InitializeLiveAmp] Cannot read Serial number from device # " << i << ";  error code = " << l_i32Res << "\n";
				gtk_widget_destroy (dialog);
				return false;
			}

			m_rDriverContext.getLogManager() << LogLevel_Info << "[InitializeLiveAmp] Serial number = " << sVar << "\n";

			int32 l_i32Check = strcmp(sVar, serialN.c_str());
			if(l_i32Check == 0)
			{
				m_rDriverContext.getLogManager() << LogLevel_Info << "[InitializeLiveAmp] Serial numbers match: " << serialN.c_str() << "\n";
				m_pHandle = hlocDevice; // save device handler
				gtk_widget_destroy (dialog);
				return true;
			}
			else
			{		
				l_i32Res = ampCloseDevice(hlocDevice);
				if(l_i32Res != AMP_OK)
				{						
					m_rDriverContext.getLogManager() << LogLevel_Error << "[InitializeLiveAmp] Cannot close the device # " << i << "; error code = " << l_i32Res << "\n";
					gtk_widget_destroy (dialog);
					return false;
				}
			}
		}
	}

	gtk_widget_destroy (dialog);

	if(m_pHandle == NULL)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[InitializeLiveAmp] There is no LiveAmp with serial number: " << serialN.c_str() << " detected!";		
		return false;
	}

	return false;
}

OpenViBE::boolean CDriverBrainProductsLiveAmp::configureLiveAmp(void)
{	
	// amplifier configuration
	float32 l_f32Var = static_cast<float32> (m_oHeader.getSamplingFrequency());
	int32 l_i32Res = ampSetProperty(m_pHandle, PG_DEVICE, 0, DPROP_F32_BaseSampleRate, &l_f32Var, sizeof(l_f32Var));
	if(l_i32Res != AMP_OK)					
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[Config]  Error setting sampling rate, error code:  "  << l_i32Res << "\n";
		return false;
	}

	m_rDriverContext.getLogManager() << LogLevel_Info << "[Config]  Set sampling frequency = "<<  m_oHeader.getSamplingFrequency()  << "\n";

	
	m_iRecordingMode = RM_NORMAL;  // initialize acquisition mode: standard/normal
	l_i32Res = ampSetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_RecordingMode, &m_iRecordingMode, sizeof(m_iRecordingMode));
	if(l_i32Res != AMP_OK)					
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[Config] Error setting acquisition mode, error code:  " << l_i32Res << "\n";	
		return false;
	}

	// set good and bad impedance level
	l_i32Res = ampSetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_GoodImpedanceLevel, &m_ui32GoodImpedanceLimit, sizeof(m_ui32GoodImpedanceLimit));
	if(l_i32Res != AMP_OK)					
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[Config] Error setting DPROP_I32_GoodImpedanceLevel, error code:  " << l_i32Res << "\n";	
		return false;
	}

	l_i32Res = ampSetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_BadImpedanceLevel, &m_ui32BadImpedanceLimit, sizeof(m_ui32BadImpedanceLimit));
	if(l_i32Res != AMP_OK)					
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[Config] Error setting DPROP_I32_BadImpedanceLevel, error code:  " << l_i32Res << "\n";	
		return false;
	}

	return true;
}

OpenViBE::boolean CDriverBrainProductsLiveAmp::checkAvailableChannels(void)
{
	// check the "LiveAmp_Channel" version: LiveAmp8, LiveAmp16, LiveAmp32 or LiveAmp64
	uint32 l_ui32ModuleChannels; // check number of channels that are allowed!
	int32 l_i32Res = ampGetProperty(m_pHandle, PG_MODULE, 0, MPROP_I32_UseableChannels, &l_ui32ModuleChannels, sizeof(l_ui32ModuleChannels));
	if (l_i32Res != AMP_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "#1 MPROP_I32_UseableChannels, error code= " << l_i32Res << "\n";
		return false;
	}

	// checks available channels, gets the count of each channel type
	uint32 l_ui32AvailableEEG = 0;
	uint32 l_ui32AvailableAUX = 0;
	uint32 l_ui32AvailableACC = 0;
	uint32 l_ui32AvailableTrig = 0;

	int32 l_i32Avlbchannels;
	l_i32Res = ampGetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_AvailableChannels, &l_i32Avlbchannels, sizeof(l_i32Avlbchannels));	
		
	for (int32 c = 0; c < l_i32Avlbchannels; c++)
	{
		int32 l_i32Type;
		l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_I32_Type, &l_i32Type, sizeof(l_i32Type));		
		if (l_i32Res != AMP_OK)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "[Check] GetProperty type error: " << l_i32Res << "\n";
			return false;
		}
		
		if (l_i32Type == CT_AUX)
		{		
			char cValue[20];	
			l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_CHR_Function, &cValue, sizeof(cValue));
			if (l_i32Res != AMP_OK)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "[Check] GetProperty CPROP_CHR_Function #1 error: " << l_i32Res << "\n";
				return false;
			}	

			if(cValue[0] == 'X' || cValue[0] == 'Y' ||cValue[0] == 'Z' || cValue[0] == 'x' ||cValue[0] == 'y' ||cValue[0] == 'z')
				l_ui32AvailableACC++;
			else
				l_ui32AvailableAUX++;
		}
		else if (l_i32Type == CT_EEG || l_i32Type == CT_BIP)
			l_ui32AvailableEEG++;

		else if (l_i32Type == CT_TRG || l_i32Type == CT_DIG)
		{
			char cValue[20];	
			l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_CHR_Function, &cValue, sizeof(cValue));
			if (l_i32Res != AMP_OK)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "[Check] GetProperty CPROP_CHR_Function #2 error: " << l_i32Res << "\n";
				return false;
			}	
			if(strcmp("Trigger Input", cValue) == 0 )
				l_ui32AvailableTrig += 1 + 8; // One LiveAmp trigger input + 8 digital inputs from AUX box		
		}
	}


	//*********************************************************************************
	// very important check !!! EEG + Bipolar must match configuration limitations
	//*********************************************************************************
	if (l_ui32ModuleChannels == 32)
	{
		// if there is any Bipolar channel, it means that last 8 physical channels must be Bipolar
		if (m_ui32CountBipolar > 0 && m_ui32CountEEG > (32 - 8))
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "[Check] Number of EEG channels:" << m_ui32CountEEG << " and Bipolar channels: " << m_ui32CountBipolar << " don't match the LiveAmp configuration !!!\n";
			return false;
		}
	}

	
	// Used EEG channels:
	if (m_ui32CountEEG + m_ui32CountBipolar > l_ui32ModuleChannels)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[Check] Number of used EEG and Bip. channels '" << (m_ui32CountEEG + m_ui32CountBipolar )<< "' don't match with number of channels from LiveAmp Channel configuration '" << l_ui32ModuleChannels << "\n";
		return false;
	}
	
	else if (m_ui32CountEEG + m_ui32CountBipolar > l_ui32AvailableEEG)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[Check] Number of available EEG channels '" << l_ui32AvailableEEG << "' don't match with number of channels from Device configuration '" << m_ui32CountEEG << "\n";
		return false;
	} 		 
	else if (m_oHeader.getSamplingFrequency() >= 1000 && l_ui32ModuleChannels >= 32 && (m_ui32CountEEG + m_ui32CountBipolar)> 24)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "If the sampling rate is 1000Hz, there should be 24 EEG (or 21 EEG and 3 AUX)  channels used, to avoid sample loss due to Bluetooth connection.\n";
		return false;
	}


	if(m_ui32CountAux > l_ui32AvailableAUX)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Number of input AUX channeles (" << m_ui32CountAux << ") don't match available number of AUX channels (" << l_ui32AvailableAUX << ") \n";
		return false;
	}

	if (m_oHeader.getSamplingFrequency() >= 1000 && l_ui32ModuleChannels >= 32 && (m_ui32CountEEG + m_ui32CountBipolar +  m_ui32CountAux + m_ui32CountACC) > 24)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error <<  "If the sampling rate is 1000Hz, there should be 24 EEG (or 21 EEG and 3 AUX)  channels used, to avoid sample loss due to Bluetooth connection. \n ";
		return false;
	}

	return true;
}

OpenViBE::boolean CDriverBrainProductsLiveAmp::disableAllAvailableChannels()
{
	// disables all channle first. It is better to do so, than to enable only the channels that will be used, according to the driver settings.
	int32 l_i32Avlbchannels;
	int32 l_i32Res = ampGetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_AvailableChannels, &l_i32Avlbchannels, sizeof(l_i32Avlbchannels));	
	if (l_i32Res != AMP_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error <<  " Get available channels, error code= " << l_i32Res << "\n";		
		return false;
	}


	// now disable all channel first,	
	for (int32 c = 0; c < l_i32Avlbchannels; c++)
	{
		BOOL disabled = false;
		int32 l_i32Type = 0;

		int32 l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_I32_Type, &l_i32Type, sizeof(l_i32Type));	
		if (l_i32Res != AMP_OK)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error <<  " Error: ampGetProperty for channel type, channel= " << c << "; error code= " << l_i32Res << "\n";
			return false;
		}

		// can not disable trigger and digital channels.
		if (l_i32Type == CT_DIG  || l_i32Type == CT_TRG)
			continue;
				
		l_i32Res = ampSetProperty(m_pHandle, PG_CHANNEL, c, CPROP_B32_RecordingEnabled, &disabled, sizeof(disabled));
		if (l_i32Res != AMP_OK)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error <<  " Error: ampGetProperty for channel type, channel= " << c << "; error code= " << l_i32Res << "\n";
			return false;
		}
	}	

	return true;
}


OpenViBE::boolean CDriverBrainProductsLiveAmp::getChannelIndices()
{	
	int32 l_i32Enable = 1;
	m_ui32EnabledChannels = 0;
	m_vTriggerIndices.clear();
	
	std::vector<int32> l_vAccessIndices;  // indexes of physical channel 

	int32 l_i32Avlbchannels;
	int32 l_i32Res = ampGetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_AvailableChannels, &l_i32Avlbchannels, sizeof(l_i32Avlbchannels));	
	if (l_i32Res != AMP_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error <<  "#3 Get available channels, error code= " << l_i32Res << "\n";		
		return false;
	}


	// check the "LiveAmp_Channel" version: LiveAmp8, LiveAmp16, LiveAmp32 or LiveAmp64
	uint32 l_ui32ModuleChannels; 
	l_i32Res = ampGetProperty(m_pHandle, PG_MODULE, 0, MPROP_I32_UseableChannels, &l_ui32ModuleChannels, sizeof(l_ui32ModuleChannels));
	if (l_i32Res != AMP_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "#2 MPROP_I32_UseableChannels, error code= " << l_i32Res << "\n";
		return false;
	}

	// enable channels and get indexes of channels to be used!
	// The order of physical channels by LiveAmp: EEGs, BIPs, AUXs, ACCs, TRIGs 
	uint32 l_i32EegCnt = 0;
	uint32 l_i32BipCnt = 0;
	uint32 l_i32AuxCnt = 0;
	uint32 l_i32AccCnt = 0;
	
	for (int32 c =0; c < l_i32Avlbchannels; c++ )	
	{
		int32 l_i32Type;
		l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_I32_Type, &l_i32Type, sizeof(l_i32Type));		
		if (l_i32Res != AMP_OK)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "[Check] GetProperty type error: " << l_i32Res << "\n";
			return false;
		}
		
		// type of channel is first EEG. After one of the channle is re-typed as Bipolar, that the rest of 8 channels in the group of 32 will be Bipolar as well
		if (l_i32Type == CT_EEG || l_i32Type == CT_BIP)
		{
			if (l_i32EegCnt < m_ui32CountEEG)
			{
				int32 l_i32Res = ampSetProperty(m_pHandle, PG_CHANNEL, c, CPROP_B32_RecordingEnabled, &l_i32Enable, sizeof(l_i32Enable));
				if (l_i32Res != AMP_OK)
				{
					m_rDriverContext.getLogManager() << LogLevel_Error << " Cannot enable channel: " << c << "; error code= " << l_i32Res << "\n";
					return false;
				}

				l_vAccessIndices.push_back(m_ui32EnabledChannels);
				m_ui32EnabledChannels++;
				l_i32EegCnt++;
			}
			else if (l_i32BipCnt < m_ui32CountBipolar)
			{
				//*********************************************************************************
				// If Bipolar channel will be used, set the l_i32Type to Bipolar here!!!
				// Still it works for LiveAmp8, LiveAmp16 and LiveAmp32.
				// Bipolar channels can be only phisical channels from index 24 - 31 !!!
				//*********************************************************************************
				if ((c > 23 && c < 32) || (l_ui32ModuleChannels > 32 && c > 55 && c < 64))  // last 8 channels of 32ch module can be bipolar channels 
				{	
					int32 l_i32Res = ampSetProperty(m_pHandle, PG_CHANNEL, c, CPROP_B32_RecordingEnabled, &l_i32Enable, sizeof(l_i32Enable));
					if (l_i32Res != AMP_OK)
					{
						m_rDriverContext.getLogManager() << LogLevel_Error << " Cannot enable channel: " << c << "; error code= " << l_i32Res << "\n";
						return false;
					}

					int32 l_i32TP = CT_BIP;
					l_i32Res = ampSetProperty(m_pHandle, PG_CHANNEL, c, CPROP_I32_Type, &l_i32TP, sizeof(l_i32TP));	
					if(l_i32Res != AMP_OK)
					{
						m_rDriverContext.getLogManager() << LogLevel_Error << "[Check] SetProperty type CT_BIP error: " << l_i32Res << "\n";
						return false;
					}
					
					l_vAccessIndices.push_back(m_ui32EnabledChannels);
				    m_ui32EnabledChannels++;
				    l_i32BipCnt++;
				}
			}
		}
		else if (l_i32Type == CT_AUX)
		{
			char cValue[20];	
			l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_CHR_Function, &cValue, sizeof(cValue));
			if (l_i32Res != AMP_OK)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "[Check] GetProperty CPROP_CHR_Function #1 error: " << l_i32Res << "\n";
				return false;
			}	

			// detect ACC channels
			if(cValue[0] == 'X' || cValue[0] == 'Y' ||cValue[0] == 'Z' || cValue[0] == 'x' ||cValue[0] == 'y' ||cValue[0] == 'z')
			{
				if(l_i32AccCnt < m_ui32CountACC)
				{					 
					int32 l_i32Res = ampSetProperty(m_pHandle, PG_CHANNEL, c, CPROP_B32_RecordingEnabled, &l_i32Enable, sizeof(l_i32Enable));
					if (l_i32Res != AMP_OK)
					{
						m_rDriverContext.getLogManager() << LogLevel_Error << " Cannot enable channel: "<< c << "; error code= " << l_i32Res << "\n";
						return false;
					}
					l_vAccessIndices.push_back(m_ui32EnabledChannels); 
					m_ui32EnabledChannels++;
					l_i32AccCnt++;
				}
			}
			else
			{
				if(l_i32AuxCnt < m_ui32CountAux)
				{
					int32 l_i32Res = ampSetProperty(m_pHandle, PG_CHANNEL, c, CPROP_B32_RecordingEnabled, &l_i32Enable, sizeof(l_i32Enable));
					if (l_i32Res != AMP_OK)
					{
						m_rDriverContext.getLogManager() << LogLevel_Error << " Cannot enable channel: "<< c << "; error code= " << l_i32Res << "\n";
						return false;
					}
					l_vAccessIndices.push_back(m_ui32EnabledChannels); 
					m_ui32EnabledChannels++;
					l_i32AuxCnt++;
				}
			}
		}

		else if (l_i32Type == CT_TRG || l_i32Type == CT_DIG)  // those channels are always enabled!
		{
			char cValue[20];	
			l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_CHR_Function, &cValue, sizeof(cValue));
			if (l_i32Res != AMP_OK)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << " GetProperty CPROP_CHR_Function error by Trigger, error code= " << l_i32Res << "\n";
				return false;
			}	
			
			if(strcmp("Trigger Input", cValue) == 0 ) 			
			{
				m_vTriggerIndices.push_back(m_ui32EnabledChannels);
			}
			
			m_ui32EnabledChannels++; // Trigger and digital channels are always enabled!
		}
	}

	// initialize trigger states
	for (uint32 i=0; i < m_vTriggerIndices.size(); i++)
		m_vLastTriggerStates.push_back(0);

	// double check
	int32 l_i32Avlbchannels2;
	l_i32Res = ampGetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_AvailableChannels, &l_i32Avlbchannels2, sizeof(l_i32Avlbchannels2));
	if (l_i32Res != AMP_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "#3 Get available channels, error code= " << l_i32Res << "\n";
		return false;
	}

	return true;
}


OpenViBE::boolean CDriverBrainProductsLiveAmp::configureImpedanceMessure()
{
	if(!m_rDriverContext.isImpedanceCheckRequested())
		return true;

	m_iRecordingMode = RM_IMPEDANCE;
	int32 l_i32Res = ampSetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_RecordingMode, &m_iRecordingMode, sizeof(m_iRecordingMode));
	if(l_i32Res != AMP_OK)					
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[ImpedanceMessureInit] Error setting impedance mode, error code:  " << l_i32Res << "\n";	
		return false;
	}

	m_ui32ImpedanceChannels = 2; // GND + REF

	// for impedance measurements use only EEG and Bip channels

	int32 l_i32Avlbchannels;
	l_i32Res = ampGetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_AvailableChannels, &l_i32Avlbchannels, sizeof(l_i32Avlbchannels));	
	if (l_i32Res != AMP_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error <<  "[ImpedanceMessureInit] Get available channels, error code= " << l_i32Res << "\n";		
		return false;
	}

	// enable channels and get indexes of channels to be used!
	// The order of physical channels by LiveAmp: EEGs, AUXs, ACCs, TRIGs 
	int32 l_i32EegCnt = 0;
	for (int32 c =0; c < l_i32Avlbchannels; c++ )	
	{
		int32 l_i32Type;
		l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_I32_Type, &l_i32Type, sizeof(l_i32Type));		
		if (l_i32Res != AMP_OK)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "[ImpedanceMessureInit] GetProperty type error: " << l_i32Res << "\n";
			return false;
		}

		if (l_i32Type == CT_EEG || l_i32Type == CT_BIP)
		{
			BOOL enabled = false;
			l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_B32_RecordingEnabled, &enabled, sizeof(enabled));
			if (l_i32Res != AMP_OK)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << " [ImpedanceMessureInit] Cannot read enable channel: "<< c << "; error code= " << l_i32Res << "\n";
				return false;
			}
		
			if (enabled)
				l_i32EegCnt += 2; // for each channel CH+ and CH-
			
		}
	}

	m_ui32ImpedanceChannels += l_i32EegCnt;

	return true;
}


OpenViBE::uint32 CDriverBrainProductsLiveAmp::getLiveAmpSampleSize(void)
{
	int32 l_i32Channels, l_i32DataType;
	float32	l_f32Resolution;
	int32 l_i32ByteSize = 0;

	m_vDataTypeArray.clear();
	m_ui32UsedChannelsCounter = 0;

	// iterate through all enabled channels
	int32 l_i32Res = ampGetProperty(m_pHandle, PG_DEVICE, 0, DPROP_I32_AvailableChannels, &l_i32Channels, sizeof(l_i32Channels));
	for (int32 c = 0; c < l_i32Channels; c++)
	{
		int32 l_i32Enabled;
		l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_B32_RecordingEnabled, &l_i32Enabled, sizeof(l_i32Enabled));
		if (l_i32Enabled)
		{
			// get the type of channel
			l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_I32_DataType, &l_i32DataType, sizeof(l_i32DataType));
			m_vDataTypeArray.push_back(l_i32DataType);
			l_i32Res = ampGetProperty(m_pHandle, PG_CHANNEL, c, CPROP_F32_Resolution, &l_f32Resolution, sizeof(l_f32Resolution));			
			m_vResolutionArray.push_back(l_f32Resolution);
			m_ui32UsedChannelsCounter++;

			switch (l_i32DataType)
			{
			case DT_INT16:
			case DT_UINT16:
				{
					l_i32ByteSize += 2;
				}
				break;
			case DT_INT32:
			case DT_UINT32:
			case DT_FLOAT32:
				{
					l_i32ByteSize += 4;				
					
				}
				break;
			case DT_INT64:
			case DT_UINT64:
			case DT_FLOAT64:
				{
					l_i32ByteSize += 8;
				}
				break;
			default:
				break;
			}
		}
	}
	
	l_i32ByteSize += 8; // add the sample counter size

	return l_i32ByteSize;
}

void CDriverBrainProductsLiveAmp::liveAmpExtractData(OpenViBE::int32 samplesRead, std::vector<std::vector<OpenViBE::float32>> &extractData)
{
	// extracts the samples for each channel, saves to in the appropriate format.
	uint64 l_ui64SampCnt;
	int32 l_i32NumSamples = samplesRead / m_ui32SampleSize;
	int32 l_i32Offset = 0;
	float32 l_f32Sample = 0;

	extractData.clear();
	extractData.resize(l_i32NumSamples);

	for (int32 s = 0; s < l_i32NumSamples; s++)
	{
		l_i32Offset = 0;
		l_ui64SampCnt = *(uint64*)&m_pSampleBuffer[s*m_ui32SampleSize + l_i32Offset];		
		l_i32Offset += 8; // sample counter offset

		extractData[s].resize(m_ui32UsedChannelsCounter);

		for (uint32 i=0; i < m_ui32UsedChannelsCounter; i++)
		{
			switch (m_vDataTypeArray[i])
			{
				case DT_INT16:
					{
						int16_t tmp = reinterpret_cast<int16_t&> (m_pSampleBuffer[s*m_ui32SampleSize + l_i32Offset]);
						l_f32Sample = static_cast<float32> (tmp) * m_vResolutionArray[i];
						l_i32Offset += 2;
						break;
					}
				case DT_UINT16:
					{
						uint16_t tmp = reinterpret_cast<uint16_t&> (m_pSampleBuffer[s*m_ui32SampleSize + l_i32Offset]);
						l_f32Sample  = static_cast<float32> (tmp) * m_vResolutionArray[i];
						l_i32Offset += 2;
						break;
					}					
				case DT_INT32:
					{
						int32_t tmp = reinterpret_cast<int32_t&> (m_pSampleBuffer[s*m_ui32SampleSize + l_i32Offset]);
						l_f32Sample = static_cast<float32> (tmp) * m_vResolutionArray[i];
						l_i32Offset += 4;
						break;
					}
				case DT_UINT32:
					{
						uint32_t tmp = reinterpret_cast<uint32_t&> (m_pSampleBuffer[s*m_ui32SampleSize + l_i32Offset]);
						l_f32Sample  = static_cast<float32> (tmp) * m_vResolutionArray[i];
						l_i32Offset += 4;
						break;
					}
				case DT_FLOAT32:
					{
						float32 tmp = reinterpret_cast<float32&> (m_pSampleBuffer[s*m_ui32SampleSize + l_i32Offset]);
						l_f32Sample = tmp * m_vResolutionArray[i];
						l_i32Offset += 4;
						break;
					}
				case DT_INT64:
					{
						int64_t tmp = reinterpret_cast<int64_t&> (m_pSampleBuffer[s*m_ui32SampleSize + l_i32Offset]);
						l_f32Sample = static_cast<float32> (tmp) * m_vResolutionArray[i];
						l_i32Offset += 8;
						break;
					}
				case DT_UINT64:
					{
						uint64_t tmp = reinterpret_cast<uint64_t&> (m_pSampleBuffer[s*m_ui32SampleSize + l_i32Offset]);
						l_f32Sample = static_cast<float32> (tmp) * m_vResolutionArray[i];
						l_i32Offset += 8;
						break;
					}
				case DT_FLOAT64:
					{
						float64 tmp = reinterpret_cast<float64&> (m_pSampleBuffer[s*m_ui32SampleSize + l_i32Offset]);
						l_f32Sample = static_cast<float32> (tmp) * m_vResolutionArray[i];
						l_i32Offset += 8;
						break;					
					}					
				default:
					break;
			}
			
			extractData[s][i] = l_f32Sample;
		}
	}
}

void CDriverBrainProductsLiveAmp::liveAmpExtractImpedanceData(OpenViBE::int32 samplesRead, std::vector<std::vector<OpenViBE::float32>> &extractData)
{
	// extracts samples of each channel. In this case, since it is a impedance measurements it extracts EEG-channel data.
	int32 l_i32NumSamples = samplesRead / m_ui32SampleSize;
	int32 l_i32Offset = 0;
	int32 l_i32OffsetStep = sizeof(float32);

	extractData.clear();
	extractData.resize(l_i32NumSamples);

	for (int32 s = 0; s < l_i32NumSamples; s++)
	{		
		extractData[s].resize(m_ui32ImpedanceChannels);

		for (uint32 i=0; i < m_ui32ImpedanceChannels; i++)
		{
			float32 l_f32Samp = *(float32*)&m_pSampleBuffer[s*m_ui32SampleSize + l_i32Offset];
			extractData[s][i] = l_f32Samp;
			l_i32Offset += l_i32OffsetStep; // sample counter offset
		}
	}
}


#endif //  TARGET_HAS_ThirdPartyLiveAmpAPI