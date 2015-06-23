#include "ovpCBoxAlgorithmConfusionMatrix.h"

#include "../algorithms/ovpCAlgorithmConfusionMatrix.h"
#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Measurement;
using namespace std;

boolean CBoxAlgorithmConfusionMatrix::initialize(void)
{
	//Initialize input/output
	m_oTargetStimulationDecoder.initialize(*this, 0);
	m_oClassifierStimulationDecoder.initialize(*this, 1);

	m_oOutputMatrixEncoder.initialize(*this, 0);

	//CONFUSION MATRIX ALGORITHM
	m_pConfusionMatrixAlgorithm = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ConfusionMatrix));
	m_pConfusionMatrixAlgorithm->initialize();

	CString l_sPercentageSetting;
	TParameterHandler < OpenViBE::boolean > ip_bPercentages(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Percentage));
	getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(0,l_sPercentageSetting);
	ip_bPercentages = this->getConfigurationManager().expandAsBoolean(l_sPercentageSetting);

	CString l_sSumsSetting;
	TParameterHandler<boolean> ip_bSums(m_pConfusionMatrixAlgorithm->getInputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Sums));
	getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(1,l_sSumsSetting);
	ip_bSums = this->getConfigurationManager().expandAsBoolean(l_sSumsSetting);

	uint32 l_ui32ClassCount=getBoxAlgorithmContext()->getStaticBoxContext()->getSettingCount() - FIRST_CLASS_SETTING_INDEX;
	vector < uint64 > l_vClassCodes;
	l_vClassCodes.resize(l_ui32ClassCount);
	for(uint32 i = 0; i< l_ui32ClassCount; i++)
	{
		CString l_sClassValue;
		getStaticBoxContext().getSettingValue(i+FIRST_CLASS_SETTING_INDEX, l_sClassValue); // classes are settings from 2 to n
		l_vClassCodes[i] =(uint64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i+FIRST_CLASS_SETTING_INDEX);
	}
	// verification...
	for(uint32 i = 0; i< l_ui32ClassCount; i++)
	{
		for(uint32 j = i+1; j< l_ui32ClassCount; j++)
		{
			if(l_vClassCodes[i] == l_vClassCodes[j])
			{
				CString l_sClassValue;
				getStaticBoxContext().getSettingValue(i+FIRST_CLASS_SETTING_INDEX, l_sClassValue);
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

	TParameterHandler < IMatrix* > op_pConfusionMatrix(m_pConfusionMatrixAlgorithm->getOutputParameter(OVP_Algorithm_ConfusionMatrixAlgorithm_OutputParameterId_ConfusionMatrix));
	m_oOutputMatrixEncoder.getInputMatrix().setReferenceTarget(op_pConfusionMatrix);

	return true;
}

boolean CBoxAlgorithmConfusionMatrix::uninitialize(void)
{
	m_pConfusionMatrixAlgorithm->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_pConfusionMatrixAlgorithm);

	m_oOutputMatrixEncoder.uninitialize();
	m_oTargetStimulationDecoder.uninitialize();
	m_oClassifierStimulationDecoder.uninitialize();

	return true;
}

boolean CBoxAlgorithmConfusionMatrix::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmConfusionMatrix::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//Input 0: Targets
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		m_oTargetStimulationDecoder.decode(i);

		if(m_oTargetStimulationDecoder.isHeaderReceived())
		{
			m_pConfusionMatrixAlgorithm->process(OVP_Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetTarget);

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
