#include "ovpCBoxAlgorithmClassifierTrainer.h"

#include <system/ovCMemory.h>
#include <system/ovCMath.h>

#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

#include <map>

#include <iomanip> // setw

#include <xml/IXMLHandler.h>
#include <xml/IXMLNode.h>

//This needs to reachable from outside
const char* const c_sClassifierRoot = "OpenViBE-Classifier";

const char* const c_sXmlVersionAttributeName = "XMLVersion";
const char* const c_sIdentifierAttributeName = "class-id";

const char* const c_sStrategyNodeName = "Strategy-Identifier";
const char* const c_sAlgorithmNodeName = "Algorithm-Identifier";
const char* const c_sStimulationsNodeName = "Stimulations";
const char* const c_sRejectedClassNodeName = "Rejected-Class";
const char* const c_sClassStimulationNodeName = "Class-Stimulation";

const char* const c_sClassificationBoxRoot = "OpenViBE-Classifier-Box";

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;
using namespace std;

boolean CBoxAlgorithmClassifierTrainer::initialize(void)
{
	m_pClassifier = NULL;
	m_pStimulationsDecoder = NULL;
	m_pStimulationsEncoder = NULL;

	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	//As we add some parameter in the middle of "static" parameters, we cannot rely on settings index.
	m_pParameter = new map<CString , CString> ();
	for(uint32 i = 0 ; i < l_rStaticBoxContext.getSettingCount() ; ++i)
	{
		CString l_pInputName;
		CString l_pInputValue;
		l_rStaticBoxContext.getSettingName(i, l_pInputName);
		l_rStaticBoxContext.getSettingValue(i, l_pInputValue);
		(*m_pParameter)[l_pInputName] = l_pInputValue;

	}

	boolean l_bIsPairing=false;


	CString l_sConfigurationFilename(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2));
	if(l_sConfigurationFilename == CString(""))
	{
		this->getLogManager() << LogLevel_Error << "An output filename is required\n";
		return false;
	}

	CIdentifier l_oStrategyClassIdentifier, l_oClassifierAlgorithmClassIdentifier;

	l_oStrategyClassIdentifier=this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_ClassificationStrategy, (*m_pParameter)[c_sMulticlassStrategySettingName]);
	l_oClassifierAlgorithmClassIdentifier=this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_ClassificationAlgorithm, (*m_pParameter)[c_sAlgorithmSettingName]);

	if(l_oStrategyClassIdentifier==OV_UndefinedIdentifier)
	{
		//That means that we want to use a classical algorithm so just let's create it
		const CIdentifier l_oClassifierAlgorithmIdentifier = this->getAlgorithmManager().createAlgorithm(l_oClassifierAlgorithmClassIdentifier);
		if(l_oClassifierAlgorithmIdentifier == OV_UndefinedIdentifier)
		{
			this->getLogManager() << LogLevel_Error << "Error instantiating classifier class with id " 
				<< l_oClassifierAlgorithmClassIdentifier
				<< ". If you've loaded an old scenario or configuration file(s), make sure that the classifiers specified in it are still available.\n";
			return false;
		}

		m_pClassifier=&this->getAlgorithmManager().getAlgorithm(l_oClassifierAlgorithmIdentifier);
		m_pClassifier->initialize();
	}
	else
	{
		l_bIsPairing = true;
		m_pClassifier=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(l_oStrategyClassIdentifier));
		m_pClassifier->initialize();
	}
	m_ui64TrainStimulation = this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation ,(*m_pParameter)[c_sTrainTriggerSettingName]);

	int64 l_i64PartitionCount = this->getConfigurationManager().expandAsInteger((*m_pParameter)[c_sKFoldSettingName]);
	if(l_i64PartitionCount<0)
	{
		this->getLogManager() << LogLevel_Error << "Partition count can not be less than 0 (was " << l_i64PartitionCount << ")\n";
		return false;
	}
	m_ui64PartitionCount=uint64(l_i64PartitionCount);

	for(uint32 i=1; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		m_vFeatureVectorsDecoder[i-1]=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorStreamDecoder));
		m_vFeatureVectorsDecoder[i-1]->initialize();
	}

	m_pStimulationsDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamDecoder));
	m_pStimulationsDecoder->initialize();

	//We link the parameters to the extra parameters input parameter to transmit them
	TParameterHandler < map<CString , CString> * > ip_pExtraParameter(m_pClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));
	ip_pExtraParameter = m_pParameter;

	m_pStimulationsEncoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamEncoder));
	m_pStimulationsEncoder->initialize();

	m_vFeatureCount.clear();

	//If we have to deal with a pairing strategy we have to pass argument
	if(l_bIsPairing)
	{
		TParameterHandler < uint64 > ip_pClassCount(m_pClassifier->getInputParameter(OVTK_Algorithm_PairingStrategy_InputParameterId_ClassCount));
		if(l_rStaticBoxContext.getInputCount()==0)
		{
			// This shouldn't happen.
			this->getLogManager() << LogLevel_Error << "Must have more than 0 inputs\n";
			return false;
		}
		ip_pClassCount = l_rStaticBoxContext.getInputCount() -1;	 // >=0 by above test. -1 because one input connector is for stimulations.
		TParameterHandler < CIdentifier* > ip_oClassId(m_pClassifier->getInputParameter(OVTK_Algorithm_PairingStrategy_InputParameterId_SubClassifierAlgorithm));
		ip_oClassId = &l_oClassifierAlgorithmClassIdentifier;
		if(!m_pClassifier->process(OVTK_Algorithm_PairingStrategy_InputTriggerId_DesignArchitecture))
		{
			//This call can return false if there is no function to compare to classification register
			return false;
		}
	}

	return true;
}

