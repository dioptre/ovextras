#include "ovpCBoxAlgorithmClassifierProcessor.h"

#include <sstream>

#include <xml/IXMLHandler.h>
#include <xml/IXMLNode.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;
using namespace std;

boolean CBoxAlgorithmClassifierProcessor::initialize(void)
{
	m_pClassifier = NULL;

	m_oFeatureVectorDecoder.initialize(*this, 0);

	m_oLabelsEncoder.initialize(*this, 0);
	m_oHyperplanValuesEncoder.initialize(*this, 1);
	m_oProbabilitiesValuesEncoder.initialize(*this, 2);


	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	//First of all, let's get the XML file for configuration
	CString l_sConfigurationFilename;
	l_rStaticBoxContext.getSettingValue(0, l_sConfigurationFilename);

	if(l_sConfigurationFilename == CString("")) 
	{
		this->getLogManager() << LogLevel_Error << "You need to specify a classifier .xml for the box (use Classifier Trainer to create one)\n";
		return false;
	}

	XML::IXMLHandler *l_pHandler = XML::createXMLHandler();
	XML::IXMLNode *l_pRootNode = l_pHandler->parseFile(l_sConfigurationFilename.toASCIIString());

	if(!l_pRootNode) 
	{
		this->getLogManager() << LogLevel_Error << "Unable to get root node from [" << l_sConfigurationFilename << "]\n";
		return false;
	}

	//Now check the version, and let's display a message if the version is not good
	string l_sVersion;
	if(l_pRootNode->hasAttribute(c_sXmlVersionAttributeName))
	{
		l_sVersion = l_pRootNode->getAttribute(c_sXmlVersionAttributeName);
		std::stringstream l_sData(l_sVersion);
		uint32 l_ui32Version;
		l_sData >> l_ui32Version;
		if(l_ui32Version != OVP_Classification_BoxTrainerXMLVersion)
		{
			this->getLogManager() << LogLevel_Warning << "The configuration file doesn't have the same version number as the box. Trouble may appear in loading process.\n";
		}
	}
	else
	{
		this->getLogManager() << LogLevel_Warning << "The configuration file has no version information. Trouble may appear in loading process.\n";
	}

	CIdentifier l_oAlgorithmClassIdentifier = OV_UndefinedIdentifier;

	XML::IXMLNode * l_pTempNode = l_pRootNode->getChildByName(c_sStrategyNodeName);
	if(l_pTempNode) {
		l_oAlgorithmClassIdentifier.fromString(l_pTempNode->getAttribute(c_sIdentifierAttributeName));
	} else {
		this->getLogManager() << LogLevel_Warning << "The configuration file had no node [" << c_sStrategyNodeName << "]. Trouble may appear later.\n";
	}

	//If the Identifier is undefined, that means we need to load a native algorithm
	if(l_oAlgorithmClassIdentifier == OV_UndefinedIdentifier){
		this->getLogManager() << LogLevel_Trace << "Using Native algorithm\n";
		l_pTempNode = l_pRootNode->getChildByName(c_sAlgorithmNodeName);
		if(l_pTempNode)
		{
			l_oAlgorithmClassIdentifier.fromString(l_pTempNode->getAttribute(c_sIdentifierAttributeName));
		}
		else
		{
			this->getLogManager() << LogLevel_Warning << "The configuration file had no node [" << c_sAlgorithmNodeName << "]. Trouble may appear later.\n";
		}

		//If the algorithm is still unknown, that means that we face an error
		if(l_oAlgorithmClassIdentifier==OV_UndefinedIdentifier)
		{
			this->getLogManager() << LogLevel_Error << "Couldn't restore a classifier from the file [" << l_sConfigurationFilename << "].\n";
			return false;
		}
	}

	//Now loading all stimulations output
	XML::IXMLNode *l_pStimulationsNode = l_pRootNode->getChildByName(c_sStimulationsNodeName);
	if(l_pStimulationsNode)
	{
		//Now load every stimulation and store them in the map with the right class id
		for(uint32 i=0; i < l_pStimulationsNode->getChildCount(); i++)
		{
			l_pTempNode = l_pStimulationsNode->getChild(i);
			if(!l_pTempNode)
			{
				this->getLogManager() << LogLevel_Error << "Expected child node " << i << " for node [" << c_sStimulationsNodeName << "]. Output labels not known. Aborting.\n";
				return false;
			}
			CString l_sStimulationName(l_pTempNode->getPCData());

			OpenViBE::float64 l_f64ClassId;
			const char *l_sAttributeData = l_pTempNode->getAttribute(c_sIdentifierAttributeName);
			if(!l_sAttributeData) 
			{
				this->getLogManager() << LogLevel_Error << "Expected child node " << i << " for node [" << c_sStimulationsNodeName << "] to have attribute [" << c_sIdentifierAttributeName <<  "]. Aborting.\n";
				return false;
			}

			std::stringstream l_sIdentifierData(l_sAttributeData);
			l_sIdentifierData >> l_f64ClassId ;
			m_vStimulation[l_f64ClassId]=this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, l_sStimulationName);
		}
	}
	else
	{
		this->getLogManager() << LogLevel_Warning << "The configuration file had no node " << c_sStimulationsNodeName << ". Trouble may appear later.\n";
	}

	const CIdentifier l_oClassifierAlgorithmIdentifier = this->getAlgorithmManager().createAlgorithm(l_oAlgorithmClassIdentifier);
	if(l_oClassifierAlgorithmIdentifier == OV_UndefinedIdentifier)
	{
		this->getLogManager() << LogLevel_Error << "Error instantiating classifier class with id " 
			<< l_oAlgorithmClassIdentifier
			<< ". If you've loaded an old scenario or configuration file(s), make sure that the classifiers specified in it are still available.\n";
		return false;
	}
	m_pClassifier=&this->getAlgorithmManager().getAlgorithm(l_oClassifierAlgorithmIdentifier);
	m_pClassifier->initialize();

	TParameterHandler < IMatrix* > ip_pClassifierStimulationSet(m_pClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVector));
	ip_pClassifierStimulationSet.setReferenceTarget(m_oFeatureVectorDecoder.getOutputMatrix());

	m_oHyperplanValuesEncoder.getInputMatrix().setReferenceTarget(m_pClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
	m_oProbabilitiesValuesEncoder.getInputMatrix().setReferenceTarget(m_pClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ProbabilityValues));

	TParameterHandler < XML::IXMLNode* > ip_pClassificationConfiguration(m_pClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_Configuration));
	ip_pClassificationConfiguration = l_pRootNode->getChildByName(c_sClassifierRoot)->getChild(0);
	if(!m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_LoadConfiguration)){
		this->getLogManager() << LogLevel_Error << "Subclassifier failed to load config\n";
		return false;
	}

	l_pRootNode->release();
	l_pHandler->release();

	return true;
}

