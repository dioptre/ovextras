#if defined(TARGET_HAS_ThirdPartyEIGEN)

#include "ovpCAlgorithmSingleTrialPhaseLockingValue.h"
#include <cmath>
#include <complex>
#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>
#include <system/Memory.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

using namespace Eigen;



void CAlgorithmSingleTrialPhaseLockingValue::HilbertPhase(RowVectorXd InputVector, RowVectorXd OutputVector)
{
	/*------------------------------------------------------------------
	 *
	 * Compute instantaneous phase using analytical signal (Hilbert
	 * transform) on InputVector, the result is stored in OutputVector
	 *
	 -------------------------------------------------------------------*/

		uint32 l_ui32SamplesPerChannel = InputVector.cols();

		RowVectorXcd l_vecXcdSignalBuffer; // Input signal Buffer
		RowVectorXcd l_vecXcdSignalFourier; // Fourier Transform of input signal
		RowVectorXd l_vecXdHilbert; // Vector h used to apply Hilbert transform

		FFT< double, internal::kissfft_impl<double > > fft;

				//Initialization of buffer vectors
				l_vecXcdSignalBuffer = RowVectorXcd::Zero(l_ui32SamplesPerChannel);
				l_vecXcdSignalFourier = RowVectorXcd::Zero(l_ui32SamplesPerChannel);

				//Initialization of vector h used to compute analytic signal
				l_vecXdHilbert.resize(l_ui32SamplesPerChannel);
				l_vecXdHilbert(0) = 1.0;
				if(l_ui32SamplesPerChannel%2 == 0)
				{
					l_vecXdHilbert(l_ui32SamplesPerChannel/2) = 1.0;
					for(uint32 i=1; i<l_ui32SamplesPerChannel/2; i++)
					{
						l_vecXdHilbert(i) = 2.0;
					}
					for(uint32 i=(l_ui32SamplesPerChannel/2)+1; i<l_ui32SamplesPerChannel; i++)
					{
						l_vecXdHilbert(i) = 0.0;
					}
				}
				else
				{
					l_vecXdHilbert((l_ui32SamplesPerChannel/2)+1) = 1.0;
					for(uint32 i=1; i<(l_ui32SamplesPerChannel/2)+1; i++)
					{
						l_vecXdHilbert(i) = 2.0;
					}
						for(uint32 i=(l_ui32SamplesPerChannel/2)+2; i<l_ui32SamplesPerChannel; i++)
					{
						l_vecXdHilbert(i) = 0.0;
					}
				}

				//Copy input signal chunk on buffer
				for(uint32 samples=0; samples<l_ui32SamplesPerChannel;samples++)
				{
					l_vecXcdSignalBuffer(samples).real(InputVector(samples));
					l_vecXcdSignalBuffer(samples).imag(0.0);
				}

				//Fast Fourier Transform of input signal
				fft.fwd(l_vecXcdSignalFourier, l_vecXcdSignalBuffer);

				//Apply Hilbert transform by element-wise multiplying fft vector by h
				l_vecXcdSignalFourier = l_vecXcdSignalFourier * l_vecXdHilbert;

				//Inverse Fast Fourier transform
				fft.inv(l_vecXcdSignalBuffer, l_vecXcdSignalFourier); //l_vecXcdSignalBuffer is now the analytical signal of the initial input signal

				//Compute phase and pass it to the corresponding output
				for(uint32 samples=0; samples<l_ui32SamplesPerChannel;samples++)
				{
					OutputVector(samples) = arg(l_vecXcdSignalBuffer(samples));
				}
}