boolean CBoxAlgorithmClassifierTrainer::uninitialize(void)
{

	// IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
	if(m_pClassifier) 
	{
		m_pClassifier->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pClassifier);
	}
	if(m_pStimulationsDecoder)
	{
		m_pStimulationsDecoder->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pStimulationsDecoder);
	}
	if(m_pStimulationsEncoder)
	{
		m_pStimulationsEncoder->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pStimulationsEncoder);
	}

	for(uint32 i=0; i<m_vFeatureVectorsDecoder.size(); i++)
	{
		m_vFeatureVectorsDecoder[i]->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_vFeatureVectorsDecoder[i]);
	}
	m_vFeatureVectorsDecoder.clear();

	for(uint32 i=0;i<m_vFeatureVector.size();i++) {
		delete m_vFeatureVector[i].m_pFeatureVectorMatrix;
		m_vFeatureVector[i].m_pFeatureVectorMatrix = NULL;
	}
	m_vFeatureVector.clear();
	m_vFeatureVectorIndex.clear();

	// @fixme who frees this? freeing here -> crash
	/*
	if(m_pExtraParameter != NULL)
	{
		delete m_pExtraParameter;
		m_pExtraParameter = NULL;
	}
	*/

	return true;
}

boolean CBoxAlgorithmClassifierTrainer::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmClassifierTrainer::process(void)
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

					this->getLogManager() << LogLevel_Trace << "Raising train-completed Flag.\n";
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
				CBoxAlgorithmClassifierTrainer::SFeatureVector l_oFeatureVector;
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
				random_shuffle(m_vFeatureVectorIndex.begin(), m_vFeatureVectorIndex.end(), System::Math::randomUInteger32WithCeiling);
			}

			const uint32 l_ui32ClassCount = l_rStaticBoxContext.getInputCount() - 1;
			CMatrix l_oConfusion;
			l_oConfusion.setDimensionCount(2);
			l_oConfusion.setDimensionSize(0, l_ui32ClassCount);
			l_oConfusion.setDimensionSize(1, l_ui32ClassCount);

			if(m_ui64PartitionCount>=2)
			{

				OpenViBEToolkit::Tools::Matrix::clearContent(l_oConfusion);

				this->getLogManager() << LogLevel_Info << "k-fold test could take quite a long time, be patient\n";
				for(uint64 i=0; i<m_ui64PartitionCount; i++)
				{
					size_t l_uiStartIndex=(size_t)(((i  )*m_vFeatureVector.size())/m_ui64PartitionCount);
					size_t l_uiStopIndex =(size_t)(((i+1)*m_vFeatureVector.size())/m_ui64PartitionCount);

					this->getLogManager() << LogLevel_Trace << "Training on partition " << i << " (feature vectors " << (uint32)l_uiStartIndex << " to " << (uint32)l_uiStopIndex-1 << ")...\n";
					if(this->train(l_uiStartIndex, l_uiStopIndex))
					{
						l_f64PartitionAccuracy=this->getAccuracy(l_uiStartIndex, l_uiStopIndex, l_oConfusion);
						l_vPartitionAccuracies[(unsigned int)i]=l_f64PartitionAccuracy;
						l_f64FinalAccuracy+=l_f64PartitionAccuracy;
					} else {
						this->getLogManager() << LogLevel_Error << "Bailing out (from xval)...\n";
						return false;
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

				printConfusionMatrix(l_oConfusion);
			} 
			else
			{
				this->getLogManager() << LogLevel_Info << "Training without cross-validation.\n";
				this->getLogManager() << LogLevel_Info << "*** Reported training set accuracy will be optimistic ***\n";
			}

			this->getLogManager() << LogLevel_Trace << "Training final classifier on the whole set...\n";
			if(!this->train(0, 0))
			{
				this->getLogManager() << LogLevel_Error << "Bailing out (from whole set training)...\n";
				return false;
			}

			OpenViBEToolkit::Tools::Matrix::clearContent(l_oConfusion);

			this->getLogManager() << LogLevel_Info << "Training set accuracy is " << this->getAccuracy(0, m_vFeatureVector.size(), l_oConfusion) << "% (optimistic)\n";

			printConfusionMatrix(l_oConfusion);

			if(!this->saveConfiguration())
				return false;
		}
	}

	return true;
}

