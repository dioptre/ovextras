#include "ovpCBoxAlgorithmNoiseGenerator.h"

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#include <openvibe/ovITimeArithmetics.h>
#include <system/ovCMath.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;
using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::DataGeneration;
using namespace OpenViBEToolkit;
using namespace std;

CNoiseGenerator::CNoiseGenerator(void)
	:
	m_bHeaderSent(false)
	,m_ui64ChannelCount(0)
	,m_ui64SamplingFrequency(0)
	,m_ui64GeneratedEpochSampleCount(0)
	,m_ui64SentSampleCount(0)
{
}

void CNoiseGenerator::release(void)
{
	delete this;
}

boolean CNoiseGenerator::initialize(void)
{
	m_oSignalEncoder.initialize(*this,0);

	m_ui64ChannelCount=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0); 
	m_ui64SamplingFrequency=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_ui64GeneratedEpochSampleCount=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	m_bHeaderSent=false;

	if(m_ui64ChannelCount == 0)
	{
		this->getLogManager() << LogLevel_Error << "Channel count is 0. At least 1 channel required. Check box settings.\n";
		return false;
	}

	if(m_ui64SamplingFrequency == 0)
	{
		this->getLogManager() << LogLevel_Error << "Sampling rate of 0 is not supported. Check box settings.\n";
		return false;
	}

	if(m_ui64GeneratedEpochSampleCount == 0)
	{
		this->getLogManager() << LogLevel_Error << "Epoch sample count is 0. An epoch must have at least 1 sample. Check box settings.\n";
		return false;
	}

	m_oSignalEncoder.getInputSamplingRate() = m_ui64SamplingFrequency;

	IMatrix* l_pSampleMatrix = m_oSignalEncoder.getInputMatrix();

	l_pSampleMatrix->setDimensionCount(2);
	l_pSampleMatrix->setDimensionSize(0,static_cast<uint32>(m_ui64ChannelCount));
	l_pSampleMatrix->setDimensionSize(1,static_cast<uint32>(m_ui64GeneratedEpochSampleCount));
	for(uint32 i=0;i<static_cast<uint32>(m_ui64ChannelCount);i++)
	{
		char l_sBuffer[64];
		sprintf(l_sBuffer, "Noise %d", i);
		l_pSampleMatrix->setDimensionLabel(0, i, l_sBuffer);
	}

	return true;
}

boolean CNoiseGenerator::uninitialize(void)
{
	m_oSignalEncoder.uninitialize();

	return true;
}

boolean CNoiseGenerator::processClock(CMessageClock& rMessageClock)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CNoiseGenerator::process(void)
{
	IBoxIO* l_pDynamicBoxContext=getBoxAlgorithmContext()->getDynamicBoxContext();

	if(!m_bHeaderSent)
	{
		m_oSignalEncoder.encodeHeader();

		uint64 l_ui64Time=ITimeArithmetics::sampleCountToTime(m_ui64SamplingFrequency, m_ui64SentSampleCount);
		l_pDynamicBoxContext->markOutputAsReadyToSend(0, l_ui64Time, l_ui64Time);

		m_bHeaderSent=true;
	}
	else
	{
		float64* l_pSampleBuffer = m_oSignalEncoder.getInputMatrix()->getBuffer();

		uint32 l_ui32SentSampleCount=(uint32)m_ui64SentSampleCount;
		for(uint32 i=0; i<(uint32)m_ui64ChannelCount; i++)
		{
			for(uint32 j=0; j<(uint32)m_ui64GeneratedEpochSampleCount; j++)
			{
				l_pSampleBuffer[i*m_ui64GeneratedEpochSampleCount+j]=(float64)System::Math::randomFloat32BetweenZeroAndOne();
			}
		}
		m_ui64SentSampleCount+=m_ui64GeneratedEpochSampleCount;

		uint64 l_ui64StartTime = ITimeArithmetics::sampleCountToTime(m_ui64SamplingFrequency, l_ui32SentSampleCount);
		uint64 l_ui64EndTime = ITimeArithmetics::sampleCountToTime(m_ui64SamplingFrequency, m_ui64SentSampleCount);
		
		m_oSignalEncoder.encodeBuffer();

		l_pDynamicBoxContext->markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);
	}

	return true;
}

OpenViBE::uint64 CNoiseGenerator::getClockFrequency(void) 
{
	// Intentional parameter swap to get the frequency
	return ITimeArithmetics::sampleCountToTime(m_ui64GeneratedEpochSampleCount, m_ui64SamplingFrequency);
}
