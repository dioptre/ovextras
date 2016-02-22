#if defined TARGET_HAS_ThirdPartyUSBFirstAmpAPI

#include "ovasCDriverBrainProductsVAmp.h"
#include "ovasCConfigurationBrainProductsVAmp.h"
#include "ovasCHeaderBrainProductsVAmp.h"
#include <openvibe/ovITimeArithmetics.h>

#include <system/ovCTime.h>
#include <windows.h>
#include <FirstAmp.h>

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <vector>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

#define boolean OpenViBE::boolean
#define OVAS_Driver_VAmp_ImpedanceMask_BaseComponent 127

namespace
{
	// Low pass FIR filters before downsampling
	//
	// Computed with python and scipy
	// References :
	// http://docs.scipy.org/doc/numpy/reference/generated/numpy.ndarray.tofile.html
	// http://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.firwin.html
	//
	// The following filters have roughly 50ms delay as described in "2.1.4 What is the
	// delay of a linear-phase FIR?" at http://www.dspguru.com/book/export/html/3 :
	// n =   200, Fs =   2000, delay = (n-1)/(2*Fs) = 0.04975
	// n =  2000, Fs =  20000, delay = (n-1)/(2*Fs) = 0.049975
	//
	// In order to correct this delay, filtering should be done as a two steps process with a forward filter
	// followed by a backward filter. However, this leads to an n square complexity where a linear complexity is
	// sufficient in forward only filtering (using 100kHz input signal, n=10000 taps !)
	//
	// To avoid such complexity, it is chosen to antedate acquired samples by 50ms cheating the drift correction
	// process. Indeed, the acquisition server application monitors the drifting of the acquisition process and
	// corrects this drift upon demand. It is up to the driver to require this correction and it can be chosen not
	// to fit the 0 drift, but to fit an arbitrary fixed drift instead.
	//
	// The offset for this correction is stored in the m_i64DriftOffsetSampleCount variable

/* ---------------------------------------------------------------------------------------------------------------
from scipy import signal

N = 200
signal.firwin(N, cutoff= 256./(0.5*2000.), window='hamming').tofile("f64_2k_512.bin")
signal.firwin(N, cutoff= 128./(0.5*2000.), window='hamming').tofile("f64_2k_256.bin")
signal.firwin(N, cutoff=  64./(0.5*2000.), window='hamming').tofile("f64_2k_128.bin")

N = 2000
signal.firwin(N, cutoff=2048./(0.5*20000.), window='hamming').tofile("f64_20k_4096.bin")
signal.firwin(N, cutoff=1024./(0.5*20000.), window='hamming').tofile("f64_20k_2048.bin")
signal.firwin(N, cutoff= 512./(0.5*20000.), window='hamming').tofile("f64_20k_1024.bin")
signal.firwin(N, cutoff= 256./(0.5*20000.), window='hamming').tofile("f64_20k_512.bin")
signal.firwin(N, cutoff= 128./(0.5*20000.), window='hamming').tofile("f64_20k_256.bin")
signal.firwin(N, cutoff=  64./(0.5*20000.), window='hamming').tofile("f64_20k_128.bin")

--------------------------------------------------------------------------------------------------------------- */

	static bool loadFilter(const char* sFilename, std::vector < float64 >& vFilter)
	{
		FILE* l_pFile = ::fopen(sFilename, "rb");
		if(!l_pFile)
		{
			vFilter.clear();
			vFilter.push_back(1);
			return false;
		}
		else
		{
			::fseek(l_pFile, 0, SEEK_END);
			size_t len=::ftell(l_pFile);
			::fseek(l_pFile, 0, SEEK_SET);
			vFilter.resize(len/sizeof(float64));
			::fread(&vFilter[0], len, 1, l_pFile);
			::fclose(l_pFile);
		}
		return true;
	}
};


//___________________________________________________________________//
//                                                                   //

