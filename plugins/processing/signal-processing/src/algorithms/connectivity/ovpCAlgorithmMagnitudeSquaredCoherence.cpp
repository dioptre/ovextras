#if defined(TARGET_HAS_ThirdPartyEIGEN)

#include "ovpCAlgorithmMagnitudeSquaredCoherence.h"
#include <cmath>
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
using namespace std;



boolean CAlgorithmMagnitudeSquaredCoherence::computePeriodogram(const VectorXd& vecXdInput, Eigen::MatrixXcd& matXcdSignalFourier, const Eigen::VectorXd& vecXdWindow, const uint32& ui32NSegments, const uint32& ui32LSegments, const uint32& ui32NOverlap)
{
	MatrixXd l_matXdWindowedSegments = MatrixXd::Zero(ui32LSegments, ui32NSegments);

	m_f64U = 0; // Window normalization constant

	// Calculate window normalization constant. A 1/ui32LSegments factor has been removed since it will be canceled below
	for(uint32 i = 0; i<ui32LSegments; i++)
	{
		m_f64U += pow(vecXdWindow(i),2);
	}

	// Segment input vector and apply window to segment each segment
	for(uint32 k = 0; k<ui32NSegments; k++)
	{
		VectorXd l_vecXdWinSeg = VectorXd::Zero(ui32LSegments);
		VectorXcd l_vecXcdFourier = VectorXcd::Zero(ui32LSegments);


		for(uint32 i = 0; i<ui32LSegments; i++)
		{
			l_matXdWindowedSegments(i,k) = vecXdInput(i+k*ui32NOverlap)*vecXdWindow(i);
		}

		l_vecXdWinSeg = l_matXdWindowedSegments.col(k);

		// FFT of windowed segments
		m_oFFT.fwd(l_vecXcdFourier,l_vecXdWinSeg);

		matXcdSignalFourier.col(k) = l_vecXcdFourier;

	}
	return true;
}


boolean CAlgorithmMagnitudeSquaredCoherence::powerSpectralDensity(const VectorXd& vecXdInput, VectorXd& vecXdOutput, const VectorXd& vecXdWindow, const uint32& ui32NSegments, const uint32& ui32LSegments, const uint32& ui32NOverlap)
{
	MatrixXcd l_matXcdSignalFourier = MatrixXcd::Zero(ui32LSegments, ui32NSegments);
	MatrixXd l_matXdPeriodograms = MatrixXd::Zero(ui32LSegments, ui32NSegments);

	// Compute periodograms
	CAlgorithmMagnitudeSquaredCoherence::computePeriodogram(vecXdInput, l_matXcdSignalFourier, vecXdWindow, ui32NSegments, ui32LSegments, ui32NOverlap);

	for(uint32 k = 0; k<ui32NSegments; k++)
	{
		for(uint32 i = 0; i<ui32LSegments; i++)
		{
			l_matXdPeriodograms(i+k*ui32LSegments) = real(l_matXcdSignalFourier(i+k*ui32LSegments)*conj(l_matXcdSignalFourier(i+k*ui32LSegments)))/m_f64U;
			//vecXdOutput = vecXdOutput + l_matXdPeriodograms.col(k);
		}

		vecXdOutput = vecXdOutput + l_matXdPeriodograms.col(k);
	}

	vecXdOutput = vecXdOutput / ui32NSegments;

	return true;

}