boolean CAlgorithmSingleTrialPhaseLockingValue::initialize(void)
{

	m_pInputMatrix1 = ip_pSignal1;
    m_pInputMatrix2 = ip_pSignal2;

    m_ui64SamplingRate1 = ip_ui64SamplingRate1;
    m_ui64SamplingRate2 = ip_ui64SamplingRate2;

    m_pChannelPairs = ip_pChannelPairs;
    m_pOutputMatrix = op_pMatrix;

    m_ui32ChannelCount1 = m_pInputMatrix1->getDimensionSize(0);
    m_ui32SamplesPerChannel1 = m_pInputMatrix1->getDimensionSize(1);

    m_ui32ChannelCount2 = m_pInputMatrix2->getDimensionSize(0);
    m_ui32SamplesPerChannel2 = m_pInputMatrix2->getDimensionSize(1);

    m_ui32PairsCount = m_pChannelPairs->getDimensionSize(1)/2;

	//_______________________________________________________________________________________
	//
	// Form pairs with the lookup matrix given
	//_______________________________________________________________________________________
	//

    for(uint32 i=0; i<m_pChannelPairs->getDimensionSize(1); i=i+2)
    {
    	if(m_pChannelPairs->getBuffer()[i] < m_pInputMatrix1->getDimensionSize(0))
    	{
    		System::Memory::copy(m_pChannelToCompare->getBuffer()+i*m_ui32SamplesPerChannel1, m_pInputMatrix1->getBuffer()+(uint32)m_pChannelPairs->getBuffer()[i]*m_ui32SamplesPerChannel1,
    							m_ui32SamplesPerChannel1*sizeof(float64));
    	}
    }

    for(uint32 i=1; i<m_pChannelPairs->getDimensionSize(1); i=i+2)
        {
        	if(m_pChannelPairs->getBuffer()[i] < m_pInputMatrix2->getDimensionSize(0))
        	{
        		System::Memory::copy(m_pChannelToCompare->getBuffer()+i*m_ui32SamplesPerChannel2, m_pInputMatrix2->getBuffer()+(uint32)m_pChannelPairs->getBuffer()[i]*m_ui32SamplesPerChannel2,
        							m_ui32SamplesPerChannel2*sizeof(float64));
        	}
        }
	return true;
}

boolean CAlgorithmSingleTrialPhaseLockingValue::uninitialize(void) {
	// NOP
	return true;
}


boolean CAlgorithmSingleTrialPhaseLockingValue::process(void)
{

	    std::complex <double> iComplex(0.0,1.0);

	    if(this->isInputTriggerActive(OVTK_Algorithm_Connectivity_InputTriggerId_Initialize))
	    {

			if(m_ui32SamplesPerChannel1 != m_ui32SamplesPerChannel2)
			{
				this->getLogManager() << LogLevel_Error << "Can't compute S-PLV on two signals with different lengths";
				return false;
			}

	    	// Setting size of output
	    	m_pOutputMatrix->setDimensionCount(2); // the output matrix will have 2 dimensions
	    	m_pOutputMatrix->setDimensionSize(0,m_ui32PairsCount); //
	    	m_pOutputMatrix->setDimensionSize(1,1);//

	    }

	    if(this->isInputTriggerActive(OVTK_Algorithm_Connectivity_InputTriggerId_Process))
	   	{
	    	RowVectorXd l_vecXdChannelToCompare1;
	    	RowVectorXd l_vecXdChannelToCompare2;
			RowVectorXd l_vecXdPhase1;
			RowVectorXd l_vecXdPhase2;

			std::complex <double> sum(0.0,0.0);

	    	//Compute S-PLV for each pairs
	    	for(uint32 channel = 0; channel < m_pChannelPairs->getDimensionSize(1); channel = channel+2)
	    	{
		    	l_vecXdChannelToCompare1 = RowVectorXd::Zero(m_ui32SamplesPerChannel1);
		    	l_vecXdChannelToCompare2 = RowVectorXd::Zero(m_ui32SamplesPerChannel2);
				l_vecXdPhase1 = RowVectorXd::Zero(m_ui32SamplesPerChannel1);
				l_vecXdPhase2 = RowVectorXd::Zero(m_ui32SamplesPerChannel2);

	    		for(uint32 i=0; i<m_ui32SamplesPerChannel1; i++)
	    		{
			    	l_vecXdChannelToCompare1(i) = m_pChannelToCompare->getBuffer()[i + channel*m_ui32SamplesPerChannel1];
			    	l_vecXdChannelToCompare2(i) = m_pChannelToCompare->getBuffer()[i + (channel+1)*m_ui32SamplesPerChannel2];
	    		}

	    		// Compute instantaneous phase using Hilbert transform (analytical signal)
	    		HilbertPhase(l_vecXdChannelToCompare1, l_vecXdPhase1);
	    		HilbertPhase(l_vecXdChannelToCompare2, l_vecXdPhase2);

	    		for(uint32 i=0; i<m_ui32SamplesPerChannel1; i++)
	    		{
		    		sum += exp(iComplex*(l_vecXdPhase1(i)-l_vecXdPhase2(i)));
	    		}

	    		m_pOutputMatrix->getBuffer()[channel] = abs(sum)/m_ui32SamplesPerChannel1;

	    	}

	    	this->activateOutputTrigger(OVTK_Algorithm_Connectivity_OutputTriggerId_ProcessDone, true);
	   	}
	return true;
}


#endif //TARGET_HAS_ThirdPartyEIGEN