CDriverBrainProductsVAmp::CDriverBrainProductsVAmp(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_BrainProducts-VAmp", m_rDriverContext.getConfigurationManager())
	,m_bAcquireAuxiliaryAsEEG(false)
	,m_bAcquireTriggerAsEEG(false)
	,m_oHeader()
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_ui32TotalSampleCount(0)
	,m_bFirstStart(false)
	,m_ui32LastTrigger(0)
{
	// default mode is VAmp 16, aux and trigger depending on the config tokens
	m_oHeader.setAcquisitionMode(AcquisitionMode_VAmp16);

	m_ui32EEGChannelCount=m_oHeader.getEEGChannelCount(AcquisitionMode_VAmp16);
	m_ui32AuxiliaryChannelCount=(m_bAcquireAuxiliaryAsEEG ? m_oHeader.getAuxiliaryChannelCount(AcquisitionMode_VAmp16) : 0);
	m_ui32TriggerChannelCount=(m_bAcquireTriggerAsEEG ? m_oHeader.getTriggerChannelCount(AcquisitionMode_VAmp16) : 0);
	m_oHeader.setChannelCount(m_ui32EEGChannelCount + m_ui32AuxiliaryChannelCount + m_ui32TriggerChannelCount);

	if(m_bAcquireAuxiliaryAsEEG)
	{
		m_oHeader.setChannelName(m_ui32EEGChannelCount, "Aux 1");
		m_oHeader.setChannelName(m_ui32EEGChannelCount+1, "Aux 2");
	}
	if(m_bAcquireTriggerAsEEG)
	{
		m_oHeader.setChannelName(m_ui32EEGChannelCount+m_ui32AuxiliaryChannelCount, "Trigger line");
	}

	m_oHeader.setSamplingFrequency(512);

	t_faDataModeSettings l_tVamp4FastSettings;
	l_tVamp4FastSettings.Mode20kHz4Channels.ChannelsPos[0] = 7;
	l_tVamp4FastSettings.Mode20kHz4Channels.ChannelsNeg[0] = -1;
	l_tVamp4FastSettings.Mode20kHz4Channels.ChannelsPos[1] = 8;
	l_tVamp4FastSettings.Mode20kHz4Channels.ChannelsNeg[1] = -1;
	l_tVamp4FastSettings.Mode20kHz4Channels.ChannelsPos[2] = 9;
	l_tVamp4FastSettings.Mode20kHz4Channels.ChannelsNeg[2] = -1;
	l_tVamp4FastSettings.Mode20kHz4Channels.ChannelsPos[3] = 10;
	l_tVamp4FastSettings.Mode20kHz4Channels.ChannelsNeg[3] = -1;

	m_oHeader.setFastModeSettings(l_tVamp4FastSettings);
	m_oHeader.setDeviceId(FA_ID_INVALID);

	// @note m_oHeader is CHeaderBrainProductsVAmp, whereas the current interface supports only IHeader. Thus, some info may not be loaded/saved.
	m_oSettings.add("Header", &m_oHeader);
	m_oSettings.add("AcquireAuxiliaryAsEEG", &m_bAcquireAuxiliaryAsEEG);
	m_oSettings.add("AcquireTriggerAsEEG", &m_bAcquireTriggerAsEEG);
	m_oSettings.load();

}

CDriverBrainProductsVAmp::~CDriverBrainProductsVAmp(void)
{
}

const char* CDriverBrainProductsVAmp::getName(void)
{
	return "Brain Products V-Amp / First-Amp";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverBrainProductsVAmp::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	uint32 i;

	m_rDriverContext.getLogManager() << LogLevel_Trace << "INIT called.\n";
	if(m_rDriverContext.isConnected())
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] VAmp Driver: Driver already initialized.\n";
		return false;
	}

	if(m_bAcquireAuxiliaryAsEEG) m_rDriverContext.getLogManager() << LogLevel_Trace << "[INIT] VAmp Driver: will acquire aux as EEG\n";
	else                         m_rDriverContext.getLogManager() << LogLevel_Trace << "[INIT] VAmp Driver: will NOT acquire aux as EEG\n";
	if(m_bAcquireTriggerAsEEG) m_rDriverContext.getLogManager() << LogLevel_Trace << "[INIT] VAmp Driver: will acquire trigger as EEG\n";
	else                       m_rDriverContext.getLogManager() << LogLevel_Trace << "[INIT] VAmp Driver: will NOT acquire trigger as EEG\n";

	m_ui32AcquisitionMode=m_oHeader.getAcquisitionMode();
	m_ui32EEGChannelCount=m_oHeader.getEEGChannelCount(m_ui32AcquisitionMode);
	m_ui32AuxiliaryChannelCount=(m_bAcquireAuxiliaryAsEEG ? m_oHeader.getAuxiliaryChannelCount(m_ui32AcquisitionMode) : 0);
	m_ui32TriggerChannelCount=(m_bAcquireTriggerAsEEG ? m_oHeader.getTriggerChannelCount(m_ui32AcquisitionMode) : 0);

	m_oHeader.setChannelCount(m_ui32EEGChannelCount + m_ui32AuxiliaryChannelCount + m_ui32TriggerChannelCount);

	if(m_bAcquireAuxiliaryAsEEG)
	{
		if(::strlen(m_oHeader.getChannelName(m_ui32EEGChannelCount)) == 0 || ::strcmp(m_oHeader.getChannelName(m_ui32EEGChannelCount), "Trigger line") == 0)
		{
			m_oHeader.setChannelName(m_ui32EEGChannelCount, "Aux 1");
		}
		if(::strlen(m_oHeader.getChannelName(m_ui32EEGChannelCount+1)) == 0)
		{
			m_oHeader.setChannelName(m_ui32EEGChannelCount+1, "Aux 2");
		}
	}
	if(m_bAcquireTriggerAsEEG)
	{
		m_oHeader.setChannelName(m_ui32EEGChannelCount+m_ui32AuxiliaryChannelCount, "Trigger line");
	}

	if(!m_oHeader.isChannelCountSet()
	 ||!m_oHeader.isSamplingFrequencySet())
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] VAmp Driver: Channel count or frequency not set.\n";
		return false;
	}

	// Builds up physical sampling rate
	switch(m_ui32AcquisitionMode)
	{
		case AcquisitionMode_VAmp16:    m_ui32PhysicalSampleRateHz = 2000; break;
		case AcquisitionMode_VAmp8:     m_ui32PhysicalSampleRateHz = 2000; break;
		case AcquisitionMode_VAmp4Fast: m_ui32PhysicalSampleRateHz = 20000; break;
		default:
			m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Vamp Driver: Unsupported acquisition mode [" << m_ui32AcquisitionMode << "].\n";
			return false;
	}

	// Loading low pass filter for decimation
	m_rDriverContext.getLogManager() << LogLevel_Trace << "[INIT] Vamp Driver: Setting up the FIR filter for signal decimation (physical rate " << m_ui32PhysicalSampleRateHz << " > driver rate " << m_oHeader.getSamplingFrequency() << ").\n";
	switch(m_ui32PhysicalSampleRateHz)
	{
		case 2000:  loadFilter(OpenViBE::Directories::getDataDir() + "applications/acquisition-server/filters/f64_2k_512.bin", m_vFilter); break;
		case 20000: loadFilter(OpenViBE::Directories::getDataDir() + "applications/acquisition-server/filters/f64_20k_512.bin", m_vFilter); break;
		default:
			m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Vamp Driver: Unsupported physical sampling rate [" << m_ui32PhysicalSampleRateHz << "].\n";
			return false;
	}

	// Builds up a buffer to store acquired samples. This buffer will be sent to the acquisition server later.
	m_vSample.clear();
	m_vSample.resize(m_oHeader.getChannelCount());
	m_vResolution.clear();
	m_vResolution.resize(m_oHeader.getChannelCount());

	// Prepares cache for filtering
	m_vSampleCache.clear();
	for(i=0; i<m_vFilter.size(); i++)
	{
		m_vSampleCache.push_back(m_vSample);
	}

	// Setting the inner latency as described in the beginning of the file
	m_i64DriftOffsetSampleCount=int64(m_oHeader.getSamplingFrequency()*50)/1000;
	m_rDriverContext.setInnerLatencySampleCount(-m_i64DriftOffsetSampleCount);
	m_rDriverContext.getLogManager() << LogLevel_Trace << "Driver inner latency set to 50ms to compensate FIR filtering.\n";

	// Prepares downsampling
	m_ui64Counter=0;
	m_ui64CounterStep=(uint64(m_oHeader.getSamplingFrequency())<<32) / m_ui32PhysicalSampleRateHz;

	// Gets device Id
	int32 l_i32DeviceId = m_oHeader.getDeviceId();

	//__________________________________
	// Hardware initialization

	// if no device selected with the properties dialog
	// we take the last device connected
	if(l_i32DeviceId == FA_ID_INVALID)
	{
		// We try to get the last opened device,
		uint32 l_uint32LastOpenedDeviceID = faGetCount(); // Get the last opened Device id.

		if (l_uint32LastOpenedDeviceID == FA_ID_INVALID) // failed
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] VAmp Driver: faGetCount failed to get last opened device.\n";
			return false;
		}

		l_i32DeviceId = faGetId(l_uint32LastOpenedDeviceID -1);
		m_oHeader.setDeviceId(l_i32DeviceId);
	}

	if (l_i32DeviceId != FA_ID_INVALID)
	{
		m_rDriverContext.getLogManager() << LogLevel_Trace << "[INIT] VAmp Driver: Active device ID(" << m_oHeader.getDeviceId() << ").\n";
	}
	else
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] VAmp Driver: No device connected !\n";
		return false;
	}

	// Open the device.
	int32 l_int32OpenReturn = faOpen(l_i32DeviceId);
	if (l_int32OpenReturn != FA_ERR_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] VAmp Driver: faOpen(" << l_i32DeviceId << ") FAILED(" << l_int32OpenReturn << ").\n";
		return false;
	}

	if(m_ui32AcquisitionMode == AcquisitionMode_VAmp4Fast)
	{
		faSetDataMode(l_i32DeviceId, dm20kHz4Channels, &(m_oHeader.getFastModeSettings()));
	}
	else
	{
		faSetDataMode(l_i32DeviceId, dmNormal, NULL);
	}

	if(m_rDriverContext.isImpedanceCheckRequested())
	{
		faStart(l_i32DeviceId);
		faStartImpedance(l_i32DeviceId);
	}

	if(!m_rDriverContext.isImpedanceCheckRequested())
	{
		HBITMAP l_bitmap = (HBITMAP) LoadImage(NULL, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/vamp-standby.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		if(l_bitmap == NULL || faSetBitmap(m_oHeader.getDeviceId(),l_bitmap ) != FA_ERR_OK)
		{
			m_rDriverContext.getLogManager() << LogLevel_Warning << "[LOOP] VAmp Driver: BMP load failed.\n";
		}
	}

	
	// Gets properties
	t_faProperty l_oProperties;
	if(::faGetProperty(m_oHeader.getDeviceId(), &l_oProperties))
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not get properties - Got error \n";
		return false;
	}
	
	uint32 j=0;
	for(i=0; i<m_ui32EEGChannelCount; i++, j++) m_vResolution[j]=m_oHeader.getChannelGain(i)*l_oProperties.ResolutionEeg*1E6f; // converts to µV
	for(i=0; i<m_ui32AuxiliaryChannelCount; i++, j++) m_vResolution[j]=l_oProperties.ResolutionAux*1E6f; // converts to µV
	
	for(uint32 i=0; i<m_ui32EEGChannelCount; i++, j++)
	{
		m_oHeader.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);
	}
	for(i=0; i<m_ui32AuxiliaryChannelCount; i++, j++)
	{
		m_oHeader.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Micro); // converts to µV
	}

	//__________________________________
	// Saves parameters
	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;

	return true;
}

