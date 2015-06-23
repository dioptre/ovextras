#include "ovpCBoxAlgorithmKappaCoefficient.h"
#include "../algorithms/ovpCAlgorithmConfusionMatrix.h"

#include <xml/IXMLHandler.h>
#include <sstream>
#include <vector>

using namespace std;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Measurement;

namespace{
	const uint32 c_ui32ClassLabelOffset = 1;
}

boolean CBoxAlgorithmKappaCoefficient::initialize(void)
{
	//Initialize input/output
	m_oTargetStimulationDecoder.initialize(*this, 0);
	m_oClassifierStimulationDecoder.initialize(*this, 1);

	m_oOutputMatrixEncoder.initialize(*this, 0);

	//CONFUSION MATRIX ALGORITHM
	m_pConfusionMatrixAlgorithm = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ConfusionMatrix));
	m_pConfusionMatrixAlgorithm->initialize();

	TParameterHandler < OpenViBE::boolean > ip_bPercentages(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Percentage));
	ip_bPercentages = false;

	TParameterHandler<boolean> ip_bSums(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Sums));
	ip_bSums = false;

	m_ui32AmountClass=getBoxAlgorithmContext()->getStaticBoxContext()->getSettingCount() - c_ui32ClassLabelOffset;
	vector < uint64 > l_vClassCodes;
	l_vClassCodes.resize(m_ui32AmountClass);
	for(uint32 i = 0; i< m_ui32AmountClass; i++)
	{
		CString l_sClassValue;
		getStaticBoxContext().getSettingValue(i+c_ui32ClassLabelOffset, l_sClassValue); // classes are settings from 2 to n
		l_vClassCodes[i] =(uint64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i+c_ui32ClassLabelOffset);
	}
	// verification...
	for(uint32 i = 0; i< m_ui32AmountClass; i++)
	{
		for(uint32 j = i+1; j< m_ui32AmountClass; j++)
		{
			if(l_vClassCodes[i] == l_vClassCodes[j])
			{
				CString l_sClassValue;
				getStaticBoxContext().getSettingValue(i+c_ui32ClassLabelOffset, l_sClassValue);
				getLogManager() << LogLevel_Error << "You must use unique classes to compute a confusion matrix. Class "<<i+1<<" and "<<j+1<< " are the same ("<<l_sClassValue.toASCIIString()<<").\n";
				return false;
			}
		}
	}

	TParameterHandler < OpenViBE::IStimulationSet* > ip_pClassesCodes(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassCodes));
	for(uint32 i = 0 ; i<l_vClassCodes.size(); i++)
	{
		ip_pClassesCodes->appendStimulation(l_vClassCodes[i],0,0);
	}

	//Link all input/output
	TParameterHandler < IStimulationSet* > ip_pClassifierStimulationSet(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassifierStimulationSet));
	ip_pClassifierStimulationSet.setReferenceTarget(m_oClassifierStimulationDecoder.getOutputStimulationSet());

	TParameterHandler < IStimulationSet* > ip_pTargetStimulationSet(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_TargetStimulationSet));
	ip_pTargetStimulationSet.setReferenceTarget(m_oTargetStimulationDecoder.getOutputStimulationSet());

	op_pConfusionMatrix.initialize(m_pConfusionMatrixAlgorithm->getOutputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_OutputParameterId_ConfusionMatrix));

	return true;
}

boolean CBoxAlgorithmKappaCoefficient::uninitialize(void)
{
	m_pConfusionMatrixAlgorithm->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_pConfusionMatrixAlgorithm);

	m_oOutputMatrixEncoder.uninitialize();
	m_oTargetStimulationDecoder.uninitialize();
	m_oClassifierStimulationDecoder.uninitialize();

	return true;
}


