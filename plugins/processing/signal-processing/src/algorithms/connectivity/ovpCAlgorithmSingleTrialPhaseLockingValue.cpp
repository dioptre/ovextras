#if defined(TARGET_HAS_ThirdPartyEIGEN)

#include "ovpCAlgorithmSingleTrialPhaseLockingValue.h"
#include <cmath>
#include <complex>
#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>
#include <system/Memory.h>
#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

using namespace Eigen;
using namespace std;


boolean CAlgorithmSingleTrialPhaseLockingValue::initialize(void)
{

	ip_pSignal1.initialize(this->getInputParameter(OVTK_Algorithm_Connectivity_InputParameterId_InputMatrix1));
	ip_pSignal2.initialize(this->getInputParameter(OVTK_Algorithm_Connectivity_InputParameterId_InputMatrix2));

	ip_pChannelPairs.initialize(this->getInputParameter(OVTK_Algorithm_Connectivity_InputParameterId_LookupMatrix));
	op_pMatrix.initialize(this->getOutputParameter(OVTK_Algorithm_Connectivity_OutputParameterId_OutputMatrix));

	// Create algorithm instance of Hilbert transform
	m_pHilbertTransform = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_HilbertTransform));
	m_pHilbertTransform->initialize();

	ip_pHilbertInput.initialize(m_pHilbertTransform->getInputParameter(OVP_Algorithm_HilbertTransform_InputParameterId_Matrix));
	op_pInstantaneousPhase.initialize(m_pHilbertTransform->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_PhaseMatrix));

	return true;
}

boolean CAlgorithmSingleTrialPhaseLockingValue::uninitialize(void) {

	ip_pSignal1.uninitialize();
	ip_pSignal2.uninitialize();

	ip_pHilbertInput.uninitialize();
	op_pInstantaneousPhase.uninitialize();

	ip_pChannelPairs.uninitialize();
	op_pMatrix.uninitialize();

	m_pHilbertTransform->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_pHilbertTransform);

	return true;
}


