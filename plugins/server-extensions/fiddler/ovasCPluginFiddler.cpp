
/*
 * Linearly superposes a simple phase-locked template around 300ms after OVTK_StimulationId_Target
 *
 * Fiddler can be used to debug P300 scenarios. Note that the same effect could be 
 * achieved with a box that tampers the signal after specific stimulation markers.
 * It can also be used as an example of how to manipulate the signal on the 
 * acquisition server side using a plugin.
 *
 * Known limitations: Overlapping patterns not handled
 *
 */

#include "ovasCPluginFiddler.h"

#include <vector>
#include <iostream>

#include <stdio.h>  // sprintf on Linux

#include <openvibe/ovITimeArithmetics.h>
#include <system/ovCMath.h>

#include <cmath>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace OpenViBEAcquisitionServerPlugins;
using namespace std;

CPluginFiddler::CPluginFiddler(const IKernelContext& rKernelContext) :
	IAcquisitionServerPlugin(rKernelContext, CString("AcquisitionServer_Plugin_Fiddler")),
	m_f32FiddlerStrength(0),
	m_ui32SampleCountPerSentBlock(0)
{
	m_rKernelContext.getLogManager() << LogLevel_Info << "Loading plugin: Fiddler\n";

	m_oSettingsHelper.add("Fiddler_Strength",  &m_f32FiddlerStrength); // The amplitude of the pattern, 0 = disable
	m_oSettingsHelper.load();

}

CPluginFiddler::~CPluginFiddler()
{

}

// Hooks


void CPluginFiddler::startHook(const std::vector<OpenViBE::CString>& vSelectedChannelNames, uint32_t ui32SamplingFrequency, uint32_t ui32ChannelCount, uint32_t ui32SampleCountPerSentBlock)
{
	if (m_f32FiddlerStrength>10e-06)
	{
		m_ui32SampleCountPerSentBlock = ui32SampleCountPerSentBlock;

		m_ui64StartSample = std::numeric_limits<uint64_t>::max();
		m_ui64EndSample = 0;
		m_ui64ProcessedSampleCount = 0;
		m_ui64Counter = 0;

		m_ui32SamplingFrequency = ui32SamplingFrequency;

		m_ui64LastPendingBufferSize = 0;

		m_rKernelContext.getLogManager() << LogLevel_Warning << "Fiddler is enabled! Only use for debug purposes. Set strength=0 to disable.\n";
	}

}

void CPluginFiddler::loopHook(std::deque < std::vector < OpenViBE::float32 > >& vPendingBuffer, CStimulationSet &stimulationSet, uint64_t start, uint64_t end, uint64_t /* sampleTime */)
{
	if (m_f32FiddlerStrength>10e-06)
	{
		// We need to make sure we don't process the same samples twice
		uint64_t processedSamplesInBuffer = 0;
		if(m_ui64LastPendingBufferSize>m_ui32SampleCountPerSentBlock)
		{
			processedSamplesInBuffer = m_ui64LastPendingBufferSize - m_ui32SampleCountPerSentBlock;
		}

		// Loop over the stimulations
		for(size_t i=0;i<stimulationSet.getStimulationCount();i++)
		{
			const uint64_t l_oStimulationId = stimulationSet.getStimulationIdentifier(i);
			const uint64_t l_ui64StimulationTime = stimulationSet.getStimulationDate(i);
			const uint64_t l_ui64StimulationSample = ITimeArithmetics::timeToSampleCount(m_ui32SamplingFrequency, l_ui64StimulationTime);

			if(l_oStimulationId == OVTK_StimulationId_Target && l_ui64StimulationSample > m_ui64ProcessedSampleCount)
			{
				m_ui64StartSample = l_ui64StimulationSample + ITimeArithmetics::timeToSampleCount(m_ui32SamplingFrequency, ITimeArithmetics::secondsToTime(0.0));
				m_ui64EndSample = l_ui64StimulationSample + ITimeArithmetics::timeToSampleCount(m_ui32SamplingFrequency, ITimeArithmetics::secondsToTime(0.5));

				m_ui64Counter = 0;
			} 
		}

		for(size_t i=static_cast<size_t>(processedSamplesInBuffer);i<vPendingBuffer.size();i++)
		{
			// std::cout << "Sample " << ITimeArithmetics::timeToSeconds(l_ui64SampleTime) << " range " << ITimeArithmetics::timeToSeconds(m_ui64StartTime) << " stop " <<  ITimeArithmetics::timeToSeconds(m_ui64EndTime);

			if(m_ui64ProcessedSampleCount > m_ui64StartSample 
				&& m_ui64ProcessedSampleCount <= m_ui64EndSample)
			{
				const float32 lobe1 = 0.25f;		    // Position, in seconds
				const float32 lobe2 = 0.30f;
				const float32 spread1 = 0.008f;         // Width
				const float32 spread2 = 0.004f;

				// Two beta functions weighted by gaussians; the beta parameters are ^1 and ^4
				// n.b. not terribly efficient as the same pattern is created every time anew. In practice this is of little importance.
				const float32 st = static_cast<float32>(ITimeArithmetics::timeToSeconds(ITimeArithmetics::sampleCountToTime(m_ui32SamplingFrequency, m_ui64Counter)));
				const float32 bump1 = std::exp(-std::pow(st-lobe1, 2)/spread1)*(st*std::pow(1-st, 4));
				const float32 bump2 = std::exp(-std::pow(st-lobe2, 2)/spread2)*(st*std::pow(1-st, 4));
				const float32 value = (-0.5f*bump1 + 0.9f*bump2)*40.0f;

				std::vector<float32>& sample = vPendingBuffer[i];
				float32* tmp = &sample[0];
				for(size_t j=0;j<sample.size();j++)
				{
					tmp[j] += value*m_f32FiddlerStrength;
				}
				m_ui64Counter++;
			}

			m_ui64ProcessedSampleCount++;
		}

		m_ui64LastPendingBufferSize = vPendingBuffer.size();
	}
}

void CPluginFiddler::stopHook()
{

}

