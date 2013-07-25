#if defined(TARGET_HAS_ThirdPartyEIGEN)

#include "ovpCHilbertTransform.h"
#include <complex>
#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>
#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

using namespace Eigen;


boolean CAlgorithmHilbertTransform::initialize(void)
{
	ip_pMatrix.initialize(this->getInputParameter(OVP_Algorithm_HilbertTransform_InputParameterId_Matrix));
	op_pHilbertMatrix.initialize(this->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_HilbertMatrix));
	op_pEnvelopeMatrix.initialize(this->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_EnvelopeMatrix));
	op_pPhaseMatrix.initialize(this->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_PhaseMatrix));

	return true;
}

boolean CAlgorithmHilbertTransform::uninitialize(void)
{
	op_pHilbertMatrix.uninitialize();
	op_pEnvelopeMatrix.uninitialize();
	op_pPhaseMatrix.uninitialize();
	ip_pMatrix.uninitialize();
	return true;
}

boolean CAlgorithmHilbertTransform::process(void)
{

	uint32 l_ui32ChannelCount = ip_pMatrix->getDimensionSize(0);
	uint32 l_ui32SamplesPerChannel = ip_pMatrix->getDimensionSize(1);


	IMatrix* l_pInputMatrix = ip_pMatrix;
	IMatrix* l_pOutputHilbertMatrix = op_pHilbertMatrix;
	IMatrix* l_pOutputEnvelopeMatrix = op_pEnvelopeMatrix;
	IMatrix* l_pOutputPhaseMatrix = op_pPhaseMatrix;

	FFT< double, internal::kissfft_impl<double > > fft; //create instance of fft transform

	if(this->isInputTriggerActive(OVP_Algorithm_HilbertTransform_InputTriggerId_Initialize)) //Check if the input is correct
	{
		if( l_pInputMatrix->getDimensionCount() != 2)
		{
			this->getLogManager() << LogLevel_Error << "The input matrix must have 2 dimensions, here the dimension is ";
			std::cout<<l_pInputMatrix->getDimensionCount()<<std::endl;
			return false;
		}

		//Setting size of outputs

		l_pOutputHilbertMatrix->setDimensionCount(2);
		l_pOutputHilbertMatrix->setDimensionSize(0,l_ui32ChannelCount);
		l_pOutputHilbertMatrix->setDimensionSize(1,l_ui32SamplesPerChannel);

		l_pOutputEnvelopeMatrix->setDimensionCount(2);
		l_pOutputEnvelopeMatrix->setDimensionSize(0,l_ui32ChannelCount);
		l_pOutputEnvelopeMatrix->setDimensionSize(1,l_ui32SamplesPerChannel);

		l_pOutputPhaseMatrix->setDimensionCount(2);
		l_pOutputPhaseMatrix->setDimensionSize(0,l_ui32ChannelCount);
		l_pOutputPhaseMatrix->setDimensionSize(1,l_ui32SamplesPerChannel);

	}

	if(this->isInputTriggerActive(OVP_Algorithm_HilbertTransform_InputTriggerId_Process))
	{

		//Computing Hilbert transform for all channels
		for(uint32 channel=0; channel<l_ui32ChannelCount; channel++)
		{
			//Initialization of buffer vectors
			m_vecXcdSignalBuffer = VectorXcd::Zero(l_ui32SamplesPerChannel);
			m_vecXcdSignalFourier = VectorXcd::Zero(l_ui32SamplesPerChannel);

			//Initialization of vector h used to compute analytic signal
			m_vecXdHilbert.resize(l_ui32SamplesPerChannel);
			m_vecXdHilbert(0) = 1.0;

			if(l_ui32SamplesPerChannel%2 == 0)
			{
				m_vecXdHilbert(l_ui32SamplesPerChannel/2) = 1.0;
				for(uint32 i=1; i<l_ui32SamplesPerChannel/2; i++)
				{
					m_vecXdHilbert(i) = 2.0;
				}
				for(uint32 i=(l_ui32SamplesPerChannel/2)+1; i<l_ui32SamplesPerChannel; i++)
				{
					m_vecXdHilbert(i) = 0.0;
				}
			}
			else
			{
				m_vecXdHilbert((l_ui32SamplesPerChannel+1)/2) = 1.0;
				for(uint32 i=1; i<(l_ui32SamplesPerChannel+1); i++)
				{
					m_vecXdHilbert(i) = 2.0;
				}
				for(uint32 i=(l_ui32SamplesPerChannel+1)/2+1; i<l_ui32SamplesPerChannel; i++)
				{
					m_vecXdHilbert(i) = 0.0;
				}
			}

			//Copy input signal chunk on buffer
			for(uint32 samples=0; samples<l_ui32SamplesPerChannel;samples++)
			{
				m_vecXcdSignalBuffer(samples).real(l_pInputMatrix->getBuffer()[samples + channel * (l_ui32SamplesPerChannel)]);
				m_vecXcdSignalBuffer(samples).imag(0.0);
			}

			//Fast Fourier Transform of input signal
			fft.fwd(m_vecXcdSignalFourier, m_vecXcdSignalBuffer);

			//Apply Hilbert transform by element-wise multiplying fft vector by h
			for(uint32 samples=0; samples<l_ui32SamplesPerChannel;samples++)
			{
				m_vecXcdSignalFourier(samples) = m_vecXcdSignalFourier(samples)*m_vecXdHilbert(samples);
			}

			//Inverse Fast Fourier transform
			fft.inv(m_vecXcdSignalBuffer, m_vecXcdSignalFourier); // m_vecXcdSignalBuffer is now the analytical signal of the initial input signal

			//Compute envelope and phase and pass it to the corresponding output
			for(uint32 samples=0; samples<l_ui32SamplesPerChannel;samples++)
			{
				l_pOutputHilbertMatrix->getBuffer()[samples + channel*l_ui32SamplesPerChannel] = m_vecXcdSignalBuffer(samples).imag();
				l_pOutputEnvelopeMatrix->getBuffer()[samples + channel*l_ui32SamplesPerChannel] = abs(m_vecXcdSignalBuffer(samples));
				l_pOutputPhaseMatrix->getBuffer()[samples + channel*l_ui32SamplesPerChannel] = arg(m_vecXcdSignalBuffer(samples));
			}

		}

	}
	return true;
}

#endif //TARGET_HAS_ThirdPartyEIGEN