boolean CBoxAlgorithmClassifierProcessor::uninitialize(void)
{
	m_oFeatureVectorDecoder.uninitialize();
	m_oLabelsEncoder.uninitialize();
	m_oHyperplanValuesEncoder.uninitialize();
	m_oProbabilitiesValuesEncoder.uninitialize();

	if(m_pClassifier)
	{
		m_pClassifier->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pClassifier);
		m_pClassifier = NULL;
	}

	return true;
}

boolean CBoxAlgorithmClassifierProcessor::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmClassifierProcessor::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		uint64 l_ui64StartTime=l_rDynamicBoxContext.getInputChunkStartTime(0, i);
		uint64 l_ui64EndTime=l_rDynamicBoxContext.getInputChunkEndTime(0, i);

		TParameterHandler < IStimulationSet* > &ip_pLabelsStimulationSet = m_oLabelsEncoder.getInputStimulationSet();
		TParameterHandler < float64 > op_f64ClassificationStateClass(m_pClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Class));

		m_oFeatureVectorDecoder.decode(i);

		if(m_oFeatureVectorDecoder.isHeaderReceived())
		{
			m_oLabelsEncoder.encodeHeader();
			m_oHyperplanValuesEncoder.encodeHeader();
			m_oProbabilitiesValuesEncoder.encodeHeader();

			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);
			l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_ui64StartTime, l_ui64EndTime);
			l_rDynamicBoxContext.markOutputAsReadyToSend(2, l_ui64StartTime, l_ui64EndTime);
		}
		if(m_oFeatureVectorDecoder.isBufferReceived())
		{
			if(m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Classify))
			{
				if (m_pClassifier->isOutputTriggerActive(OVTK_Algorithm_Classifier_OutputTriggerId_Success))
				{
					//this->getLogManager() << LogLevel_Warning << "---Classification successful---\n";
					ip_pLabelsStimulationSet->setStimulationCount(1);
					ip_pLabelsStimulationSet->setStimulationIdentifier(0, m_vStimulation[op_f64ClassificationStateClass]);
					ip_pLabelsStimulationSet->setStimulationDate(0, l_ui64EndTime);
					ip_pLabelsStimulationSet->setStimulationDuration(0, 0);

					m_oLabelsEncoder.encodeBuffer();
					m_oHyperplanValuesEncoder.encodeBuffer();
					m_oProbabilitiesValuesEncoder.encodeBuffer();

					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);
					l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_ui64StartTime, l_ui64EndTime);
					l_rDynamicBoxContext.markOutputAsReadyToSend(2, l_ui64StartTime, l_ui64EndTime);
				}
				else
				{
					this->getLogManager() << LogLevel_Error << "Classification failed (success trigger not active).\n";
					return false;
				}
			}
			else
			{
				this->getLogManager() << LogLevel_Error << "Classification algorithm failed.\n";
				return false;
			}
		}
		if(m_oFeatureVectorDecoder.isEndReceived())
		{
			m_oLabelsEncoder.encodeEnd();
			m_oHyperplanValuesEncoder.encodeEnd();
			m_oProbabilitiesValuesEncoder.encodeEnd();

			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);
			l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_ui64StartTime, l_ui64EndTime);
			l_rDynamicBoxContext.markOutputAsReadyToSend(2, l_ui64StartTime, l_ui64EndTime);
		}

		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}

	return true;
}