boolean CBoxAlgorithmClassifierTrainer::train(const size_t uiStartIndex, const size_t uiStopIndex)
{
	if(uiStopIndex-uiStartIndex==1)
	{
		this->getLogManager() << LogLevel_Error << "stopIndex-trainIndex=1\n";
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

	if(m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Train) == false)
	{
		this->getLogManager() << LogLevel_Error << "Training failed.\n";
		return false;
	}

	TParameterHandler < XML::IXMLNode* > op_pConfiguration(m_pClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	XML::IXMLNode *l_pTempNode = (XML::IXMLNode*)op_pConfiguration;

	if(l_pTempNode != NULL){
		l_pTempNode->release();
	}
	op_pConfiguration = NULL;

	bool l_bReturnValue = m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_SaveConfiguration);

	return l_bReturnValue;
}

float64 CBoxAlgorithmClassifierTrainer::getAccuracy(const size_t uiStartIndex, const size_t uiStopIndex, CMatrix& oConfusionMatrix)
{
	size_t l_iSuccessfullTrainerCount=0;

	if(uiStopIndex-uiStartIndex==0)
	{
		this->getLogManager() << LogLevel_Error << "Start and stop indexes are the same (" << static_cast<uint64>(uiStartIndex) << ")\n";
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
		const size_t k = m_vFeatureVectorIndex[j];

		float64* l_pFeatureVectorBuffer=ip_pFeatureVector->getBuffer();
		const float64 l_f64CorrectValue=(float64)m_vFeatureVector[k].m_ui32InputIndex;
		System::Memory::copy(
			l_pFeatureVectorBuffer,
			m_vFeatureVector[k].m_pFeatureVectorMatrix->getBuffer(),
			l_ui32FeatureVectorSize*sizeof(float64));

		m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Classify);

		const float64 l_f64PredictedValue = op_f64ClassificationStateClass;
		//std::cout << "Try to recognize " << l_f64CorrectValue << " and get " << l_f64PredictedValue << std::endl;
		if(l_f64PredictedValue==l_f64CorrectValue)
		{
			l_iSuccessfullTrainerCount++;
		}

		oConfusionMatrix[static_cast<uint32>(l_f64CorrectValue-1)*oConfusionMatrix.getDimensionSize(1) + 
			static_cast<uint32>(l_f64PredictedValue-1)] += 1.0;
	}

	return static_cast<float64>((l_iSuccessfullTrainerCount*100.0)/(uiStopIndex-uiStartIndex));
}

boolean CBoxAlgorithmClassifierTrainer::printConfusionMatrix(const CMatrix& oMatrix)
{
	if(oMatrix.getDimensionCount() != 2 ||
		oMatrix.getDimensionSize(0) != oMatrix.getDimensionSize(1))
	{
		this->getLogManager() << LogLevel_Warning << "Confusion matrix has unusual size, not printing\n";
		return false;
	}

	const uint32 l_ui32Rows = oMatrix.getDimensionSize(0);

	if(l_ui32Rows>10 && !this->getConfigurationManager().expandAsBoolean("${Plugin_Classification_ForceConfusionMatrixPrint}"))
	{
		this->getLogManager() << LogLevel_Info << "Over 10 classes, not printing the confusion matrix. If needed, override with setting Plugin_Classification_ForceConfusionMatrixPrint token to true.\n";
		return true;
	}

	// Normalize
	CMatrix l_oTmp,l_oRowSum;
	OpenViBEToolkit::Tools::Matrix::copy(l_oTmp, oMatrix);
	l_oRowSum.setDimensionCount(1);
	l_oRowSum.setDimensionSize(0,l_ui32Rows);
	OpenViBEToolkit::Tools::Matrix::clearContent(l_oRowSum);

	for(uint32 i=0;i<l_ui32Rows;i++) {
		float64 l_f64RowSum = 0;
		for(uint32 j=0;j<l_ui32Rows;j++) {
			l_oRowSum[i] += l_oTmp[i*l_ui32Rows+j];
		}
		for(uint32 j=0;j<l_ui32Rows;j++) {
			l_oTmp[i*l_ui32Rows+j] /= l_oRowSum[i];
		}
	}

	std::stringstream ss;
	ss << std::fixed;

	ss << "  Cls vs cls ";
	for(uint32 i=0;i<l_ui32Rows;i++) {
		ss << setw(6) << (i+1);
	}
	this->getLogManager() << LogLevel_Info << ss.str().c_str()<< "\n";

	ss.precision(1);
	for(uint32 i=0;i<l_ui32Rows;i++) {
		ss.str("") ; 
		ss << "  Target " << setw(2) << (i+1) << ": ";
		for(uint32 j=0;j<l_ui32Rows;j++) {
			ss << setw(6) << l_oTmp[i*l_ui32Rows+j]*100;
		}
		this->getLogManager() << LogLevel_Info << ss.str().c_str() << " %, " << static_cast<uint32>(l_oRowSum[i]) << " examples\n";
	}

	return true;
}

