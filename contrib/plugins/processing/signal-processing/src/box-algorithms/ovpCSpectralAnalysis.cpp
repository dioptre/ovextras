#if defined TARGET_HAS_ThirdPartyITPP

#include "ovpCSpectralAnalysis.h"

#include <iostream>
#include <sstream>

#include <itpp/itstat.h>
#include <itpp/itsignal.h>

using namespace itpp;
using namespace OpenViBE;
using namespace OpenViBE::Plugins;
using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessingGpl;
using namespace OpenViBEToolkit;
using namespace std;
using namespace OpenViBE::Kernel;

CSpectralAnalysis::CSpectralAnalysis(void)
	:m_ui64LastChunkStartTime(0),
	m_ui64LastChunkEndTime(0),
	m_ui32ChannelCount(0),
	m_ui32SampleCount(0),
	m_bCoefComputed(false),
	m_ui32FFTSize(1),
	m_ui32HalfFFTSize(1)

{
}

void CSpectralAnalysis::release(void)
{
	delete this;
}

boolean CSpectralAnalysis::initialize()
{
	//reads the plugin settings
	CString l_sSpectralComponents;
	getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(0, l_sSpectralComponents);
	uint64 l_ui64SpectralComponents=this->getTypeManager().getBitMaskEntryCompositionValueFromName(OVP_TypeId_SpectralComponent, l_sSpectralComponents);

	m_bAmplitudeSpectrum = ((l_ui64SpectralComponents & OVP_TypeId_SpectralComponent_Amplitude.toUInteger())>0);
	m_bPhaseSpectrum     = ((l_ui64SpectralComponents & OVP_TypeId_SpectralComponent_Phase.toUInteger())>0);
	m_bRealPartSpectrum  = ((l_ui64SpectralComponents & OVP_TypeId_SpectralComponent_RealPart.toUInteger())>0);
	m_bImagPartSpectrum  = ((l_ui64SpectralComponents & OVP_TypeId_SpectralComponent_ImaginaryPart.toUInteger())>0);



	m_pSignalDecoder = new OpenViBEToolkit::TSignalDecoder < CSpectralAnalysis >(*this,0);
	for(uint32 i=0; i<4; i++)
	{
		m_vpSpectrumEncoder.push_back(new OpenViBEToolkit::TSpectrumEncoder < CSpectralAnalysis >(*this,i));
	}

	return true;
}

boolean CSpectralAnalysis::uninitialize()
{
	m_pSignalDecoder->uninitialize();
	delete m_pSignalDecoder;

	for(uint32 i=0; i<4; i++)
	{
		m_vpSpectrumEncoder[i]->uninitialize();
	}
	m_vpSpectrumEncoder.clear();

	return true;
}