boolean CAlgorithmMagnitudeSquaredCoherence::crossSpectralDensity(const VectorXd& vecXdInput1, const VectorXd& vecXdInput2, VectorXcd& vecXcdOutput, const VectorXd& vecXdWindow, const uint32& ui32NSegments, const uint32& ui32LSegments, const uint32& ui32NOverlap)
{
	MatrixXcd l_matXcdSignalFourier1 = MatrixXcd::Zero(ui32LSegments, ui32NSegments);
	MatrixXcd l_matXcdSignalFourier2 = MatrixXcd::Zero(ui32LSegments, ui32NSegments);
	MatrixXcd l_matXcdPeriodograms = MatrixXcd::Zero(ui32LSegments, ui32NSegments);

	//Compute periodograms for input 1 and 2
	CAlgorithmMagnitudeSquaredCoherence::computePeriodogram(vecXdInput1, l_matXcdSignalFourier1, vecXdWindow, ui32NSegments, ui32LSegments, ui32NOverlap);
	CAlgorithmMagnitudeSquaredCoherence::computePeriodogram(vecXdInput2, l_matXcdSignalFourier2, vecXdWindow, ui32NSegments, ui32LSegments, ui32NOverlap);


	for(uint32 k = 0; k<ui32NSegments; k++)
	{
		l_matXcdPeriodograms.col(k) = l_matXcdSignalFourier2.col(k).cwiseProduct(conj(l_matXcdSignalFourier1.col(k)))/m_f64U;
		vecXcdOutput = vecXcdOutput + l_matXcdPeriodograms.col(k);
	}

	vecXcdOutput = vecXcdOutput / ui32NSegments;

	return true;

}

boolean CAlgorithmMagnitudeSquaredCoherence::initialize(void)
{

	ip_pSignal1.initialize(this->getInputParameter(OVP_Algorithm_Connectivity_InputParameterId_InputMatrix1));
	ip_pSignal2.initialize(this->getInputParameter(OVP_Algorithm_Connectivity_InputParameterId_InputMatrix2));
	ip_ui64SamplingRate1.initialize(this->getInputParameter(OVP_Algorithm_Connectivity_InputParameterId_ui64SamplingRate1));

	ip_pChannelPairs.initialize(this->getInputParameter(OVP_Algorithm_Connectivity_InputParameterId_LookupMatrix));
	op_pMatrixMean.initialize(this->getOutputParameter(OVP_Algorithm_Connectivity_OutputParameterId_OutputMatrix));
	op_pMatrixSpectrum.initialize(this->getOutputParameter(OVP_Algorithm_MagnitudeSquaredCoherence_OutputParameterId_OutputMatrixSpectrum));
	op_pFrequencyBandVector.initialize(this->getOutputParameter(OVP_Algorithm_MagnitudeSquaredCoherence_OutputParameterId_FreqVector));

	ip_ui64WindowType.initialize(this->getInputParameter(OVP_Algorithm_MagnitudeSquaredCoherence_InputParameterId_Window));
	ip_ui64SegmentLength.initialize(this->getInputParameter(OVP_Algorithm_MagnitudeSquaredCoherence_InputParameterId_SegLength));
	ip_ui64Overlap.initialize(this->getInputParameter(OVP_Algorithm_MagnitudeSquaredCoherence_InputParameterId_Overlap));


	return true;
}

boolean CAlgorithmMagnitudeSquaredCoherence::uninitialize(void)
{

	ip_pSignal1.uninitialize();
	ip_pSignal2.uninitialize();
	ip_ui64SamplingRate1.uninitialize();

	ip_pChannelPairs.uninitialize();
	op_pMatrix.uninitialize();
	op_pMatrixSpectrum.uninitialize();
	op_pFrequencyBandVector.uninitialize();

	ip_ui64WindowType.uninitialize();
	ip_ui64SegmentLength.uninitialize();
	ip_ui64Overlap.uninitialize();

	return true;
}