boolean CDriverBrainProductsVAmp::start(void)
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

	m_bFirstStart = true;
	uint32 l_uint32ErrorCode = FA_ERR_OK;
	int32 l_i32DeviceId = m_oHeader.getDeviceId();

	if(m_rDriverContext.isImpedanceCheckRequested())
	{
		faStopImpedance(l_i32DeviceId);
		// stops the impedance mode, but not the acquisition
	}
	else
	{
		// we did not start the acquisition yet, let's do it now.
		l_uint32ErrorCode = faStart(l_i32DeviceId);
	}

	if (l_uint32ErrorCode != FA_ERR_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[START] VAmp Driver: faStart FAILED(" << l_uint32ErrorCode << "). Closing device.\n";
		faClose(l_i32DeviceId);
		return false;
	}

	//The bonus...
	HBITMAP l_bitmap = (HBITMAP) LoadImage(NULL, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/vamp-acquiring.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if(l_bitmap == NULL || faSetBitmap(m_oHeader.getDeviceId(),l_bitmap ) != FA_ERR_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Warning << "[START] VAmp Driver: BMP load failed.\n";
	}

	return true;

}

boolean CDriverBrainProductsVAmp::loop(void)
{
	uint32 i, j;

	if(!m_rDriverContext.isConnected())
	{
		return false;
	}

	t_faDataModel16 l_DataBufferVAmp16; // buffer for the next block in normal mode
	uint32 l_uint32ReadLengthVAmp16 = sizeof(t_faDataModel16);

	t_faDataModel8 l_DataBufferVAmp8; // buffer for the next block in normal mode
	uint32 l_uint32ReadLengthVAmp8 = sizeof(t_faDataModel8);

	t_faDataFormatMode20kHz l_DataBufferVamp4Fast; // buffer for fast mode acquisition
	uint32 l_uint32ReadLengthVamp4Fast = sizeof(t_faDataFormatMode20kHz);
	
	unsigned int l_uiStatus = 0;

	int32 l_i32DeviceId = m_oHeader.getDeviceId();

	if(m_rDriverContext.isStarted())
	{
		uint32 l_i32ReceivedSamples=0;
#if DEBUG
		uint32 l_uint32ReadErrorCount = 0;
		uint32 l_uint32ReadSuccessCount = 0;
		uint32 l_uint32ReadZeroCount = 0;
#endif
		if(m_bFirstStart)
		{
			//empty buffer
			switch(m_ui32AcquisitionMode)
			{
				case AcquisitionMode_VAmp16:
					while(faGetData(l_i32DeviceId, &l_DataBufferVAmp16, l_uint32ReadLengthVAmp16) > 0);
					l_uiStatus = l_DataBufferVAmp16.Status;
					break;
				case AcquisitionMode_VAmp8:
					while(faGetData(l_i32DeviceId, &l_DataBufferVAmp8, l_uint32ReadLengthVAmp8) > 0);
					l_uiStatus = l_DataBufferVAmp8.Status;
					break;
				case AcquisitionMode_VAmp4Fast:
					while(faGetData(l_i32DeviceId, &l_DataBufferVamp4Fast, l_uint32ReadLengthVamp4Fast) > 0);
					l_uiStatus = l_DataBufferVamp4Fast.Status;
					break;
			}
			// Trigger: Digital inputs (bits 0 - 8) + output (bit 9) state + 22 MSB reserved bits
			// The trigger value received is in [0-255]
			l_uiStatus &= 0x000000ff;
			m_ui32LastTrigger = l_uiStatus;
			m_bFirstStart = false;
		}

		boolean l_bFinished = false;
		while(!l_bFinished)
		{
			// we need to "getData" with the right output structure according to acquisition mode

			int32 l_i32ReturnLength = 0;
			signed int* l_pEEGArray=NULL;
			signed int* l_pAuxiliaryArray=NULL;
			unsigned int l_uiStatus=0;
			switch(m_ui32AcquisitionMode)
			{
				case AcquisitionMode_VAmp16:
					l_i32ReturnLength = faGetData(l_i32DeviceId, &l_DataBufferVAmp16, l_uint32ReadLengthVAmp16);
					l_pEEGArray=l_DataBufferVAmp16.Main;
					l_pAuxiliaryArray=l_DataBufferVAmp16.Aux;
					l_uiStatus=l_DataBufferVAmp16.Status;
					break;

				case AcquisitionMode_VAmp8:
					l_i32ReturnLength = faGetData(l_i32DeviceId, &l_DataBufferVAmp8, l_uint32ReadLengthVAmp8);
					l_pEEGArray=l_DataBufferVAmp8.Main;
					l_pAuxiliaryArray=l_DataBufferVAmp8.Aux;
					l_uiStatus=l_DataBufferVAmp8.Status;
					break;

				case AcquisitionMode_VAmp4Fast:
					l_i32ReturnLength = faGetData(l_i32DeviceId, &l_DataBufferVamp4Fast, l_uint32ReadLengthVamp4Fast);
					l_pEEGArray=l_DataBufferVamp4Fast.Main;
					// l_pAuxiliaryArray=l_DataBufferVamp4Fast.Aux;
					l_uiStatus=l_DataBufferVamp4Fast.Status;
					break;

				default:
					m_rDriverContext.getLogManager() << LogLevel_ImportantWarning << "[LOOP] VAmp Driver: unsupported acquisition mode, this should never happen\n";
					return false;
					break;
			}
			// Trigger: Digital inputs (bits 0 - 8) + output (bit 9) state + 22 MSB reserved bits
			// The trigger value received is in [0-255]
			l_uiStatus &= 0x000000ff;

			if(l_i32ReturnLength > 0)
			{
#if DEBUG
				l_uint32ReadSuccessCount++;
#endif

#if 0
				for(i=0; i < m_ui32EEGChannelCount; i++)
				{
					m_pSample[i*m_ui32SampleCountPerSentBlock+l_i32ReceivedSamples] = (float32)(l_pEEGArray[i]*m_oHeader.getChannelGain(i));
				}
				for(i=0; i < m_ui32AuxiliaryChannelCount; i++)
				{
					m_pSample[(m_ui32EEGChannelCount+i)*m_ui32SampleCountPerSentBlock+l_i32ReceivedSamples] = (float32)(l_pAuxiliaryArray[i]);
				}
				for(i=0; i < m_ui32TriggerChannelCount; i++)
				{
					m_pSample[(m_ui32EEGChannelCount+m_ui32AuxiliaryChannelCount+i)*m_ui32SampleCountPerSentBlock+l_i32ReceivedSamples] = (float32)(l_uiStatus);
				}
#else
				// Stores acquired sample
				for(i=0; i<m_ui32EEGChannelCount; i++)
				{
					m_vSample[i] = (float32)((l_pEEGArray[i] - l_pEEGArray[m_ui32EEGChannelCount]) * m_vResolution[i]) ;
				}
				for(i=0; i<m_ui32AuxiliaryChannelCount; i++)
				{
					m_vSample[m_ui32EEGChannelCount + i] = (float32)(l_pAuxiliaryArray[i] * m_vResolution[i]);
				}
				for(i=0; i<m_ui32TriggerChannelCount; i++)
				{
					m_vSample[m_ui32EEGChannelCount+m_ui32AuxiliaryChannelCount+i] = (float32) l_uiStatus;
				}

				// Updates cache
				//m_vSampleCache.erase(m_vSampleCache.begin());
				m_vSampleCache.pop_front();
				m_vSampleCache.push_back(m_vSample);

				// Every time that a trigger has changed, we send a stimulation.
				if(l_uiStatus != m_ui32LastTrigger)
				{
					// The date is relative to the last buffer start time (cf the setSamples before setStimulationSet)
					uint64 l_ui64Date = ITimeArithmetics::sampleCountToTime(m_ui32PhysicalSampleRateHz, m_ui32TotalSampleCount);
					// Code of stimulation = OVTK_StimulationId_LabelStart + value of the trigger bytes.
					m_oStimulationSet.appendStimulation(OVTK_StimulationId_Label(l_uiStatus), 0, 0);
					m_rDriverContext.getLogManager() << LogLevel_Debug << "[LOOP] VAmp Driver: Send stimulation: "<< l_uiStatus <<" at date: "<<l_ui64Date<<".\n";
					m_ui32LastTrigger = l_uiStatus;
				}

				m_ui64Counter+=m_ui64CounterStep;
				if(m_ui64Counter>=(1LL<<32))
				{
					m_ui64Counter-=(1LL<<32);

					// Filters last samples
					std::deque<std::vector < OpenViBE::float32 > >::iterator it;
					for(i=0; i<m_vSample.size(); i++)
					{
						m_vSample[i]=0;
						it = m_vSampleCache.begin();
						for(j=0; j<m_vFilter.size(); j++)
						{
							//m_vSample[i]+=m_vFilter[j]*m_vSampleCache[j][i];
							m_vSample[i]+=(float32)(m_vFilter[j]*(*it)[i]);
							it++;
						}
					}

					m_pCallback->setSamples(&m_vSample[0], 1);
					m_pCallback->setStimulationSet(m_oStimulationSet);
					m_oStimulationSet.clear();
				}
#endif

				l_i32ReceivedSamples++;
			}
			else if(l_i32ReturnLength==0)
			{
				l_bFinished = true;
				System::Time::sleep(2);
			}
#if DEBUG
			if(l_i32ReturnLength < 0)
			{
				l_uint32ReadErrorCount++;
				l_bFinished = true;
			}
			if(l_i32ReturnLength == 0)
			{
				l_uint32ReadZeroCount++;
				l_bFinished = true;
			}
#endif
		}
#if DEBUG
		m_rDriverContext.getLogManager() << LogLevel_Debug << "[LOOP] VAmp Driver: stats for the current block : Success="<<l_uint32ReadSuccessCount<<" Error="<<l_uint32ReadErrorCount<<" Zero="<<l_uint32ReadZeroCount<<"\n";
#endif
		//____________________________

#if 0
		m_pCallback->setSamples(m_pSample);
#endif

		// As described in at the begining of this file, the drift
		// correction process is altered to consider the ~ 50ms delay
		// of the single way filtering process
		// Inner latency was set accordingly.
		m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());
	}
	else
	{
		// we drop any data in internal buffer
		switch(m_ui32AcquisitionMode)
		{
			case AcquisitionMode_VAmp16:
				while(faGetData(l_i32DeviceId, &l_DataBufferVAmp16, l_uint32ReadLengthVAmp16) > 0);
				break;
			case AcquisitionMode_VAmp8:
				while(faGetData(l_i32DeviceId, &l_DataBufferVAmp8, l_uint32ReadLengthVAmp8) > 0);
				break;
			case AcquisitionMode_VAmp4Fast:
				while(faGetData(l_i32DeviceId, &l_DataBufferVamp4Fast, l_uint32ReadLengthVamp4Fast) > 0);
				break;
		}

		if(m_rDriverContext.isImpedanceCheckRequested())
		{
			// Reads impedances
			vector<unsigned int> l_vImpedanceBuffer;
			l_vImpedanceBuffer.resize(20, 0); // all possible channels + ground electrode
			uint32 l_uint32ErrorCode = faGetImpedance(l_i32DeviceId,&l_vImpedanceBuffer[0], 20*sizeof(unsigned int));
			if(l_uint32ErrorCode != FA_ERR_OK)
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "[LOOP] Can not read impedances on device id " << l_i32DeviceId << " - faGetImpedance FAILED(" << l_uint32ErrorCode << ")\n";
			}
			else
			{
				// Updates impedances

				uint64 l_ui64GoodImpedanceLimit = m_rDriverContext.getConfigurationManager().expandAsUInteger("${AcquisitionServer_DefaultImpedanceLimit}", 5000);
				// as with the default acticap settings (values provided by Brain Products) :
				uint64 l_ui64BadImpedanceLimit  = 2*l_ui64GoodImpedanceLimit;

				m_rDriverContext.getLogManager() << LogLevel_Debug << "Impedances are [ ";
				for(uint32 j=0; j<m_ui32EEGChannelCount; j++)//we do not update the last impedance (ground)
				{
					m_rDriverContext.updateImpedance(j, l_vImpedanceBuffer[j]);
					m_rDriverContext.getLogManager() << uint32(l_vImpedanceBuffer[j]) << " ";
				}
				m_rDriverContext.getLogManager() << "]\n";

				//print impedances on the LCD screen
				HBITMAP l_pBitmapHandler = (HBITMAP) LoadImage(NULL, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/vamp-impedance-mask.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
				if(l_pBitmapHandler)
				{
					BITMAP l_oBitmap;
					GetObject(l_pBitmapHandler, sizeof(l_oBitmap), &l_oBitmap);
					uint32 l_ui32Width = l_oBitmap.bmWidth;
					uint32 l_ui32Height = l_oBitmap.bmHeight;
					BYTE* l_pBytePtr = (BYTE*)l_oBitmap.bmBits; // 3 bytes per pixel for RGB values

					//printf("H x W : %u x %u with %u per scan\n",l_ui32Height, l_ui32Width, l_oBitmap.bmWidthBytes);

					uint32 x, y; // pixels coordinates
					uint32 byte_idx; // for each pixels, 3 RGB bytes
					uint32 channel_idx; // 1-based
					for(y = 0; y < l_ui32Height; y++) // scan lines
					{
						x = 0;
						unsigned char * R, * G,* B;
						for(byte_idx = 0; byte_idx < static_cast<uint32>(l_oBitmap.bmWidthBytes); byte_idx+=3)
						{
							x++;
							B = (l_pBytePtr + l_oBitmap.bmWidthBytes*y + byte_idx);
							G = (l_pBytePtr + l_oBitmap.bmWidthBytes*y + byte_idx +1);
							R = (l_pBytePtr + l_oBitmap.bmWidthBytes*y + byte_idx +2);
							// The impedance mask is a BLACK and WHITE bitmap with grey squares for the channels.
							// For each channel, the grey RGB components are equal to OVAS_Driver_VAmp_ImpedanceMask_BaseComponent - channel index - 1
							// e.g. with 16 channels and base at 127, the 5th channel square as RGB components to 127 - 5 = 122
							// Every pixel in the grey range is a channel square and needs repaint according to impedance.
							if(    *R < OVAS_Driver_VAmp_ImpedanceMask_BaseComponent && *R >= OVAS_Driver_VAmp_ImpedanceMask_BaseComponent - m_ui32EEGChannelCount
								&& *G < OVAS_Driver_VAmp_ImpedanceMask_BaseComponent && *G >= OVAS_Driver_VAmp_ImpedanceMask_BaseComponent - m_ui32EEGChannelCount
								&& *B < OVAS_Driver_VAmp_ImpedanceMask_BaseComponent && *B >= OVAS_Driver_VAmp_ImpedanceMask_BaseComponent - m_ui32EEGChannelCount)
							{
								channel_idx = OVAS_Driver_VAmp_ImpedanceMask_BaseComponent - *R - 1; //0-based
								if(l_vImpedanceBuffer[channel_idx] <= l_ui64GoodImpedanceLimit)
								{
									// good impedance: green
									*R = 0x00;
									*G = 0xff;
									*B = 0x00;
								}
								else if(l_vImpedanceBuffer[channel_idx] <= l_ui64BadImpedanceLimit)
								{
									// unsufficient impedance: yellow
									*R = 0xff;
									*G = 0xff;
									*B = 0x00;
								}
								else
								{
									// bad impedance: red
									*R = 0xff;
									*G = 0x00;
									*B = 0x00;
								}
							}
						}
					}
				}

				if(l_pBitmapHandler == NULL || faSetBitmap(m_oHeader.getDeviceId(),l_pBitmapHandler ) != FA_ERR_OK)
				{
					m_rDriverContext.getLogManager() << LogLevel_Warning << "[LOOP] VAmp Driver: BMP load failed.\n";
				}

			}
		}
	}

	return true;
}

