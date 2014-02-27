#include "ovpCBoxAlgorithmMulticlassClassifierTrainer.h"

#include <system/Memory.h>

#include <fstream>
#include <cmath>
#include <algorithm>

#include <xml/IXMLHandler.h>
#include <xml/IXMLNode.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;
using namespace std;

boolean CBoxAlgorithmMulticlassClassifierTrainer::initialize(void)
{
	uint32 i;
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	CIdentifier l_oClassifierAlgorithmClassIdentifier;
	CString l_sClassifierAlgorithmClassIdentifier;
	l_rStaticBoxContext.getSettingValue(0, l_sClassifierAlgorithmClassIdentifier);

	m_ui64TrainStimulation=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

	l_oClassifierAlgorithmClassIdentifier=this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_ClassificationAndPairingAlgorithm, l_sClassifierAlgorithmClassIdentifier);

	if(l_oClassifierAlgorithmClassIdentifier==OV_UndefinedIdentifier)
	{
		this->getLogManager() << LogLevel_Error << "Unknown classifier algorithm [" << l_sClassifierAlgorithmClassIdentifier << "]\n";
		return false;
	}


	int64 l_i64PartitionCount=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
	if(l_i64PartitionCount<0)
	{
		this->getLogManager() << LogLevel_Error << "Partition count can not be less than 0 (was " << l_i64PartitionCount << ")\n";
		return false;
	}
	m_ui64PartitionCount=uint64(l_i64PartitionCount);

	m_pStimulationsDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamDecoder));
	m_pStimulationsDecoder->initialize();

	m_pStimulationsEncoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamEncoder));
	m_pStimulationsEncoder->initialize();


	for(i=1; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		m_vFeatureVectorsDecoder[i-1]=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorStreamDecoder));
		m_vFeatureVectorsDecoder[i-1]->initialize();
	}

	uint32 l_iClassCount = l_rStaticBoxContext.getInputCount() -1;

	this->getLogManager() << LogLevel_Info << "Got only 2 classes so let's use classic trainer\n";

	m_pClassifier=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(l_oClassifierAlgorithmClassIdentifier));
	m_pClassifier->initialize();

	//Format extra settings to give them to the box
	i = OVP_BoxAlgorithm_MulticlassClassifierTrainer_CommonSettingsCount; // number of settings when no additional setting is added

	m_pExtraParemeter = new map<CString , CString> ();
	while(i < l_rStaticBoxContext.getSettingCount())
	{
		CString l_pInputName;
		CString l_pInputValue;
		l_rStaticBoxContext.getSettingName(i, l_pInputName);
		l_rStaticBoxContext.getSettingValue(i, l_pInputValue);
		(*m_pExtraParemeter)[l_pInputName] = l_pInputValue;

		cout << "Extra parameter " << l_pInputName << " (" << (*m_pExtraParemeter)[l_pInputName] << ")" << endl;
		++i;
	}
	TParameterHandler < map<CString , CString> * > ip_pExtraParameter(m_pClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));
	ip_pExtraParameter = m_pExtraParemeter;


	//If we have to deal with a pairing strategy we have to pass argument
	if(m_pClassifier->isAlgorithmDerivedFrom(OVTK_ClassId_Algorithm_PairingStrategy))
	{
		TParameterHandler < uint64* > ip_pClassAmount(m_pClassifier->getInputParameter(OVTK_Algorithm_PairingStrategy_InputParameterId_ClassAmount));
		*ip_pClassAmount = l_iClassCount;
		m_pClassifier->process(OVTK_Algorithm_PairingStrategy_InputTriggerId_DesignArchitecture);
	}

	return true;
}

boolean CBoxAlgorithmMulticlassClassifierTrainer::uninitialize(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	if(m_pClassifier != NULL)
	{
		m_pClassifier->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pClassifier);
	}

	m_pStimulationsDecoder->uninitialize();
	m_pStimulationsEncoder->uninitialize();


	this->getAlgorithmManager().releaseAlgorithm(*m_pStimulationsDecoder);
	this->getAlgorithmManager().releaseAlgorithm(*m_pStimulationsEncoder);

	for(uint32 i=1; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		m_vFeatureVectorsDecoder[i-1]->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_vFeatureVectorsDecoder[i-1]);
	}
	m_vFeatureVectorsDecoder.clear();

	for(uint32 i=0;i<m_vFeatureVector.size();i++) {
		delete m_vFeatureVector[i].m_pFeatureVectorMatrix;
		m_vFeatureVector[i].m_pFeatureVectorMatrix = NULL;
	}
	m_vFeatureVector.clear();
	m_vFeatureVectorIndex.clear();

	return true;
}