boolean CAlgorithmMagnitudeSquaredCoherence::process(void)
{

		IMatrix* l_pInputMatrix1 = ip_pSignal1;
		IMatrix* l_pInputMatrix2 = ip_pSignal2;
		uint64 l_ui64SamplingRate = ip_ui64SamplingRate1;

		IMatrix* l_pChannelPairs = ip_pChannelPairs;
		IMatrix* l_pOutputMatrixMeanCoherence = op_pMatrixMean;
		IMatrix* l_pOutputMatrixCoherenceSpectrum = op_pMatrixSpectrum;

		IMatrix* l_pFrequencyVector = op_pFrequencyBandVector;

		uint32 l_ui32ChannelCount1 = l_pInputMatrix1->getDimensionSize(0);
		uint32 l_ui32SamplesPerChannel1 = l_pInputMatrix1->getDimensionSize(1);

		uint32 l_ui32ChannelCount2 = l_pInputMatrix2->getDimensionSize(0);
		uint32 l_ui32SamplesPerChannel2 = l_pInputMatrix2->getDimensionSize(1);

		uint32 l_ui32PairsCount = ip_pChannelPairs->getDimensionSize(0)/2;

		MatrixXd l_pChannelToCompare = MatrixXd::Zero(ip_pChannelPairs->getDimensionSize(0),l_ui32SamplesPerChannel1);


		float64* l_ipMatrixBuffer1 = l_pInputMatrix1->getBuffer();
		float64* l_ipMatrixBuffer2 = l_pInputMatrix2->getBuffer();
		float64* l_opMatrixMeanBuffer = l_pOutputMatrixMeanCoherence->getBuffer();
		float64* l_opMatrixSpectrumBuffer = l_pOutputMatrixCoherenceSpectrum->getBuffer();

		uint32 l_ui32SegmentsLength = (uint32) ip_ui64SegmentLength;
		uint64 l_ui64WindowType = ip_ui64WindowType;

		uint32 l_ui32OverlapPercent = (uint32) ip_ui64Overlap;

		// Setting window vector
		VectorXd m_vecXdWindow = VectorXd::Zero(l_ui32SegmentsLength);

		// Getting window vector
		if (l_ui64WindowType == OVP_TypeId_WindowType_Bartlett)
		{
			m_oWindow.bartlett(m_vecXdWindow, l_ui32SegmentsLength);
		}

		else if (l_ui64WindowType == OVP_TypeId_WindowType_Hamming)
		{
			m_oWindow.hamming(m_vecXdWindow, l_ui32SegmentsLength);
		}

		else if (l_ui64WindowType == OVP_TypeId_WindowType_Hann)
		{
			m_oWindow.hann(m_vecXdWindow, l_ui32SegmentsLength);
		}

		else if (l_ui64WindowType == OVP_TypeId_WindowType_Parzen)
		{
			m_oWindow.parzen(m_vecXdWindow, l_ui32SegmentsLength);
		}

		else if (l_ui64WindowType == OVP_TypeId_WindowType_Welch)
		{
			m_oWindow.welch(m_vecXdWindow, l_ui32SegmentsLength);
		}
		else // else rectangular window
		{
			m_vecXdWindow = VectorXd::Ones(l_ui32SegmentsLength);
		}
//		cout<<"window = "<<m_vecXdWindow.transpose()<<endl;


		if(this->isInputTriggerActive(OVP_Algorithm_Connectivity_InputTriggerId_Initialize))
		{

			// Do some verification
			if(l_ui32SamplesPerChannel1 != l_ui32SamplesPerChannel2)
			{
				this->getLogManager() << LogLevel_Error << "Can't compute MSCoherence on two signals with different lengths\n";
				return false;
			}

			if(l_ui32SamplesPerChannel1==0||l_ui32SamplesPerChannel2==0)
			{
				this->getLogManager() << LogLevel_Error << "Can't compute MSCoherence, input signal size = 0\n";
				return false;
			}

			if(l_ui32OverlapPercent < 0 || l_ui32OverlapPercent > 100)
			{
				this->getLogManager() << LogLevel_Error << "Overlap must be a value between 0 and 100\n";
				return false;
			}

			if(l_ui32SegmentsLength == 0)
			{
				this->getLogManager() << LogLevel_Error << "Segments must have a strictly positive length (>0)\n";
				return false;
			}

			// Setting size of outputs
			l_pOutputMatrixMeanCoherence->setDimensionCount(2); // the output matrix will have 2 dimensions
			l_pOutputMatrixMeanCoherence->setDimensionSize(0,l_ui32PairsCount);
			l_pOutputMatrixMeanCoherence->setDimensionSize(1,1); // Compute the mean so only one value

			l_pOutputMatrixCoherenceSpectrum->setDimensionCount(2); // the output matrix will have 2 dimensions
			l_pOutputMatrixCoherenceSpectrum->setDimensionSize(0,l_ui32PairsCount);
			l_pOutputMatrixCoherenceSpectrum->setDimensionSize(1, l_ui32SegmentsLength);

			l_pFrequencyVector->setDimensionCount(2);
			l_pFrequencyVector->setDimensionSize(0,l_ui32PairsCount);
			l_pFrequencyVector->setDimensionSize(1, 2*(l_ui32SegmentsLength-1));


			// Setting name of output channels for visualization
			CString l_name1, l_name2, l_name;
			uint32 l_ui32Index;
			for(uint32 i=0;i<l_ui32PairsCount;i++)
			{
				l_ui32Index=2*i;
				l_name1 = l_pInputMatrix1->getDimensionLabel(0,l_pChannelPairs->getBuffer()[l_ui32Index+1]);
				l_name2 = l_pInputMatrix2->getDimensionLabel(0,l_pChannelPairs->getBuffer()[l_ui32Index+2]);
				l_name = l_name1+" "+l_name2;
				l_pOutputMatrixMeanCoherence->setDimensionLabel(0,i,l_name);
				l_pOutputMatrixCoherenceSpectrum->setDimensionLabel(0,i,l_name);
			}


		}

		if(this->isInputTriggerActive(OVP_Algorithm_Connectivity_InputTriggerId_Process))
		{


			VectorXd l_vecXdChannelToCompare1;
			VectorXd l_vecXdChannelToCompare2;

			VectorXd l_vecXdCoherenceNum;
			VectorXd l_vecXdCoherenceDen;
			VectorXd l_vecXdCoherence;

			uint32 l_ui32NOverlap = l_ui32OverlapPercent*l_ui32SegmentsLength/100; // Convert percentage in samples

			uint32 l_ui32NbSegments = 0;
			float64 l_f64MeanCohere = 0;

			vector<float64> l_vecFreqVector;

			float64 l_f64FrequencyBandStart = 0;
			float64 l_f64FrequencyBandStop = 0;

			// Calculate number of segment on data set giving segment's length and overlap
			if(l_ui32NOverlap != 0)
			{
				l_ui32NbSegments = (l_ui32SamplesPerChannel1 - l_ui32SegmentsLength)/l_ui32NOverlap + 1;
			}
			else
			{
				l_ui32NbSegments = l_ui32SamplesPerChannel1/l_ui32SegmentsLength;
			}


			//_______________________________________________________________________________________
			//
			// Compute MSC for each pairs
			//_______________________________________________________________________________________
			//

			for(uint32 channel = 0; channel < l_ui32PairsCount; channel++)
			{
				l_vecXdChannelToCompare1 = VectorXd::Zero(l_ui32SamplesPerChannel1);
				l_vecXdChannelToCompare2 = VectorXd::Zero(l_ui32SamplesPerChannel2);

				l_vecXdCoherenceNum = VectorXd::Zero(l_ui32SegmentsLength);
				l_vecXdCoherenceDen = VectorXd::Zero(l_ui32SegmentsLength);
				l_vecXdCoherence = VectorXd::Zero(l_ui32SegmentsLength);

				m_vecXdPowerSpectrum1 = VectorXd::Zero(l_ui32SegmentsLength);
				m_vecXdPowerSpectrum2 = VectorXd::Zero(l_ui32SegmentsLength);
				m_vecXcdCrossSpectrum = VectorXcd::Zero(l_ui32SegmentsLength);

				uint32 l_channelIndex = 2*channel; //Index on single channel

				//_______________________________________________________________________________________
				//
				// Form pairs with the lookup matrix given
				//_______________________________________________________________________________________
				//

				for(uint32 sample = 0; sample < l_ui32SamplesPerChannel1; sample++)
				{
					if(l_pChannelPairs->getBuffer()[sample] < l_ui32ChannelCount1)
					{
						l_pChannelToCompare(l_channelIndex,sample) = l_ipMatrixBuffer1[sample+(uint32)l_pChannelPairs->getBuffer()[l_channelIndex]*l_ui32SamplesPerChannel1];
						l_pChannelToCompare(l_channelIndex+1,sample) = l_ipMatrixBuffer2[sample+(uint32)l_pChannelPairs->getBuffer()[l_channelIndex+1]*l_ui32SamplesPerChannel2];
					}
				}

				// Retrieve the 2 channel to compare
				l_vecXdChannelToCompare1 = l_pChannelToCompare.row(l_channelIndex);
				l_vecXdChannelToCompare2 = l_pChannelToCompare.row(l_channelIndex+1);

				/* Compute MSC */
				CAlgorithmMagnitudeSquaredCoherence::powerSpectralDensity(l_vecXdChannelToCompare1, m_vecXdPowerSpectrum1, m_vecXdWindow, l_ui32NbSegments, l_ui32SegmentsLength, l_ui32NOverlap);
				CAlgorithmMagnitudeSquaredCoherence::powerSpectralDensity(l_vecXdChannelToCompare2, m_vecXdPowerSpectrum2, m_vecXdWindow, l_ui32NbSegments, l_ui32SegmentsLength, l_ui32NOverlap);
				CAlgorithmMagnitudeSquaredCoherence::crossSpectralDensity(l_vecXdChannelToCompare1, l_vecXdChannelToCompare2, m_vecXcdCrossSpectrum, m_vecXdWindow, l_ui32NbSegments, l_ui32SegmentsLength, l_ui32NOverlap);

//				cout<<"Gxx = "<<m_vecXdPowerSpectrum1.transpose()<<endl;
//				cout<<"Gyy = "<<m_vecXdPowerSpectrum2.transpose()<<endl;
//				cout<<"Gxy = "<<m_vecXcdCrossSpectrum.transpose()<<endl;

				for (uint32 i = 0; i<m_vecXcdCrossSpectrum.size(); i++)
				{
					l_vecXdCoherenceNum(i) = real(m_vecXcdCrossSpectrum(i)*(conj(m_vecXcdCrossSpectrum(i))));
				}
//				l_vecXdCoherenceNum = real(m_vecXcdCrossSpectrum.cwiseProduct(conj(m_vecXcdCrossSpectrum)));
				l_vecXdCoherenceDen = m_vecXdPowerSpectrum1.cwiseProduct(m_vecXdPowerSpectrum2);
				l_vecXdCoherence = l_vecXdCoherenceNum.cwiseQuotient(l_vecXdCoherenceDen);

//				cout<<"Cxy = "<<l_vecXdCoherence.transpose()<<endl;
//				cout<<"Cohere size ="<<l_vecXdCoherence.size()<<endl;


				for(uint32 i = 0; i<l_vecXdCoherence.size(); i++)
				{
					// Write coherence to output
					l_pOutputMatrixCoherenceSpectrum->getBuffer()[i+channel*l_vecXdCoherence.size()] = l_vecXdCoherence(i);

//					cout<<"Cxy = "<<l_opMatrixSpectrumBuffer[i+channel*l_vecXdCoherence.size()]<<endl;

					// Compute MSC mean over frequencies
					l_f64MeanCohere += l_vecXdCoherence(i);

				}

				// Write coherence mean over frequencies to output
				l_pOutputMatrixMeanCoherence->getBuffer()[channel] = l_f64MeanCohere/l_vecXdCoherence.size();

//				cout<<"Mean = "<<l_pOutputMatrixMeanCoherence->getBuffer()[channel]<<endl;

				// Create frequency vector for the spectrum encoder
				for(uint32 i = 0; i<l_ui32SegmentsLength; i++)
				{
					l_f64FrequencyBandStart = i * l_ui64SamplingRate/l_ui32SegmentsLength;
					l_f64FrequencyBandStop = (i+1) * l_ui64SamplingRate/l_ui32SegmentsLength;
					l_vecFreqVector.push_back(l_f64FrequencyBandStart);
					l_vecFreqVector.push_back(l_f64FrequencyBandStop);
				}

//				cout<<"size = "<<l_vecFreqVector.size()<<endl;

				for(uint32 k = 0; k < 2*(l_ui32SegmentsLength-1);k++)
				{
					op_pFrequencyBandVector->getBuffer()[k+channel*2*(l_ui32SegmentsLength-1)] = l_vecFreqVector[k];
//					cout<<"Freq = "<<op_pFrequencyBandVector->getBuffer()[k+channel*2*(l_ui32SegmentsLength-1)]<<endl;
				}
			}
			this->activateOutputTrigger(OVP_Algorithm_Connectivity_OutputTriggerId_ProcessDone, true);
		}

	return true;
}
#endif //TARGET_HAS_ThirdPartyEIGEN