boolean CAlgorithmSingleTrialPhaseLockingValue::process(void)
{
		std::complex <double> iComplex(0.0,1.0);

		IMatrix* l_pInputMatrix1;
		IMatrix* l_pInputMatrix2;

		IMatrix* l_pChannelPairs;
		IMatrix* l_pOutputMatrix;

		MatrixXd l_pChannelToCompare;

		uint32 l_ui32ChannelCount1;
		uint32 l_ui32SamplesPerChannel1;

		uint32 l_ui32ChannelCount2;
		uint32 l_ui32SamplesPerChannel2;

		uint32 l_ui32PairsCount;

		l_pInputMatrix1 = ip_pSignal1;
		l_pInputMatrix2 = ip_pSignal2;

		l_pOutputMatrix = op_pMatrix;

		l_ui32ChannelCount1 = l_pInputMatrix1->getDimensionSize(0);
		l_ui32SamplesPerChannel1 = l_pInputMatrix1->getDimensionSize(1);

		l_ui32ChannelCount2 = l_pInputMatrix2->getDimensionSize(0);
		l_ui32SamplesPerChannel2 = l_pInputMatrix2->getDimensionSize(1);

		l_pChannelPairs = ip_pChannelPairs;
		l_ui32PairsCount = ip_pChannelPairs->getDimensionSize(0)/2;

		l_pChannelToCompare = MatrixXd::Zero(l_ui32ChannelCount1,l_ui32SamplesPerChannel1);


		if(this->isInputTriggerActive(OVTK_Algorithm_Connectivity_InputTriggerId_Initialize))
		{


			if(l_ui32SamplesPerChannel1 != l_ui32SamplesPerChannel2)
			{
				this->getLogManager() << LogLevel_Error << "Can't compute S-PLV on two signals with different lengths";
				return false;
			}

			if(l_ui32SamplesPerChannel1==0||l_ui32SamplesPerChannel2==0)
			{
				this->getLogManager() << LogLevel_Error << "Can't compute S-PLV, input signal size = 0";
				return false;
			}

			// Setting size of output
			l_pOutputMatrix->setDimensionCount(2); // the output matrix will have 2 dimensions
			l_pOutputMatrix->setDimensionSize(0,l_ui32PairsCount); //
			l_pOutputMatrix->setDimensionSize(1,1);//

			ip_pHilbertInput->setDimensionCount(2);
			ip_pHilbertInput->setDimensionSize(0,1);
			ip_pHilbertInput->setDimensionSize(1,l_ui32SamplesPerChannel1);

		}

		if(this->isInputTriggerActive(OVTK_Algorithm_Connectivity_InputTriggerId_Process))
		{
			VectorXd l_vecXdChannelToCompare1;
			VectorXd l_vecXdChannelToCompare2;
			VectorXd l_vecXdPhase1;
			VectorXd l_vecXdPhase2;


			std::complex <double> sum(0.0,0.0);
//			std::cout<<"Channel pairs = "<<l_pChannelPairs->getBuffer() <<std::endl;


			//_______________________________________________________________________________________
			//
			// Form pairs with the lookup matrix given
			//_______________________________________________________________________________________
			//

			for(uint32 i=0; i<l_ui32PairsCount*2; i++)
			{

				if(l_pChannelPairs->getBuffer()[i] < l_ui32ChannelCount1)
				{
					for(uint32 sample = 0; sample<l_ui32SamplesPerChannel1; sample++)
					{
						l_pChannelToCompare(i,sample) = l_pInputMatrix1->getBuffer()[sample+(uint32)l_pChannelPairs->getBuffer()[i]*l_ui32SamplesPerChannel1];
//						std::cout<<"Input matrix 1 = "<<l_pInputMatrix1->getBuffer()[sample+(uint32)l_pChannelPairs->getBuffer()[i]*l_ui32SamplesPerChannel1]<<std::endl;

					}

				}
			}

/*				for(uint32 i=1; i<l_pChannelPairs->getDimensionSize(0); i=i+2)
				{
					if(l_pChannelPairs->getBuffer()[i] < l_pInputMatrix2->getDimensionSize(0))
					{
						System::Memory::copy(l_pChannelToCompare->getBuffer()+i*l_ui32SamplesPerChannel2, l_pInputMatrix2->getBuffer()+(uint32)l_pChannelPairs->getBuffer()[i]*l_ui32SamplesPerChannel2,
								l_ui32SamplesPerChannel2*sizeof(float64));
					}
				}*/



			//Compute S-PLV for each pairs
			for(uint32 channel = 0; channel < l_ui32PairsCount*2; channel = channel+1)
			{
				l_vecXdChannelToCompare1 = VectorXd::Zero(l_ui32SamplesPerChannel1);
				l_vecXdChannelToCompare2 = VectorXd::Zero(l_ui32SamplesPerChannel2);
				l_vecXdPhase1 = VectorXd::Zero(l_ui32SamplesPerChannel1);
				l_vecXdPhase2 = VectorXd::Zero(l_ui32SamplesPerChannel2);
//				std::cout<<"channel to compare = "<<l_pChannelToCompare<<std::endl;

				for(uint32 i=0; i<l_ui32SamplesPerChannel1; i++)
				{
					l_vecXdChannelToCompare1(i) = l_pChannelToCompare(channel,i);
					l_vecXdChannelToCompare2(i) = l_pChannelToCompare(channel+1,i);

//					std::cout<<"channel to compare = "<<l_pChannelToCompare<<std::endl;
				}

//				std::cout<<"channel to compare 1 = "<<l_vecXdChannelToCompare1.transpose()<<std::endl;
//				std::cout<<"channel to compare 2 = "<<l_vecXdChannelToCompare2.transpose()<<std::endl;

				for(uint32 i=0; i<l_ui32SamplesPerChannel1; i++)
				{
					ip_pHilbertInput->getBuffer()[i] = l_vecXdChannelToCompare1(i);
				}

				m_pHilbertTransform->process(OVP_Algorithm_HilbertTransform_InputTriggerId_Initialize);
				m_pHilbertTransform->process(OVP_Algorithm_HilbertTransform_InputTriggerId_Process);
//				std::cout<<"N1 = "<<l_ui32SamplesPerChannel1<<"\n"<<std::endl;

				for(uint32 i=0; i<l_ui32SamplesPerChannel1; i++)
				{
					l_vecXdPhase1(i) = op_pInstantaneousPhase->getBuffer()[i];
					ip_pHilbertInput->getBuffer()[i] = (float64)l_vecXdChannelToCompare2(i);
				}

//				std::cout<<"Phase 1 = "<<l_vecXdPhase1.transpose()<<"\n"<<std::endl;

				m_pHilbertTransform->process(OVP_Algorithm_HilbertTransform_InputTriggerId_Initialize);
				m_pHilbertTransform->process(OVP_Algorithm_HilbertTransform_InputTriggerId_Process);

				for(uint32 i=0; i<l_ui32SamplesPerChannel1; i++)
				{
					l_vecXdPhase2(i) = op_pInstantaneousPhase->getBuffer()[i];

					sum += exp(iComplex*(l_vecXdPhase1(i)-l_vecXdPhase2(i)));
				}

//				std::cout<<"Phase 2 = "<<l_vecXdPhase2.transpose()<<"\n"<<std::endl;
//				std::cout<<"N2 = "<<l_ui32SamplesPerChannel1<<"\n"<<std::endl;



				l_pOutputMatrix->getBuffer()[channel] = abs(sum)/l_ui32SamplesPerChannel1;
//				std::cout<<"sum2 = "<<abs(sum)<<"\n"<<std::endl;

			}

			this->activateOutputTrigger(OVTK_Algorithm_Connectivity_OutputTriggerId_ProcessDone, true);
			}

	return true;
}


#endif //TARGET_HAS_ThirdPartyEIGEN