boolean CBoxAlgorithmMulticlassClassifierTrainer::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmMulticlassClassifierTrainer::process(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	uint32 i, j;
	boolean l_bTrainStimulationReceived=false;

	// Parses stimulations
	for(i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		TParameterHandler < const IMemoryBuffer* > ip_pMemoryBuffer(m_pStimulationsDecoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_InputParameterId_MemoryBufferToDecode));
		TParameterHandler < const IStimulationSet* > op_pStimulationSet(m_pStimulationsDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_OutputParameterId_StimulationSet));

		ip_pMemoryBuffer=l_rDynamicBoxContext.getInputChunk(0, i);

		TParameterHandler < IStimulationSet* > ip_pStimulationSet(m_pStimulationsEncoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_InputParameterId_StimulationSet));
		TParameterHandler < IMemoryBuffer* > op_pEncodedMemoryBuffer(m_pStimulationsEncoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_OutputParameterId_EncodedMemoryBuffer));

		CStimulationSet l_oStimulationSet;
		ip_pStimulationSet=&l_oStimulationSet;

		if(l_rStaticBoxContext.getOutputCount()>=1)
		{
			op_pEncodedMemoryBuffer=l_rDynamicBoxContext.getOutputChunk(0);
		}
		else
		{
			op_pEncodedMemoryBuffer->setSize(0, true);
		}

		m_pStimulationsDecoder->process();

		if(m_pStimulationsDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			m_pStimulationsEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeHeader);
		}
		if(m_pStimulationsDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedBuffer))
		{
			for(uint64 j=0; j<op_pStimulationSet->getStimulationCount(); j++)
			{
				if(op_pStimulationSet->getStimulationIdentifier(j)==m_ui64TrainStimulation)
				{
					l_bTrainStimulationReceived=true;

					this->getLogManager() << LogLevel_Info << "Raising train-completed Flag.\n";
					uint64 l_ui32TrainCompletedStimulation = this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation,"OVTK_StimulationId_TrainCompleted");
					l_oStimulationSet.appendStimulation(l_ui32TrainCompletedStimulation, op_pStimulationSet->getStimulationDate(j), 0);
				}
			}
			m_pStimulationsEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeBuffer);
		}
		if(m_pStimulationsDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedEnd))
		{
			m_pStimulationsEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeEnd);
		}

		if(l_rStaticBoxContext.getOutputCount()>=1)
		{
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}

	// Parses feature vectors
	for(i=1; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		for(j=0; j<l_rDynamicBoxContext.getInputChunkCount(i); j++)
		{
			TParameterHandler < const IMemoryBuffer* > ip_pFeatureVectorMemoryBuffer(m_vFeatureVectorsDecoder[i-1]->getInputParameter(OVP_GD_Algorithm_FeatureVectorStreamDecoder_InputParameterId_MemoryBufferToDecode));
			TParameterHandler < const IMatrix* > op_pFeatureVectorMatrix(m_vFeatureVectorsDecoder[i-1]->getOutputParameter(OVP_GD_Algorithm_FeatureVectorStreamDecoder_OutputParameterId_Matrix));
			ip_pFeatureVectorMemoryBuffer=l_rDynamicBoxContext.getInputChunk(i, j);
			m_vFeatureVectorsDecoder[i-1]->process();
			if(m_vFeatureVectorsDecoder[i-1]->isOutputTriggerActive(OVP_GD_Algorithm_FeatureVectorStreamDecoder_OutputTriggerId_ReceivedHeader))
			{
			}
			if(m_vFeatureVectorsDecoder[i-1]->isOutputTriggerActive(OVP_GD_Algorithm_FeatureVectorStreamDecoder_OutputTriggerId_ReceivedBuffer))
			{
				CBoxAlgorithmMulticlassClassifierTrainer::SFeatureVector l_oFeatureVector;
				l_oFeatureVector.m_pFeatureVectorMatrix=new CMatrix();
				l_oFeatureVector.m_ui64StartTime=l_rDynamicBoxContext.getInputChunkStartTime(i, j);
				l_oFeatureVector.m_ui64EndTime=l_rDynamicBoxContext.getInputChunkEndTime(i, j);
				l_oFeatureVector.m_ui32InputIndex=i;
				OpenViBEToolkit::Tools::Matrix::copy(*l_oFeatureVector.m_pFeatureVectorMatrix, *op_pFeatureVectorMatrix);
				m_vFeatureVector.push_back(l_oFeatureVector);
				m_vFeatureCount[i]++;
			}
			if(m_vFeatureVectorsDecoder[i-1]->isOutputTriggerActive(OVP_GD_Algorithm_FeatureVectorStreamDecoder_OutputTriggerId_ReceivedEnd))
			{
			}
			l_rDynamicBoxContext.markInputAsDeprecated(i, j);
		}
	}

	// On train stimulation reception, build up the labelled feature vector set matrix and go on training
	if(l_bTrainStimulationReceived)
	{


		if(m_vFeatureVector.size()<m_ui64PartitionCount)
		{
			this->getLogManager() << LogLevel_Error << "Fewer examples (" << (uint32)m_vFeatureVector.size() << ") than the specified partition count (" << m_ui64PartitionCount << ").\n";
			return false;
		}
		if(m_vFeatureVector.size()==0)
		{
			this->getLogManager() << LogLevel_Warning << "Received train stimulation but no training examples received\n";
		}
		else
		{
			this->getLogManager() << LogLevel_Info << "Received train stimulation. Data dim is [" << (uint32) m_vFeatureVector.size() << "x"
								  << m_vFeatureVector[0].m_pFeatureVectorMatrix->getBufferElementCount() << "]\n";
			for(i=1; i<l_rStaticBoxContext.getInputCount(); i++)
			{
				this->getLogManager() << LogLevel_Trace << "For information, we have " << m_vFeatureCount[i] << " feature vector(s) for input " << i << "\n";
			}

			TParameterHandler < XML::IXMLNode* > op_pConfiguration(m_pClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));

			float64 l_f64PartitionAccuracy=0;
			float64 l_f64FinalAccuracy=0;
			vector<float64> l_vPartitionAccuracies((unsigned int)m_ui64PartitionCount);

			boolean l_bRandomizeVectorOrder = (&(this->getConfigurationManager()))->expandAsBoolean("${Plugin_Classification_RandomizeKFoldTestData}");

			// create a vector used for mapping feature vectors (initialize it as v[i] = i)
			for (uint32 i = 0; i < m_vFeatureVector.size(); i++)
			{
				m_vFeatureVectorIndex.push_back(i);
			}

			// randomize the vector if necessary
			if (l_bRandomizeVectorOrder)
			{
				this->getLogManager() << LogLevel_Info << "Randomizing the feature vector set\n";
				random_shuffle(m_vFeatureVectorIndex.begin(), m_vFeatureVectorIndex.end());

			}

			if(m_ui64PartitionCount>=2)
			{
				this->getLogManager() << LogLevel_Info << this->getStaticBoxContext().getInputCount();
				this->getLogManager() << LogLevel_Info << "k-fold test could take quite a long time, be patient\n";
				for(uint64 i=0; i<m_ui64PartitionCount; i++)
				{
					size_t l_uiStartIndex=(size_t)(((i  )*m_vFeatureVector.size())/m_ui64PartitionCount);
					size_t l_uiStopIndex =(size_t)(((i+1)*m_vFeatureVector.size())/m_ui64PartitionCount);

					this->getLogManager() << LogLevel_Trace << "Training on partition " << i << " (feature vectors " << (uint32)l_uiStartIndex << " to " << (uint32)l_uiStopIndex-1 << ")...\n";
					if(this->train(l_uiStartIndex, l_uiStopIndex))
					{
						l_f64PartitionAccuracy=this->getAccuracy(l_uiStartIndex, l_uiStopIndex);
						l_vPartitionAccuracies[(unsigned int)i]=l_f64PartitionAccuracy;
						l_f64FinalAccuracy+=l_f64PartitionAccuracy;
					}
					this->getLogManager() << LogLevel_Info << "Finished with partition " << i+1 << " / " << m_ui64PartitionCount << " (performance : " << l_f64PartitionAccuracy << "%)\n";
				}

				float64 l_fMean = l_f64FinalAccuracy/m_ui64PartitionCount;
				float64 l_fDeviation = 0;

				for (uint64 i = 0; i < m_ui64PartitionCount; i++)
				{
					float64 l_fDiff = l_vPartitionAccuracies[(unsigned int)i] - l_fMean;
					l_fDeviation += l_fDiff * l_fDiff;
				}
				l_fDeviation = sqrt( l_fDeviation / m_ui64PartitionCount );

				this->getLogManager() << LogLevel_Info << "Cross-validation test accuracy is " << l_fMean << "% (sigma = " << l_fDeviation << "%)\n";
			}
			else
			{
				this->getLogManager() << LogLevel_Info << "Training without cross-validation.\n";
				this->getLogManager() << LogLevel_Info << "*** Reported training set accuracy will be optimistic ***\n";
			}

			this->getLogManager() << LogLevel_Trace << "Training final classifier on the whole set...\n";
			this->train(0, 0);

			this->getLogManager() << LogLevel_Info << "Training set accuracy is " << this->getAccuracy(0, m_vFeatureVector.size()) << "% (optimistic)\n";

			XML::IXMLHandler *l_pHandler = XML::createXMLHandler();
			CString l_sConfigurationFilename(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));

			if(!l_pHandler->writeXMLInFile(*op_pConfiguration, l_sConfigurationFilename.toASCIIString()))
			{
				this->getLogManager() << LogLevel_Error << "Could not save configuration to file [" << l_sConfigurationFilename << "]\n";
				return false;
			}
		}
	}

	return true;
}