boolean CDriverBrainProductsVAmp::stop(void)
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

	if(m_rDriverContext.isImpedanceCheckRequested())
	{
		faStartImpedance(m_oHeader.getDeviceId());
	}

	m_bFirstStart = false;
	if(!m_rDriverContext.isImpedanceCheckRequested())
	{
		HBITMAP l_bitmap = (HBITMAP) LoadImage(NULL, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/vamp-standby.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		if(l_bitmap == NULL || faSetBitmap(m_oHeader.getDeviceId(),l_bitmap ) != FA_ERR_OK)
		{
			m_rDriverContext.getLogManager() << LogLevel_Warning << "[STOP] VAmp Driver: BMP load failed.\n";
		}
	}

	return true;
}

boolean CDriverBrainProductsVAmp::uninitialize(void)
{
	if(!m_rDriverContext.isConnected())
	{
		return false;
	}

	if(m_rDriverContext.isStarted())
	{
		return false;
	}

	uint32 l_uint32ErrorCode = faStop(m_oHeader.getDeviceId());
	if (l_uint32ErrorCode != FA_ERR_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[UINIT] VAmp Driver: faStop FAILED(" << l_uint32ErrorCode << ").\n";
		faClose(m_oHeader.getDeviceId());
		return false;
	}

	m_rDriverContext.getLogManager() << LogLevel_Trace << "Uninitialize called. Closing the device.\n";


	// for a black bitmap use :
	//HDC l_hDC = CreateCompatibleDC(NULL);
	//HBITMAP l_bitmap = CreateCompatibleBitmap(l_hDC, 320, 240);
	// Default bitmap : BP image (provided by N. Soldati)
	HBITMAP l_bitmap = (HBITMAP) LoadImage(NULL, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/vamp-default.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if(faSetBitmap(m_oHeader.getDeviceId(), l_bitmap ) != FA_ERR_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Warning << "[UINIT] VAmp Driver: BMP load failed.\n";
	}


	l_uint32ErrorCode = faClose(m_oHeader.getDeviceId());
	if (l_uint32ErrorCode != FA_ERR_OK)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[UINIT] VAmp Driver: faClose FAILED(" << l_uint32ErrorCode << ").\n";
		return false;
	}

	m_pCallback=NULL;
	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverBrainProductsVAmp::isConfigurable(void)
{
	return true;
}

boolean CDriverBrainProductsVAmp::configure(void)
{
	CConfigurationBrainProductsVAmp m_oConfiguration(m_rDriverContext, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-BrainProducts-VAmp.ui", &m_oHeader, m_bAcquireAuxiliaryAsEEG, m_bAcquireTriggerAsEEG); // the specific header is passed into the specific configuration

	if(!m_oConfiguration.configure(*(m_oHeader.getBasicHeader()))) // the basic configure will use the basic header
	{
		return false;
	}

	m_oSettings.save();

	if(m_ui32AcquisitionMode == AcquisitionMode_VAmp4Fast)
	{
		m_rDriverContext.getLogManager() << LogLevel_Trace << "Pair names :\n";
		for(uint32 i = 0; i < m_oHeader.getPairCount();i++)
		{
			m_rDriverContext.getLogManager() << LogLevel_Trace << "  Pair " << i << " > " << m_oHeader.getPairName(i) << "\n";
		}
	}

	return true;
}

#endif // TARGET_HAS_ThirdPartyUSBFirstAmpAPI
