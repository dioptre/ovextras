#include "ovpCBoxAlgorithmNoiseGenerator.h"

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#include <openvibe/ovITimeArithmetics.h>
#include <system/CMath.h>

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
	m_pStreamEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamEncoder));
	if(!m_pStreamEncoder) {
		this->getLogManager() << LogLevel_Error << "Unable to get stream encoder.\n";
		return false;
	}

	m_pStreamEncoder->initialize();

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

	ip_ui64SignalSamplingRate.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_SignalStreamEncoder_InputParameterId_SamplingRate));
	ip_pSignalMatrix.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_SignalStreamEncoder_InputParameterId_Matrix));

	ip_ui64SignalSamplingRate = m_ui64SamplingFrequency;


	IMatrix &l_rSampleMatrix = *ip_pSignalMatrix;

	l_rSampleMatrix.setDimensionCount(2);
	l_rSampleMatrix.setDimensionSize(0,(uint32)m_ui64ChannelCount);
	l_rSampleMatrix.setDimensionSize(1,(uint32)m_ui64GeneratedEpochSampleCount);
	for(uint64 i=0;i<m_ui64ChannelCount;i++)
	{
		char l_sBuffer[64];
		sprintf(l_sBuffer, "Noise %d", i);
		l_rSampleMatrix.setDimensionLabel(0, static_cast<uint32>(i), l_sBuffer);
	}

	return true;
}

boolean CNoiseGenerator::uninitialize(void)
{
	m_pStreamEncoder->uninitialize();

	getAlgorithmManager().releaseAlgorithm(*m_pStreamEncoder);

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

	TParameterHandler < IMemoryBuffer* > l_oOutputMemoryBufferHandle(m_pStreamEncoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixStreamEncoder_OutputParameterId_EncodedMemoryBuffer));
	l_oOutputMemoryBufferHandle=l_pDynamicBoxContext->getOutputChunk(0);

	if(!m_bHeaderSent)
	{
		m_pStreamEncoder->process(OVP_GD_Algorithm_StreamedMatrixStreamEncoder_InputTriggerId_EncodeHeader);

		uint64 l_ui64Time=ITimeArithmetics::sampleCountToTime(m_ui64SamplingFrequency, m_ui64SentSampleCount);
		l_pDynamicBoxContext->markOutputAsReadyToSend(0, l_ui64Time, l_ui64Time);

		m_bHeaderSent=true;
	}
	else
	{
		IMatrix &l_rSampleMatrix = *ip_pSignalMatrix;
		float64* l_pSampleBuffer = l_rSampleMatrix.getBuffer();

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
		

		m_pStreamEncoder->process(OVP_GD_Algorithm_StreamedMatrixStreamEncoder_InputTriggerId_EncodeBuffer);

		l_pDynamicBoxContext->markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);
	}

	return true;
}

OpenViBE::uint64 CNoiseGenerator::getClockFrequency(void) 
{
	// Intentional parameter swap to get the frequency
	return ITimeArithmetics::sampleCountToTime(m_ui64GeneratedEpochSampleCount, m_ui64SamplingFrequency);
}