boolean CBoxAlgorithmMulticlassClassifierTrainer::train(const size_t uiStartIndex, const size_t uiStopIndex)
{
	this->getLogManager() << LogLevel_Info << "Starting training\n";
	if(uiStopIndex-uiStartIndex-1==0)
	{
		return false;
	}

	uint32 l_ui32FeatureVectorCount=m_vFeatureVector.size()-(uiStopIndex-uiStartIndex);
	uint32 l_ui32FeatureVectorSize=m_vFeatureVector[0].m_pFeatureVectorMatrix->getBufferElementCount();

	TParameterHandler < IMatrix* > ip_pFeatureVectorSet(m_pClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet));
	ip_pFeatureVectorSet->setDimensionCount(2);
	ip_pFeatureVectorSet->setDimensionSize(0, l_ui32FeatureVectorCount);
	ip_pFeatureVectorSet->setDimensionSize(1, l_ui32FeatureVectorSize+1);

	float64* l_pFeatureVectorSetBuffer=ip_pFeatureVectorSet->getBuffer();
	for(size_t j=0; j<m_vFeatureVector.size()-(uiStopIndex-uiStartIndex); j++)
	{
		size_t k=m_vFeatureVectorIndex[(j<uiStartIndex?j:j+(uiStopIndex-uiStartIndex))];
		float64 l_f64Class=(float64)m_vFeatureVector[k].m_ui32InputIndex;
		System::Memory::copy(
					l_pFeatureVectorSetBuffer,
					m_vFeatureVector[k].m_pFeatureVectorMatrix->getBuffer(),
					l_ui32FeatureVectorSize*sizeof(float64));

		l_pFeatureVectorSetBuffer[l_ui32FeatureVectorSize]=l_f64Class;
		l_pFeatureVectorSetBuffer+=(l_ui32FeatureVectorSize+1);
	}

	m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Train);

	m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_SaveConfiguration);

	return true;
}

