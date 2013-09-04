#include "ovpCBoxAlgorithmEvidenceAccumulator.h"
#include <iostream>
#include <limits>
#include <cmath>
//#include <sys/time.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

//struct timeval currentLTime;

#define time2ms(x,y) ((x) * 1000 + y/1000.0) + 0.5

boolean CBoxAlgorithmEvidenceAccumulator::initialize(void)
{
	m_oAlgo0_StimulationDecoder.initialize(*this);
	m_oAlgo1_FeatureVectorDecoder.initialize(*this);
	m_oAlgo2_StimulationEncoder.initialize(*this);
	m_oAlgo3_FeatureVectorEncoder.initialize(*this);
	
	m_pAccumulatedEvidence = new CMatrix();
	m_pNormalizedAccumulatedEvidence = new CMatrix();
	m_oAlgo3_FeatureVectorEncoder.getInputMatrix().setReferenceTarget(m_pNormalizedAccumulatedEvidence);
	m_oAlgo2_StimulationEncoder.getInputStimulationSet().setReferenceTarget(m_oAlgo0_StimulationDecoder.getOutputStimulationSet());
	
	m_pResetTrigger = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	CString l_sEvidenceAlgorithm = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_pStimulationBase = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	m_oEvidenceAlgorithm = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_EvidenceAccumulationAlgorithm, l_sEvidenceAlgorithm);
	m_bEarlyStoppingEnabled = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
	if (m_bEarlyStoppingEnabled)
		getLogManager() << LogLevel_Info << "Early stopping is enabled\n";
	else
		getLogManager() << LogLevel_Info << "Early stopping is disabled\n";
	m_bStopCondition = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
	m_oInputType = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_EvidenceAccumulationAlgorithmInputType, 
												     FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5));
	m_bScaleFactor = 0.5;
	
	m_bReceivedSomeEvidence = false;
	m_bEarlyStoppingConditionMet = false;
	m_ui32MaximumIndex = 0;
	
	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmEvidenceAccumulator::uninitialize(void)
{
	m_oAlgo0_StimulationDecoder.uninitialize();
	m_oAlgo1_FeatureVectorDecoder.uninitialize();
	m_oAlgo2_StimulationEncoder.uninitialize();
	m_oAlgo3_FeatureVectorEncoder.uninitialize();
	
	delete m_pAccumulatedEvidence;
	delete m_pNormalizedAccumulatedEvidence;

	return true;
}

