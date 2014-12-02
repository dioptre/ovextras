/*
 * ovasCDriverBioSemiActiveTwo.cpp
 *
 * Copyright (c) 2012, Mensia Technologies SA. All rights reserved.
 * -- Rights transferred to Inria, contract signed 21.11.2014
 *
 */

#ifdef TARGET_HAS_ThirdPartyBioSemiAPI

#include "ovasCDriverBioSemiActiveTwo.h"
#include "ovasCConfigurationBioSemiActiveTwo.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#else
// linux, mac
#include <unistd.h>
#endif	// WIN32

#include <toolkit/ovtk_all.h>
#include <system/Time.h>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
#define boolean OpenViBE::boolean

//___________________________________________________________________//
//                                                                   //

CDriverBioSemiActiveTwo::CDriverBioSemiActiveTwo(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_BioSemiActiveTwo", m_rDriverContext.getConfigurationManager())
{
	m_oHeader.setSamplingFrequency(2048);

	// The amplifier can send up to 256+8 channels
	// as a request from BioSemi, we will make available the maximum channel count
	// User is able to select from 1 to MAX channels. If no data is present on the 
	// corresponding channels, zeros will be sent.
	// The number of channels present in the data flow will still be displayed in 
	// the driver configuration window. Previously selected value will be saved
	// with other settings.
	m_oHeader.setChannelCount(BIOSEMI_ACTIVETWO_MAXCHANNELCOUNT);

	m_vTriggers.resize(16,false);

	m_ui32LastWarningTime = 0;
	m_ui32StartTime = 0;
	m_bCMCurrentlyInRange = true;

	m_oSettings.add("Header", &m_oHeader);
	m_oSettings.load();
}

void CDriverBioSemiActiveTwo::release(void)
{
	delete this;
}

const char* CDriverBioSemiActiveTwo::getName(void)
{
	return "BioSemi Active Two (MkI and MkII)";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverBioSemiActiveTwo::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	if(m_rDriverContext.isConnected()) { return false; }
	
	m_pCallback=&rCallback;

	if(!m_oBridge.open())
	{
		m_rDriverContext.getLogManager() << LogLevel_Warning << "Could not open the device.\n";
		return false;
	}

	if(!m_oBridge.start())
	{
		m_rDriverContext.getLogManager() << LogLevel_Warning << "Could not start the device.\n";
		return false;
	}

	// wait to be sure we get the first packet from which we deduce the actual channel count and other initial configuration.
	System::Time::sleep(500);

	int32 l_i32ByteRead = m_oBridge.read();
	if(l_i32ByteRead < 0)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "An error occured while reading first data packet from device !\n";
		m_oBridge.close();
		return false;
	}
	
	if(!m_oBridge.discard()) // we discard the samples.
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "An error occured while dropping unused samples at initialization time.\n";
		m_oBridge.close();
		return false;
	}

	m_rDriverContext.getLogManager() << LogLevel_Trace << "Bridge initialized with: [SF:"<<m_oBridge.getSamplingFrequency()
		<< "] [CH:"<<m_oBridge.getChannelCount()
		<< "] [MKII:"<<m_oBridge.isDeviceMarkII()
		<< "] [CMInRange:"<<m_oBridge.isCMSInRange()
		<< "] [LowBat:"<<m_oBridge.isBatteryLow()<<"]\n";

	if(m_oBridge.isBatteryLow())
	{
		m_rDriverContext.getLogManager() << LogLevel_Warning << "Device battery is low !\n";
	}

	// the sample buffer is resized to get samples from ALL channels even if the user selected
	// less channels. We will adjust the content when calling setSamples(...)
	// in case the user required more channels than the number available in the stream, we will
	// add 0-padding.
	uint32 l_ui32ChannelCountInStream = m_oBridge.getChannelCount();
	uint32 l_ui32ChannelCountRequested = m_oHeader.getChannelCount();
	m_vSample.clear();
	m_vSample.resize(l_ui32ChannelCountRequested, 0);
	if(l_ui32ChannelCountRequested > l_ui32ChannelCountInStream)
	{
		m_rDriverContext.getLogManager() << LogLevel_Warning << "The required channel count cannot be reached in current device configuration (data stream contains "<<m_oBridge.getChannelCount()<<" channels). Please check the device speed mode and setup capabilities. Channels with no data will be filled with zeros.\n";
	}
	
	
	m_ui64SampleCount = 0;
	m_rDriverContext.getLogManager() << LogLevel_Trace << "Driver initialized...\n";
	
	return true;
}

