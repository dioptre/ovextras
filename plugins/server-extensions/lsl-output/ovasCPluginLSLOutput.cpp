
#if defined TARGET_HAS_ThirdPartyLSL

#include "ovasCPluginLSLOutput.h"

#include <vector>
#include <iostream>

#include <openvibe/ovITimeArithmetics.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

// #define boolean OpenViBE::boolean

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace OpenViBEAcquisitionServerPlugins;
using namespace std;

CPluginLSLOutput::CPluginLSLOutput(const IKernelContext& rKernelContext) :
	IAcquisitionServerPlugin(rKernelContext, CString("AcquisitionServer_Plugin_LSLOutput")),
	m_bIsLSLOutputEnabled(false),
	m_sLSLOutputStreamName("openvibeLSLOutput"),
	m_oSignalOutlet(NULL),
	m_oStimulusOutlet(NULL),
	m_ui32SampleCountPerSentBlock(0)
{
	m_rKernelContext.getLogManager() << LogLevel_Info << "Loading plugin: LSL Output\n";

	m_oSettingsHelper.add("EnableLSLOutput", &m_bIsLSLOutputEnabled);
	m_oSettingsHelper.add("LSLStreamName", &m_sLSLOutputStreamName);
	m_oSettingsHelper.load();

}

CPluginLSLOutput::~CPluginLSLOutput()
{
	if(m_oSignalOutlet) 
	{
		delete m_oSignalOutlet;
		m_oSignalOutlet = NULL;
	}
	if(m_oStimulusOutlet) 
	{
		delete m_oStimulusOutlet;
		m_oStimulusOutlet = NULL;
	}
}

// Hooks


void CPluginLSLOutput::startHook(const std::vector<OpenViBE::CString>& vSelectedChannelNames, OpenViBE::uint32 ui32SamplingFrequency, OpenViBE::uint32 ui32ChannelCount, OpenViBE::uint32 ui32SampleCountPerSentBlock)
{

	m_ui32SampleCountPerSentBlock = ui32SampleCountPerSentBlock;

	if (m_bIsLSLOutputEnabled)
	{
		// Open a signal stream 
		const CString l_sIdentifier = m_sLSLOutputStreamName + "Signal"; 
		lsl::stream_info l_oSignalInfo(m_sLSLOutputStreamName.toASCIIString(),"signal",ui32ChannelCount,ui32SamplingFrequency,lsl::cf_float32,l_sIdentifier.toASCIIString());

		lsl::xml_element l_oChannels = l_oSignalInfo.desc().append_child("channels");

		for (uint32 i=0;i<ui32ChannelCount;i++)
		{
			l_oChannels.append_child("channel")
				.append_child_value("label",vSelectedChannelNames[i])
				.append_child_value("unit","unknown")
				.append_child_value("type","signal");
		}

		// make a new outlet
		m_oSignalOutlet = new lsl::stream_outlet(l_oSignalInfo, m_ui32SampleCountPerSentBlock);

		// Open a stimulus stream
		const CString l_sStimulusIdentifier = m_sLSLOutputStreamName + "Markers"; 
		lsl::stream_info l_oStimulusInfo(l_sStimulusIdentifier.toASCIIString(),"Markers",1,lsl::IRREGULAR_RATE,lsl::cf_string,l_sStimulusIdentifier.toASCIIString());

		l_oStimulusInfo.desc().append_child("channels")
						.append_child("channel")
						.append_child_value("label","Stimulations")
						.append_child_value("type","marker");

		m_oStimulusOutlet = new lsl::stream_outlet(l_oStimulusInfo);

		m_rKernelContext.getLogManager() << LogLevel_Info << "LSL Output activated...\n";
	}
}

void CPluginLSLOutput::loopHook(std::vector < std::vector < OpenViBE::float32 > >& vPendingBuffer, CStimulationSet &stimulationSet, uint64 start, uint64 end)
{
	if (m_bIsLSLOutputEnabled)
	{
		// Output signal
		if(m_oSignalOutlet->have_consumers())
		{
			for(uint32 i=0;i<m_ui32SampleCountPerSentBlock;i++)
			{
				m_oSignalOutlet->push_sample(vPendingBuffer[i]);
			}
		}

		// Output stimuli
		if(m_oStimulusOutlet->have_consumers())
		{
			for(uint32 i=0;i<stimulationSet.getStimulationCount();i++)
			{	
				if(stimulationSet.getStimulationDate(i) >= start &&
				  stimulationSet.getStimulationDate(i) < end)
				{
					char l_sStimulationCode[128];
					sprintf(l_sStimulationCode, "%ld", stimulationSet.getStimulationIdentifier(i));

					const string l_sStimulationString(l_sStimulationCode);

					const float64 l_f64StimulationDate = ITimeArithmetics::timeToSeconds(stimulationSet.getStimulationDate(i));

					// std::cout << l_f64StimulationDate << "\n";

					m_oStimulusOutlet->push_sample(&l_sStimulationString,l_f64StimulationDate);
				}
			}
		}
	}

}

void CPluginLSLOutput::stopHook()
{
	if (m_bIsLSLOutputEnabled)
	{
		if(m_oSignalOutlet)
		{
			delete m_oSignalOutlet;
			m_oSignalOutlet = NULL;
		}
		if(m_oStimulusOutlet)
		{
			delete m_oStimulusOutlet;
			m_oStimulusOutlet = NULL;
		}
	}
}


#endif