boolean CBoxAlgorithmClassifierTrainer::saveConfiguration(void)
{
	CIdentifier l_oStrategyClassIdentifier, l_oClassifierAlgorithmClassIdentifier;
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	TParameterHandler < XML::IXMLNode* > op_pConfiguration(m_pClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	XML::IXMLNode* l_pAlgorithmConfigurationNode = XML::createNode(c_sClassifierRoot);
	l_pAlgorithmConfigurationNode->addChild((XML::IXMLNode*)op_pConfiguration);

	XML::IXMLHandler *l_pHandler = XML::createXMLHandler();
	CString l_sConfigurationFilename(this->getConfigurationManager().expand((*m_pParameter)[c_sFilenameSettingName]));

	CString l_sClassifierAlgorithmClassIdentifier, l_sStrategyClassIdentifier;
	l_rStaticBoxContext.getSettingValue(0, l_sStrategyClassIdentifier);
	l_rStaticBoxContext.getSettingValue(1, l_sClassifierAlgorithmClassIdentifier);

	XML::IXMLNode *l_sRoot = XML::createNode(c_sClassificationBoxRoot);
	std::stringstream l_sVersion;
	l_sVersion << OVP_Classification_BoxTrainerXMLVersion;
	l_sRoot->addAttribute(c_sXmlVersionAttributeName, l_sVersion.str().c_str());


	XML::IXMLNode *l_pTempNode = XML::createNode(c_sStrategyNodeName);
	l_oStrategyClassIdentifier = this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_ClassificationStrategy, (*m_pParameter)[c_sMulticlassStrategySettingName]);
	l_pTempNode->addAttribute(c_sIdentifierAttributeName, l_oStrategyClassIdentifier.toString());
	l_pTempNode->setPCData((*m_pParameter)[c_sMulticlassStrategySettingName].toASCIIString());
	l_sRoot->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sAlgorithmNodeName);
	l_oClassifierAlgorithmClassIdentifier = this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_ClassificationAlgorithm, (*m_pParameter)[c_sAlgorithmSettingName]);
	l_pTempNode->addAttribute(c_sIdentifierAttributeName, l_oClassifierAlgorithmClassIdentifier.toString());
	l_pTempNode->setPCData((*m_pParameter)[c_sAlgorithmSettingName].toASCIIString());
	l_sRoot->addChild(l_pTempNode);


	XML::IXMLNode *l_pStimulationsNode = XML::createNode(c_sStimulationsNodeName);

	for(OpenViBE::uint32 i =1 ; i < l_rStaticBoxContext.getInputCount() ; ++i)
	{
		char l_sBufferSettingName[64];
		sprintf(l_sBufferSettingName, "Class %d label", i);

		l_pTempNode = XML::createNode(c_sClassStimulationNodeName);
		std::stringstream l_sBuffer;
		l_sBuffer << i;
		l_pTempNode->addAttribute(c_sIdentifierAttributeName, l_sBuffer.str().c_str());
		l_pTempNode->setPCData((*m_pParameter)[l_sBufferSettingName].toASCIIString());
		l_pStimulationsNode->addChild(l_pTempNode);
	}
	l_sRoot->addChild(l_pStimulationsNode);

	l_sRoot->addChild(l_pAlgorithmConfigurationNode);
	//std::cout << l_sRoot->getXML() << std::endl;
	if(!l_pHandler->writeXMLInFile(*l_sRoot, l_sConfigurationFilename.toASCIIString()))
	{
		this->getLogManager() << LogLevel_Error << "Could not save configuration to file [" << l_sConfigurationFilename << "]\n";
		return false;
	}


	l_pHandler->release();
	l_sRoot->release();
	op_pConfiguration=NULL;
	return true;
}