float64 CBoxAlgorithmMulticlassClassifierTrainer::getAccuracy(const size_t uiStartIndex, const size_t uiStopIndex)
{
	size_t l_iSuccessfullTrainerCount=0;

	if(uiStopIndex-uiStartIndex==0)
	{
		return 0;
	}

	uint32 l_ui32FeatureVectorSize=m_vFeatureVector[0].m_pFeatureVectorMatrix->getBufferElementCount();

	TParameterHandler < XML::IXMLNode* > op_pConfiguration(m_pClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	XML::IXMLNode * l_pNode = op_pConfiguration;//Requested for affectation
	TParameterHandler < XML::IXMLNode* > ip_pConfiguration(m_pClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_Configuration));
	ip_pConfiguration = l_pNode;

	m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_LoadConfiguration);

	TParameterHandler < IMatrix* > ip_pFeatureVector(m_pClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVector));
	TParameterHandler < float64 > op_f64ClassificationStateClass(m_pClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Class));
	ip_pFeatureVector->setDimensionCount(1);
	ip_pFeatureVector->setDimensionSize(0, l_ui32FeatureVectorSize);

	for(size_t j=uiStartIndex; j<uiStopIndex; j++)
	{
		size_t k = m_vFeatureVectorIndex[j];

		float64* l_pFeatureVectorBuffer=ip_pFeatureVector->getBuffer();
		float64 l_f64TrainerClass=(float64)m_vFeatureVector[k].m_ui32InputIndex;
		System::Memory::copy(
					l_pFeatureVectorBuffer,
					m_vFeatureVector[k].m_pFeatureVectorMatrix->getBuffer(),
					l_ui32FeatureVectorSize*sizeof(float64));

		m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Classify);

		if(op_f64ClassificationStateClass==l_f64TrainerClass)
		{
			l_iSuccessfullTrainerCount++;
		}
	}

	return (float64)(l_iSuccessfullTrainerCount*100.0)/(uiStopIndex-uiStartIndex);
}