boolean CDriverBioSemiActiveTwo::start(void)
{
	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }
	
	m_ui32StartTime = System::Time::getTime();
	m_ui32LastWarningTime = 0;

	m_rDriverContext.getLogManager() << LogLevel_Trace << "Acquisition started...\n";
	return true;
}

boolean CDriverBioSemiActiveTwo::loop(void)
{
	if(!m_rDriverContext.isConnected()) { return false; }

	if(m_rDriverContext.isStarted())
	{
		uint32 l_ui32ChannelCountInStream = m_oBridge.getChannelCount();
		uint32 l_ui32ChannelCountRequested = m_oHeader.getChannelCount();

		int32 l_i32ByteRead = m_oBridge.read();
		if(l_i32ByteRead > 0)
		{
			for(uint32 i = 0; i<m_oBridge.getAvailableSampleCount(); i++)
			{
				// we consume one sample per channel, values are given in uV
				if(!m_oBridge.consumeOneSamplePerChannel(&m_vSample[0], l_ui32ChannelCountInStream))
				{
					m_rDriverContext.getLogManager() << LogLevel_Error << "Something bad happened while consuming samples from device.\n";
					if(m_oBridge.getLastError() == Mensia::BioSemiError_SyncLost)
					{
						m_rDriverContext.getLogManager() << "\t > Synchronization lost during acquisition. Fiber optic may be loose or damaged.\n";
					}
					if(m_oBridge.getLastError() == Mensia::BioSemiError_BufferOverflow)
					{
						m_rDriverContext.getLogManager() << "\t > Buffer overflow. Please check that you have enough CPU and memory available to run the acquisition server at full speed before retrying.\n";
					}
					return false;
				}
				
				// this call uses the header's channel count, so it will naturally take the first samples (not necessarily all channels).
				m_pCallback->setSamples(&m_vSample[0],1);

				// triggers:
				// we simply send OVTK_StimulationId_Label_X where X is the trigger index between 1 and 16
				// We don't handle rising and falling edges.
				for(uint32 i=0; i<m_vTriggers.size(); i++)
				{
					if(m_oBridge.getTrigger(i) != m_vTriggers[i])
					{
						m_vTriggers[i] = m_oBridge.getTrigger(i);
						uint64 l_ui64Date = (1LL<<32) / m_oBridge.getSamplingFrequency(); // date is relative to the buffer start. I only have one sample in the buffer so it's fairly simple
						m_oStimulationSet.appendStimulation(OVTK_StimulationId_Label(i+1), l_ui64Date, 0);
						m_rDriverContext.getLogManager() << LogLevel_Trace << "Trigger "<<i+1<<"/16 has switched to " << m_vTriggers[i]<<"\n";
					}
				}

				// "CM in range" warning, once every 2 seconds max
				/*
				From: Coen (BioSemi):

				The current flow via the DRL is constantly monitored by the safety circuitry inside the AD-box. The current flow is limited to 50 uA (IEC 601 safety limit).
				If the current runs into the limit, the CMS/DRL circuit cannot keep the Common Mode value within its normal working range, and the blue LED turns off.
		
				The safety circuitry reacts on this error situation by shutting the power supply to ALL active electrodes off. Consequently, no meaningful signals can be measured so long as the blue LED is off. 
				The circuit operation described above implies that any electrode can be the cause of a CM out of range error. 
				Examples of errors are broken wires, bad connector contacts (pollution of connector with gel), defect IC inside the electrode, 
				bare electrode wire contacting the subject (damaged cable isolation) etc.. For example, if one of the active electrode wires is broken,
				the electrode input circuit is not anymore biased correctly, and the input resistance may fall below its specified value of 10^12 Ohm.
				The resultant extra input current is detected by the CMS/DRL circuit, and the blue LED goes off. 
				Save operation of the system is ensured because the power supply to the active electrodes is only restored if ALL electrodes connected to the subject work correctly.
				In other words, both cap en EX electrodes can in principle cause CM out of range errors. 
				*/

				boolean l_bWarningDisplayed = false;

				if (!m_oBridge.isCMSInRange())
				{
					// we print a warning message once every 2secs maximum
					if(System::Time::getTime() > m_ui32LastWarningTime + 2000)
					{
						l_bWarningDisplayed = true;

						m_rDriverContext.getLogManager() << LogLevel_Warning << "("<<((System::Time::getTime() - m_ui32StartTime)/1000)<<"') CM is not in range. For safety purpose, any active electrode connected has been shut down and signals should not be used.\n";
						m_rDriverContext.getLogManager() << "\t  The corresponding sample range has been tagged with OVTK_StimulationId_SegmentStart and OVTK_StimulationId_SegmentStop.\n";
						m_rDriverContext.getLogManager() << "\t  Possible causes include broken wires, damaged cable isolation, bad connector contacts, defect IC inside the electrode.\n";
					}

					// the possibly incorrect sample range is tagged using OVTK_StimulationId_SegmentStart and OVTK_StimulationId_SegmentStop
					// CM is now not in range
					if(m_bCMCurrentlyInRange)
					{
						uint64 l_ui64Date = (1LL<<32) / m_oBridge.getSamplingFrequency(); 
						m_oStimulationSet.appendStimulation(OVTK_StimulationId_SegmentStart, l_ui64Date, 0);
					}

					m_bCMCurrentlyInRange = false;
				}
				else
				{
					// CM is now back in range
					if(!m_bCMCurrentlyInRange)
					{
						uint64 l_ui64Date = (1LL<<32) / m_oBridge.getSamplingFrequency(); 
						m_oStimulationSet.appendStimulation(OVTK_StimulationId_SegmentStop, l_ui64Date, 0);
					}
					m_bCMCurrentlyInRange = true;
				}

				if(m_oBridge.isBatteryLow())
				{
					if(System::Time::getTime() > m_ui32LastWarningTime + 2000)
					{
						l_bWarningDisplayed = true;
						m_rDriverContext.getLogManager() << LogLevel_Warning << "("<<((System::Time::getTime() - m_ui32StartTime)/1000)<<"') Device battery is low !\n";
					}
				}

				if(l_bWarningDisplayed)
				{
					m_ui32LastWarningTime = System::Time::getTime();
				}

				m_pCallback->setStimulationSet(m_oStimulationSet);
				m_oStimulationSet.clear();
				
			}
		}
		if(l_i32ByteRead < 0)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "An error occured while reading data from device !\n";
			return false;
		}

		m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());
	}
	else
	{
		// acquisition is not started, but device is.
		int32 l_i32ByteRead = m_oBridge.read();
		while(l_i32ByteRead > 0)
		{
			if(!m_oBridge.discard()) // we discard the samples.
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "An error occured while dropping samples.\n";
				return false;
			}
			l_i32ByteRead = m_oBridge.read();
		}
		if(l_i32ByteRead < 0)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "An error occured while reading data from device (drop)!\n";
			return false;
		}
	}

	return true;
}

boolean CDriverBioSemiActiveTwo::stop(void)
{
	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return false; }

	m_rDriverContext.getLogManager() << LogLevel_Trace << "Acquisition stopped...\n";
	
	return true;
}

boolean CDriverBioSemiActiveTwo::uninitialize(void)
{
	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	if(!m_oBridge.close())
	{
		m_rDriverContext.getLogManager() << LogLevel_Warning << "Could not close the device.\n";
		return false;
	}

	m_rDriverContext.getLogManager() << LogLevel_Trace << "Driver uninitialized...\n";

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverBioSemiActiveTwo::isConfigurable(void)
{
	return true;
}

boolean CDriverBioSemiActiveTwo::configure(void)
{
	CConfigurationBioSemiActiveTwo m_oConfiguration(OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-BioSemi-ActiveTwo.ui");
	
	if(m_oConfiguration.configure(m_oHeader))
	{
		m_oSettings.save();
		return true;
	}

	return false;
}

#endif