boolean CBoxAlgorithmKappaCoefficient::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CBoxAlgorithmKappaCoefficient::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//Input 0: Targets
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		m_oTargetStimulationDecoder.decode(i);

		if(m_oTargetStimulationDecoder.isHeaderReceived())
		{
			m_pConfusionMatrixAlgorithm->process(OVP_Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetTarget);

			m_oOutputMatrixEncoder.getInputMatrix()->setDimensionCount(1);
			m_oOutputMatrixEncoder.getInputMatrix()->setDimensionLabel(0, 0, "Kappa");
			m_oOutputMatrixEncoder.getInputMatrix()->setDimensionSize(0, 1);

			m_oOutputMatrixEncoder.encodeHeader();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			m_ui64CurrentProcessingTimeLimit = 0;
		}

		if(m_oTargetStimulationDecoder.isBufferReceived())
		{
			uint64 l_ui64ChunkEndTime = l_rDynamicBoxContext.getInputChunkEndTime(0, i);
			m_ui64CurrentProcessingTimeLimit = (l_ui64ChunkEndTime>m_ui64CurrentProcessingTimeLimit?l_ui64ChunkEndTime:m_ui64CurrentProcessingTimeLimit);
			m_pConfusionMatrixAlgorithm->process(OVP_Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedTarget);
		}

		if(m_oTargetStimulationDecoder.isEndReceived())
		{
			m_oOutputMatrixEncoder.encodeEnd();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

	}

	//Input 1: Classifier results
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(1); i++)
	{
		uint64 l_ui64ChunkEndTime = l_rDynamicBoxContext.getInputChunkEndTime(1,i);
		if(l_ui64ChunkEndTime <= m_ui64CurrentProcessingTimeLimit)
		{
			m_oClassifierStimulationDecoder.decode(i);

			if(m_oClassifierStimulationDecoder.isHeaderReceived())
			{
				m_pConfusionMatrixAlgorithm->process(OVP_Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetClassifier);
			}

			if(m_oClassifierStimulationDecoder.isBufferReceived())
			{
				m_pConfusionMatrixAlgorithm->process(OVP_Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedClassifier);
				if(m_pConfusionMatrixAlgorithm->isOutputTriggerActive(OVP_Algorithm_ConfusionMatrixAlgorithm_OutputTriggerId_ConfusionPerformed))
				{
					//The confusion matrix has changed so we need to update the kappa coefficient
					float64* l_pConfusionMatrix = op_pConfusionMatrix->getBuffer();
					//First we need the amount of sample that have been classified
					uint32 l_ui32Total = 0;
					for(size_t j =0; j < m_ui32AmountClass * m_ui32AmountClass ; ++j)
					{
						l_ui32Total += l_pConfusionMatrix[j];
					}

					//Now we gonna compute the two sum we need to compute the kappa coefficient
					//It's more easy to use a double loop
					float64 l_f64ObservedAccurancy = 0;
					float64 l_f64ExpectedAccurancy = 0;

					for(size_t j =0; j < m_ui32AmountClass ; ++j)
					{
						l_f64ObservedAccurancy += l_pConfusionMatrix[j*m_ui32AmountClass + j];

						for(size_t k =0; k < m_ui32AmountClass ; ++k)
						{
							l_f64ExpectedAccurancy += (l_pConfusionMatrix[m_ui32AmountClass* j + k] * l_pConfusionMatrix[m_ui32AmountClass * k + j]);
						}
					}
					l_f64ObservedAccurancy /= l_ui32Total;
					l_f64ExpectedAccurancy /= (l_ui32Total * l_ui32Total);

					float64 l_f64KappaCoefficient = (l_f64ObservedAccurancy - l_f64ExpectedAccurancy)/(1 - l_f64ExpectedAccurancy);

					m_oOutputMatrixEncoder.getInputMatrix()->getBuffer()[0]=l_f64KappaCoefficient;
					m_oOutputMatrixEncoder.encodeBuffer();
					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(1, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
				}
			}

			if(m_oClassifierStimulationDecoder.isEndReceived())
			{
				m_oOutputMatrixEncoder.encodeEnd();
				l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(1, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			}

			l_rDynamicBoxContext.markInputAsDeprecated(1, i);
		}
	}

	return true;
}