boolean CSpectralAnalysis::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CSpectralAnalysis::process()
{
	IBoxIO * l_pDynamicContext = getBoxAlgorithmContext()->getDynamicBoxContext();
	uint32 l_ui32InputChunkCount = l_pDynamicContext->getInputChunkCount(0);
	float64 l_float64BandStart, l_float64BandStop;
	char l_sFrequencyBandName [1024];

	for(uint32 chunkIdx = 0; chunkIdx < l_ui32InputChunkCount; chunkIdx++)
    {
		m_ui64LastChunkStartTime=l_pDynamicContext->getInputChunkStartTime(0, chunkIdx);
		m_ui64LastChunkEndTime=l_pDynamicContext->getInputChunkEndTime(0, chunkIdx);

		m_pSignalDecoder->decode(chunkIdx);

		if(m_pSignalDecoder->isHeaderReceived())//dealing with the signal header
		{
			//we need two matrices for the spectrum encoders, the Frequency bands and the one inherited form streamed matrix (see doc for details)
			CMatrix* l_pFrequencyBands = new CMatrix();
			CMatrix* l_pStreamedMatrix = new CMatrix();
			l_pFrequencyBands->setDimensionCount(2);
			//get signal info
			m_ui32SampleCount = m_pSignalDecoder->getOutputMatrix()->getDimensionSize(1);
			m_ui32ChannelCount = m_pSignalDecoder->getOutputMatrix()->getDimensionSize(0);
			m_ui32SamplingRate = m_pSignalDecoder->getOutputSamplingRate();

			if (!m_bCoefComputed)
			{
				m_ui32FFTSize=1;
				m_ui32HalfFFTSize=1;
				// Find out a power of two thats >= m_ui32SampleCount
				while (m_ui32FFTSize < m_ui32SampleCount)
				{
					m_ui32FFTSize<<=1;
				}
				if(m_ui32FFTSize==1)
				{
					this->getLogManager() << LogLevel_Warning << "Computing FFT with size = 1\n";
				}
				// Due to the inputs being real numbers, we only need to look at half of the complex output later
				m_ui32HalfFFTSize = m_ui32FFTSize>>1;
				m_bCoefComputed = true;
			}

			m_ui32FrequencyBandCount = m_ui32HalfFFTSize;

			OpenViBEToolkit::Tools::MatrixManipulation::copyDescription(*l_pStreamedMatrix, *m_pSignalDecoder->getOutputMatrix());
			l_pStreamedMatrix->setDimensionSize(1,m_ui32FrequencyBandCount);
			l_pFrequencyBands->setDimensionSize(0,2);
			l_pFrequencyBands->setDimensionSize(1,m_ui32FrequencyBandCount);
			float64* l_pBuffer = l_pFrequencyBands->getBuffer();

			for (uint32 j=0; j < m_ui32FrequencyBandCount; j++)
			{
				l_float64BandStart = static_cast<float64>(j*((double)(m_ui32SamplingRate/2)/m_ui32FrequencyBandCount));
				l_float64BandStop = static_cast<float64>((j+1)*((double)(m_ui32SamplingRate/2)/m_ui32FrequencyBandCount));
				if (l_float64BandStop <l_float64BandStart )
				{
					l_float64BandStop = l_float64BandStart;
				}

				*(l_pBuffer+2*j) = l_float64BandStart;
				*(l_pBuffer+2*j+1) = l_float64BandStop;


				sprintf(l_sFrequencyBandName, "%lg-%lg", l_float64BandStart, l_float64BandStop);
				l_pStreamedMatrix->setDimensionLabel(1,j,l_sFrequencyBandName);//set the names of the frequency bands
			}

			for(uint32 i=0;i<4;i++)
			{
				//copy the information for each encoder
				OpenViBEToolkit::Tools::MatrixManipulation::copy(*m_vpSpectrumEncoder[i]->getInputMinMaxFrequencyBands(),*l_pFrequencyBands);
				OpenViBEToolkit::Tools::MatrixManipulation::copy(*m_vpSpectrumEncoder[i]->getInputMatrix(),*l_pStreamedMatrix);
			}

			if (m_bAmplitudeSpectrum)
			{
				m_vpSpectrumEncoder[0]->encodeHeader();
				l_pDynamicContext->markOutputAsReadyToSend(0,m_ui64LastChunkStartTime, m_ui64LastChunkEndTime);
			}
			if (m_bPhaseSpectrum)
			{
				m_vpSpectrumEncoder[1]->encodeHeader();
				l_pDynamicContext->markOutputAsReadyToSend(1,m_ui64LastChunkStartTime, m_ui64LastChunkEndTime);
			}
			if (m_bRealPartSpectrum)
			{
				m_vpSpectrumEncoder[2]->encodeHeader();
				l_pDynamicContext->markOutputAsReadyToSend(2,m_ui64LastChunkStartTime, m_ui64LastChunkEndTime);
			}
			if (m_bImagPartSpectrum)
			{
				m_vpSpectrumEncoder[3]->encodeHeader();
				l_pDynamicContext->markOutputAsReadyToSend(3,m_ui64LastChunkStartTime, m_ui64LastChunkEndTime);
			}

			delete l_pFrequencyBands;
			delete l_pStreamedMatrix;
		}
		if(m_pSignalDecoder->isBufferReceived())
		{
			//do the processing
			vec x(m_ui32SampleCount);
			cvec y(m_ui32SampleCount);
			cvec z(m_ui32ChannelCount*m_ui32HalfFFTSize);

			for (uint64 i=0;  i < m_ui32ChannelCount; i++)
			{
				for(uint64 j=0 ; j<m_ui32SampleCount ; j++)
				{
					x[(int)j] =  (double)*(l_pBuffer+i*m_ui32SampleCount+j);
				}

				y = fft_real(x, m_ui32FFTSize);

				for(uint64 k=0 ; k<m_ui32HalfFFTSize ; k++)
				{
					z[(int)(k+i*m_ui32HalfFFTSize)] = y[(int)k];
				}
			}

			if (m_bAmplitudeSpectrum)
			{
				IMatrix* l_pMatrix = m_vpSpectrumEncoder[0]->getInputMatrix();
				float64* l_pBuffer = l_pMatrix->getBuffer();
				for (uint64 i=0;  i < m_ui32ChannelCount*m_ui32HalfFFTSize; i++)
				{
					*(l_pBuffer+i) = sqrt(real(z[(int)i])*real(z[(int)i])+ imag(z[(int)i])*imag(z[(int)i]));
				}
				m_vpSpectrumEncoder[0]->encodeBuffer();
				l_pDynamicContext->markOutputAsReadyToSend(0,m_ui64LastChunkStartTime, m_ui64LastChunkEndTime);
			}
			if (m_bPhaseSpectrum)
			{
				IMatrix* l_pMatrix = m_vpSpectrumEncoder[1]->getInputMatrix();
				float64* l_pBuffer = l_pMatrix->getBuffer();
				for (uint64 i=0;  i < m_ui32ChannelCount*m_ui32HalfFFTSize; i++)
				{
					*(l_pBuffer+i) =  imag(z[(int)i])/real(z[(int)i]);
				}
				m_vpSpectrumEncoder[1]->encodeBuffer();
				l_pDynamicContext->markOutputAsReadyToSend(1,m_ui64LastChunkStartTime, m_ui64LastChunkEndTime);
			}
			if (m_bRealPartSpectrum)
			{
				IMatrix* l_pMatrix = m_vpSpectrumEncoder[2]->getInputMatrix();
				float64* l_pBuffer = l_pMatrix->getBuffer();
				for (uint64 i=0;  i < m_ui32ChannelCount*m_ui32HalfFFTSize; i++)
				{
					*(l_pBuffer+i) = real(z[(int)i]);
				}
				m_vpSpectrumEncoder[2]->encodeBuffer();
				l_pDynamicContext->markOutputAsReadyToSend(2,m_ui64LastChunkStartTime, m_ui64LastChunkEndTime);
			}
			if (m_bImagPartSpectrum)
			{
				IMatrix* l_pMatrix = m_vpSpectrumEncoder[3]->getInputMatrix();
				float64* l_pBuffer = l_pMatrix->getBuffer();
				for (uint64 i=0;  i < m_ui32ChannelCount*m_ui32HalfFFTSize; i++)
				{
					*(l_pBuffer+i) = imag(z[(int)i]);
				}
				m_vpSpectrumEncoder[3]->encodeBuffer();
				l_pDynamicContext->markOutputAsReadyToSend(3,m_ui64LastChunkStartTime, m_ui64LastChunkEndTime);
			}
		}
		l_pDynamicContext->markInputAsDeprecated(0,chunkIdx);
	}
	return true;
}

#endif