boolean CBoxAlgorithmEvidenceAccumulator::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmEvidenceAccumulator::process(void)
{
	
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//iterate over all chunk on input 0
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		// decode the chunk i on input 0
		m_oAlgo0_StimulationDecoder.decode(0,i);
		// the decoder may have decoded 3 different parts : the header, a buffer or the end of stream.
		IStimulationSet* l_pStimulationSet = m_oAlgo0_StimulationDecoder.getOutputStimulationSet();
		if(m_oAlgo0_StimulationDecoder.isHeaderReceived())
		{
			m_oAlgo2_StimulationEncoder.encodeHeader(0);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
		if(m_oAlgo0_StimulationDecoder.isBufferReceived())
		{				
			if(l_pStimulationSet->getStimulationCount()>0 && l_pStimulationSet->getStimulationIdentifier(0)==m_pResetTrigger)
			{
				getLogManager() << LogLevel_Info << "Reset trigger received, flushing most up to date evidence\n";
				/*float32 l_f32Maximum = std::numeric_limits<int>::min();
				uint32 l_ui32MaximumIndex = 0;
				for (uint32 j=0; j<m_pAccumulatedEvidence->getBufferElementCount(); j++)
					if (*(m_pAccumulatedEvidence->getBuffer()+j)>l_f32Maximum)
					{
						l_f32Maximum = *(m_pAccumulatedEvidence->getBuffer()+j);
						l_ui32MaximumIndex = j;
					}*/
				float32 l_f32Maximum;
				findMaximum(m_pNormalizedAccumulatedEvidence->getBuffer(), m_ui32MaximumIndex, l_f32Maximum);
				
				l_pStimulationSet->clear();
				
				if (m_bReceivedSomeEvidence)
				{
					//gettimeofday(&currentLTime, NULL);	
					//getLogManager() << LogLevel_Trace << "Encoding evidence and final stimulus  " << m_pStimulationBase+l_ui32MaximumIndex << " at time " << (uint32)currentLTime.tv_sec << "," << (uint32)currentLTime.tv_usec <<"\n";	
					l_pStimulationSet->appendStimulation(m_pStimulationBase+m_ui32MaximumIndex, l_rDynamicBoxContext.getInputChunkStartTime(0, i), 0);					
					l_pStimulationSet->appendStimulation(m_pResetTrigger, l_rDynamicBoxContext.getInputChunkStartTime(0, i), 0);					
					
					//OpenViBEToolkit::Tools::Matrix::copy(*m_oAlgo3_FeatureVectorEncoder.getInputMatrix(), *m_pAccumulatedEvidence);
					m_oAlgo3_FeatureVectorEncoder.encodeBuffer(1);
					l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));				
				
					m_bReceivedSomeEvidence = false;
				}
				
				std::cout << "Maximum " << l_f32Maximum << " at " << m_ui32MaximumIndex << "\n";					
				
				for (uint32 j=0; j<m_pAccumulatedEvidence->getBufferElementCount(); j++)
				{
					std::cout << " " << *(m_pNormalizedAccumulatedEvidence->getBuffer()+j);
					*(m_pAccumulatedEvidence->getBuffer()+j) = 0;	
					*(m_pNormalizedAccumulatedEvidence->getBuffer()+j) = 0;
				}
				std::cout << "\n";
				m_bEarlyStoppingConditionMet = false;
			}
			else
			{
				l_pStimulationSet->clear();
				if(m_bEarlyStoppingConditionMet && m_bEarlyStoppingEnabled)
				{
					getLogManager() << LogLevel_Info << "Early stopping enabled and condition met\n";
					/*for (uint32 j=0; j<m_pNormalizedAccumulatedEvidence->getBufferElementCount(); j++)
					{
						std::cout << " " << *(m_pNormalizedAccumulatedEvidence->getBuffer()+j);	
					}
					std::cout << "\n";*/
					
					l_pStimulationSet->appendStimulation(m_pStimulationBase+m_ui32MaximumIndex, l_rDynamicBoxContext.getInputChunkStartTime(0, i), 0);
					
					//OpenViBEToolkit::Tools::Matrix::copy(*m_oAlgo3_FeatureVectorEncoder.getInputMatrix(), *m_pAccumulatedEvidence);
					m_oAlgo3_FeatureVectorEncoder.encodeBuffer(1);
					l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));									
					
					m_bEarlyStoppingConditionMet = false;
				}						
			}
			
			//if(m_bReceivedSomeEvidence)
			//{
			m_oAlgo2_StimulationEncoder.encodeBuffer(0);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			//}
		}
		if(m_oAlgo0_StimulationDecoder.isEndReceived())
		{
			m_oAlgo2_StimulationEncoder.encodeEnd(0);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
	}
	//iterate over all chunk on input 1
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(1); i++)
	{
		// decode the chunk i on input 0
		m_oAlgo1_FeatureVectorDecoder.decode(1,i);
		IMatrix* l_pInputMatrix = m_oAlgo1_FeatureVectorDecoder.getOutputMatrix();
		if(m_oAlgo1_FeatureVectorDecoder.isHeaderReceived())
		{
			m_pAccumulatedEvidence->setDimensionCount(l_pInputMatrix->getDimensionCount());
			m_pAccumulatedEvidence->setDimensionSize(0,l_pInputMatrix->getDimensionSize(0));
			m_pAccumulatedEvidence->setDimensionSize(1,l_pInputMatrix->getDimensionSize(1));
			for (uint32 j=0; j<l_pInputMatrix->getBufferElementCount(); j++)
				*(m_pAccumulatedEvidence->getBuffer()+j) = 0;	
			OpenViBEToolkit::Tools::Matrix::copy(*m_pNormalizedAccumulatedEvidence, *m_pAccumulatedEvidence);
			
			//OpenViBEToolkit::Tools::Matrix::copyDescription(*m_oAlgo3_FeatureVectorEncoder.getInputStreamedMatrix(), *m_pAccumulatedEvidence);
			m_oAlgo3_FeatureVectorEncoder.encodeHeader(1);
			l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(1, i), l_rDynamicBoxContext.getInputChunkEndTime(1, i));
		}
		if(m_oAlgo1_FeatureVectorDecoder.isBufferReceived())
		{			
			/*if(m_bReset)
			{
				std::cout << "encoding final evidence\n";
				m_oAlgo3_FeatureVectorEncoder.encodeBuffer(1);
				l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(1, i), l_rDynamicBoxContext.getInputChunkEndTime(1, i));				
				for (uint32 j=0; j<l_pInputMatrix->getBufferElementCount(); j++)
					*(m_pAccumulatedEvidence->getBuffer()+j) = 0;
				m_bReset = false;
			}*/
			if (m_oEvidenceAlgorithm==OVP_ClassId_Algorithm_EvidenceAccumulationCounter)
			{
				//getLogManager() << LogLevel_Info << "Evidence algorithm 'Counter' update\n";
				float64 l_f64Sum = 0;
				for (uint32 j=0; j<l_pInputMatrix->getBufferElementCount(); j++)
				{
					*(m_pAccumulatedEvidence->getBuffer()+j) += *(l_pInputMatrix->getBuffer()+j);
					*(m_pNormalizedAccumulatedEvidence->getBuffer()+j) = *(m_pAccumulatedEvidence->getBuffer()+j);
					l_f64Sum += *(m_pAccumulatedEvidence->getBuffer()+j);
				}
				for (uint32 j=0; j<l_pInputMatrix->getBufferElementCount(); j++)
				{
					//std::cout << " " << *(m_pNormalizedAccumulatedEvidence->getBuffer()+j);
					*(m_pNormalizedAccumulatedEvidence->getBuffer()+j) /= l_f64Sum;
				}
				//std::cout << "\n";
				float32 l_f32Maximum;
				findMaximum(m_pAccumulatedEvidence->getBuffer(), m_ui32MaximumIndex, l_f32Maximum);
				m_bEarlyStoppingConditionMet = true;
				for (uint32 j=0; j<l_pInputMatrix->getBufferElementCount(); j++)
					if (m_ui32MaximumIndex!=j && *(m_pAccumulatedEvidence->getBuffer()+j)>l_f32Maximum-m_bStopCondition)
						m_bEarlyStoppingConditionMet = false;
				m_bReceivedSomeEvidence = true;	
			}	
			else if (m_oEvidenceAlgorithm==OVP_ClassId_Algorithm_EvidenceAccumulationBayesian)
			{
				float64 l_f64RealPredictionValue = 0.0;
				uint32 l_ui32NumberOfNonZero = 0;
				//std::cout << "Incoming values ";
				for (uint32 j=0; j<l_pInputMatrix->getBufferElementCount(); j++)
				{
					//std::cout << " " << *(l_pInputMatrix->getBuffer()+j);
					if(*(l_pInputMatrix->getBuffer()+j)!=0)
					{
						l_f64RealPredictionValue = *(l_pInputMatrix->getBuffer()+j);
						l_ui32NumberOfNonZero++;
					}
				}
				//std::cout << "\n";
				//std::cout << "RealPredictionValue " << l_f64RealPredictionValue << "\n";
				uint32 l_ui32NumberOfZero = l_pInputMatrix->getBufferElementCount()-l_ui32NumberOfNonZero;	
				
				//std::cout << "Accumulated evidence ";
				for (uint32 j=0; j<l_pInputMatrix->getBufferElementCount(); j++)
				{
					float64 l_f64ProbabilityEstimate = 0.0;
					float64 l_f64InputValue = *(l_pInputMatrix->getBuffer()+j);
					if (m_oInputType==OVP_InputType_EvidenceAccumulationDistance)
					{
						l_f64ProbabilityEstimate = std::exp(m_bScaleFactor*l_f64InputValue) / 
												(1+std::exp(m_bScaleFactor*l_f64RealPredictionValue));
					}
					else if (m_oInputType==OVP_InputType_EvidenceAccumulationProbability)
					{
						l_f64ProbabilityEstimate = l_f64InputValue==0?1.0-l_f64InputValue:l_f64InputValue;
					}
					if (l_f64InputValue!=0)
						*(m_pAccumulatedEvidence->getBuffer()+j) += std::log(l_f64ProbabilityEstimate/(float64)l_ui32NumberOfNonZero);
					else
						*(m_pAccumulatedEvidence->getBuffer()+j) += std::log(l_f64ProbabilityEstimate/(float64)l_ui32NumberOfZero);
					//*(m_pNormalizedAccumulatedEvidence->getBuffer()+j) = *(m_pAccumulatedEvidence->getBuffer()+j);
					//l_f64Sum += *(m_pAccumulatedEvidence->getBuffer()+j);
					//std::cout << " " << *(m_pAccumulatedEvidence->getBuffer()+j);
				}	
				//std::cout << "\n";
				
				float32 l_f32Maximum;
				findMaximum(m_pAccumulatedEvidence->getBuffer(), m_ui32MaximumIndex, l_f32Maximum);
				
				float64 l_f64Sum = 0.0;
				for (uint32 j=0; j<l_pInputMatrix->getBufferElementCount(); j++)
				{
					*(m_pAccumulatedEvidence->getBuffer()+j) -= l_f32Maximum;
					l_f64Sum += std::exp(*(m_pAccumulatedEvidence->getBuffer()+j));
				}
				
				for (uint32 j=0; j<l_pInputMatrix->getBufferElementCount(); j++)
					*(m_pNormalizedAccumulatedEvidence->getBuffer()+j) = std::exp(*(m_pAccumulatedEvidence->getBuffer()+j))/l_f64Sum;
				
				findMaximum(m_pNormalizedAccumulatedEvidence->getBuffer(), m_ui32MaximumIndex, l_f32Maximum);
				m_bEarlyStoppingConditionMet = true;
				for (uint32 j=0; j<l_pInputMatrix->getBufferElementCount(); j++)
					if (m_ui32MaximumIndex!=j && *(m_pNormalizedAccumulatedEvidence->getBuffer()+j)>l_f32Maximum-m_bStopCondition)
						m_bEarlyStoppingConditionMet = false;				
				m_bReceivedSomeEvidence = true;	
			}
			
			//each time an update of the evidence comes in, output the total evidence already accumulated
			m_oAlgo3_FeatureVectorEncoder.encodeBuffer(1);
			l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(1, i), l_rDynamicBoxContext.getInputChunkEndTime(1, i));												
		}
		if(m_oAlgo1_FeatureVectorDecoder.isEndReceived())
		{
			//OpenViBEToolkit::Tools::Matrix::copyDescription(*m_oAlgo3_FeatureVectorEncoder.getInputMatrix(), *l_pInputMatrix);
			m_oAlgo3_FeatureVectorEncoder.encodeEnd(1);
			l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(1, i), l_rDynamicBoxContext.getInputChunkEndTime(1, i));
		}
	}	
	
	return true;
}

void CBoxAlgorithmEvidenceAccumulator::findMaximum(float64* vector, uint32& l_ui32MaximumIndex, float32& l_f32Maximum)
{
	l_f32Maximum = std::numeric_limits<int>::min();
	l_ui32MaximumIndex = 0;
	for (uint32 j=0; j<m_pAccumulatedEvidence->getBufferElementCount(); j++)
		if (*(vector+j)>l_f32Maximum)
		{
			l_f32Maximum = *(vector+j);
			l_ui32MaximumIndex = j;
		}	
		
	//return  l_ui32MaximumIndex;
}

/*void CBoxAlgorithmEvidenceAccumulator::findMaximum(uint32& l_ui32MaximumIndex, float32& l_f32Maximum)
{
	l_f32Maximum = std::numeric_limits<int>::min();
	l_ui32MaximumIndex = 0;
	for (uint32 j=0; j<m_pAccumulatedEvidence->getBufferElementCount(); j++)
		if (*(m_pAccumulatedEvidence->getBuffer()+j)>l_f32Maximum)
		{
			l_f32Maximum = *(m_pAccumulatedEvidence->getBuffer()+j);
			l_ui32MaximumIndex = j;
		}	
		
	//return  l_ui32MaximumIndex;
}*/
