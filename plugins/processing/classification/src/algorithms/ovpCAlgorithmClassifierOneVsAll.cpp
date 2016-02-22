#include "ovpCAlgorithmClassifierOneVsAll.h"

#include <map>
#include <sstream>
#include <cstring>
#include <string>
#include <utility>
#include <iostream>
#include <system/ovCMemory.h>

namespace{
	const char* const c_sTypeNodeName = "OneVsAll";
	const char* const c_sSubClassifierIdentifierNodeName = "SubClassifierIdentifier";
	const char* const c_sAlgorithmIdAttribute = "algorithm-id";
	const char* const c_sSubClassifierCountNodeName = "SubClassifierCount";
	const char* const c_sSubClassifiersNodeName = "SubClassifiers";
	const char* const c_sSubClassifierNodeName = "SubClassifier";
}

extern const char* const c_sClassifierRoot;

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

typedef std::pair < IMatrix*, IMatrix* > CIMatrixPointerPair;
typedef std::pair< float64, IMatrix* > CClassifierOutput;

boolean CAlgorithmClassifierOneVsAll::initialize()
{
	TParameterHandler < XML::IXMLNode* > op_pConfiguration(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	op_pConfiguration=NULL;

	return CAlgorithmPairingStrategy::initialize();
}

boolean CAlgorithmClassifierOneVsAll::uninitialize(void)
{
	while(!m_oSubClassifierList.empty())
	{
		this->removeClassifierAtBack();
	}
	return CAlgorithmPairingStrategy::uninitialize();
}



boolean CAlgorithmClassifierOneVsAll::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	const uint32 l_ui32ClassCount = m_oSubClassifierList.size();
	std::map < float64, size_t > l_vClassLabels;

	for(size_t i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		if(!l_vClassLabels.count(rFeatureVectorSet[i].getLabel()))
		{
			l_vClassLabels[rFeatureVectorSet[i].getLabel()] = 0;
		}
		l_vClassLabels[rFeatureVectorSet[i].getLabel()]++;
	}

	if(l_vClassLabels.size() != l_ui32ClassCount)
	{
		this->getLogManager() << LogLevel_Error << "There are samples for " << (uint32)l_vClassLabels.size() << " classes but expected samples for " << l_ui32ClassCount << " classes.\n";
		return false;
	}

	//We set the IMatrix fo the first classifier
	const uint32 l_ui32FeatureVectorSize=rFeatureVectorSet[0].getSize();
	TParameterHandler < IMatrix* > ip_pFeatureVectorSetReference(m_oSubClassifierList[0]->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet));
	ip_pFeatureVectorSetReference->setDimensionCount(2);
	ip_pFeatureVectorSetReference->setDimensionSize(0, rFeatureVectorSet.getFeatureVectorCount());
	ip_pFeatureVectorSetReference->setDimensionSize(1, l_ui32FeatureVectorSize+1);

	float64* l_pFeatureVectorSetBuffer=ip_pFeatureVectorSetReference->getBuffer();
	for(size_t j=0; j<rFeatureVectorSet.getFeatureVectorCount(); j++)
	{
		System::Memory::copy(
					l_pFeatureVectorSetBuffer,
					rFeatureVectorSet[j].getBuffer(),
					l_ui32FeatureVectorSize*sizeof(float64));
		//We let the space for the label
		l_pFeatureVectorSetBuffer+=(l_ui32FeatureVectorSize+1);
	}

	//And then we just change adapt the label for each feature vector but we don't copy them anymore
	for(size_t l_iClassifierCounter = 1 ; l_iClassifierCounter <= m_oSubClassifierList.size() ; ++l_iClassifierCounter )
	{
		TParameterHandler < IMatrix* > ip_pFeatureVectorSet(m_oSubClassifierList[l_iClassifierCounter-1]->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet));
		ip_pFeatureVectorSet = (IMatrix*)ip_pFeatureVectorSetReference;

		float64* l_pFeatureVectorSetBuffer=ip_pFeatureVectorSet->getBuffer();
		for(size_t j=0; j<rFeatureVectorSet.getFeatureVectorCount(); j++)
		{
			//Modify the class of each featureVector
			const float64 l_f64Class = rFeatureVectorSet[j].getLabel();
			if(static_cast<uint32>(l_f64Class) == l_iClassifierCounter)
			{
				l_pFeatureVectorSetBuffer[l_ui32FeatureVectorSize]=1;
			}
			else
			{
				l_pFeatureVectorSetBuffer[l_ui32FeatureVectorSize]=2;
			}
			l_pFeatureVectorSetBuffer+=(l_ui32FeatureVectorSize+1);
		}

		m_oSubClassifierList[l_iClassifierCounter-1]->process(OVTK_Algorithm_Classifier_InputTriggerId_Train);
	}
	return true;
}

boolean CAlgorithmClassifierOneVsAll::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues, IVector& rProbabilityValue)
{
	std::vector< CClassifierOutput > l_oClassificationVector;

	const uint32 l_ui32FeatureVectorSize=rFeatureVector.getSize();

	for(size_t l_iClassifierCounter = 0 ; l_iClassifierCounter < m_oSubClassifierList.size() ; ++l_iClassifierCounter )
	{
		IAlgorithmProxy* l_pSubClassifier = this->m_oSubClassifierList[l_iClassifierCounter];
		TParameterHandler < IMatrix* > ip_pFeatureVector(l_pSubClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVector));
		TParameterHandler < float64 > op_f64ClassificationStateClass(l_pSubClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Class));
		TParameterHandler < IMatrix* > op_pClassificationValues(l_pSubClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
		TParameterHandler < IMatrix* > op_pProbabilityValues(l_pSubClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ProbabilityValues));
		ip_pFeatureVector->setDimensionCount(1);
		ip_pFeatureVector->setDimensionSize(0, l_ui32FeatureVectorSize);

		float64* l_pFeatureVectorBuffer=ip_pFeatureVector->getBuffer();
		System::Memory::copy(
					l_pFeatureVectorBuffer,
					rFeatureVector.getBuffer(),
					l_ui32FeatureVectorSize*sizeof(float64));
		l_pSubClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Classify);

		IMatrix* l_pProbabilityValue = static_cast<IMatrix*>(op_pProbabilityValues);
		//If the algorithm give a probability we take it, instead we take the first value
		if(l_pProbabilityValue->getDimensionCount() != 0)
		{
			l_oClassificationVector.push_back(CClassifierOutput(static_cast<float64>(op_f64ClassificationStateClass), l_pProbabilityValue));
		}
		else
		{
			l_oClassificationVector.push_back(CClassifierOutput(static_cast<float64>(op_f64ClassificationStateClass), static_cast<IMatrix*>(op_pClassificationValues)));
		}
		this->getLogManager() << LogLevel_Debug << static_cast<int64>(l_iClassifierCounter) << " " << (float64)op_f64ClassificationStateClass << " " << (*op_pProbabilityValues)[0] << " " << (*op_pProbabilityValues)[1] << "\n";
	}

	//Now, we determine the best classification
	CClassifierOutput l_oBest = CClassifierOutput(-1.0, static_cast<IMatrix*>(NULL));
	rf64Class = -1;

	for(size_t l_iClassificationCount = 0; l_iClassificationCount < l_oClassificationVector.size() ; ++l_iClassificationCount)
	{
		CClassifierOutput&   l_pTemp = l_oClassificationVector[l_iClassificationCount];
		if(l_pTemp.first==1)
		{
			if(l_oBest.second == NULL)
			{
				l_oBest = l_pTemp;
				rf64Class = l_iClassificationCount+1;
			}
			else
			{
				if((*m_fAlgorithmComparison)((*l_oBest.second), *(l_pTemp.second)) > 0)
				{
					l_oBest = l_pTemp;
					rf64Class = l_iClassificationCount+1;
				}
			}
		}
	}

	//If no one recognize the class, let's take the more relevant
	if(rf64Class == -1)
	{
		this->getLogManager() << LogLevel_Debug << "Unable to find a class in first instance\n";
		for(uint32 l_iClassificationCount = 0; l_iClassificationCount < l_oClassificationVector.size() ; ++l_iClassificationCount)
		{
			CClassifierOutput& l_pTemp = l_oClassificationVector[l_iClassificationCount];
			if(l_oBest.second == NULL)
			{
				l_oBest = l_pTemp;
				rf64Class = (static_cast<float64>(l_iClassificationCount))+1;
			}
			else
			{
				//We take the one that is the least like the second class
				if((*m_fAlgorithmComparison)((*l_oBest.second), *(l_pTemp.second)) < 0)
				{
					l_oBest = l_pTemp;
					rf64Class = l_iClassificationCount+1;
				}
			}
		}
	}

	//If we still don't have a better classification, we face an error
	if(l_oBest.second == NULL)
	{
		return false;
	}

	//Now that we made the calculation, we send the corresponding data
	IAlgorithmProxy* l_pWinner = this->m_oSubClassifierList[static_cast<uint32>(rf64Class)-1];
	TParameterHandler < IMatrix* > op_pClassificationWinnerValues(l_pWinner->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
	TParameterHandler < IMatrix* > op_pProbabilityWinnerValues(l_pWinner->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ProbabilityValues));

	IMatrix* l_pTempMatrix = static_cast<IMatrix*>(op_pClassificationWinnerValues);
	rClassificationValues.setSize(l_pTempMatrix->getBufferElementCount());
	System::Memory::copy(rClassificationValues.getBuffer(), l_pTempMatrix->getBuffer(), l_pTempMatrix->getBufferElementCount()*sizeof(float64));

	l_pTempMatrix = static_cast<IMatrix*>(op_pProbabilityWinnerValues);
	rProbabilityValue.setSize(op_pProbabilityWinnerValues->getBufferElementCount());
	System::Memory::copy(rProbabilityValue.getBuffer(), l_pTempMatrix->getBuffer(), l_pTempMatrix->getBufferElementCount()*sizeof(float64));

	return true;
}

boolean CAlgorithmClassifierOneVsAll::addNewClassifierAtBack(void)
{
	const CIdentifier l_oSubClassifierAlgorithm = this->getAlgorithmManager().createAlgorithm(this->m_oSubClassifierAlgorithmIdentifier);
	if(l_oSubClassifierAlgorithm == OV_UndefinedIdentifier)
	{
		this->getLogManager() << LogLevel_Error << "Unable to instantiate classifier for class " << this->m_oSubClassifierAlgorithmIdentifier << "\n. Is the classifier still available in OpenViBE?";
		return false;
	}
	IAlgorithmProxy* l_pSubClassifier = &this->getAlgorithmManager().getAlgorithm(l_oSubClassifierAlgorithm);
	l_pSubClassifier->initialize();

	//Set a references to the extra parameters input of the pairing strategy
	TParameterHandler< std::map<CString, CString>* > ip_pExtraParameters(l_pSubClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));
	ip_pExtraParameters.setReferenceTarget(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));

	this->m_oSubClassifierList.push_back(l_pSubClassifier);

	return true;
}

void CAlgorithmClassifierOneVsAll::removeClassifierAtBack(void)
{
	IAlgorithmProxy* l_pSubClassifier = m_oSubClassifierList.back();
	l_pSubClassifier->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*l_pSubClassifier);
	this->m_oSubClassifierList.pop_back();
}

boolean CAlgorithmClassifierOneVsAll::designArchitecture(const OpenViBE::CIdentifier& rId, OpenViBE::uint32 rClassCount)
{
	if(!this->setSubClassifierIdentifier(rId))
	{
		return false;
	}
	for(size_t i = 0 ; i < rClassCount ; ++i)
	{
		if(!this->addNewClassifierAtBack())
		{
			return false;
		}
	}
	return true;
}

XML::IXMLNode* CAlgorithmClassifierOneVsAll::getClassifierConfiguration(IAlgorithmProxy* rClassifier)
{
	TParameterHandler < XML::IXMLNode* > op_pConfiguration(rClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	rClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_SaveConfiguration);
	XML::IXMLNode * l_pRes = op_pConfiguration;
	return l_pRes;
}

XML::IXMLNode* CAlgorithmClassifierOneVsAll::saveConfiguration(void)
{
	std::stringstream l_sClassCountes;
	l_sClassCountes << getClassCount();

	XML::IXMLNode *l_pOneVsAllNode = XML::createNode(c_sTypeNodeName);

	XML::IXMLNode *l_pTempNode = XML::createNode(c_sSubClassifierIdentifierNodeName);
	l_pTempNode->addAttribute(c_sAlgorithmIdAttribute,this->m_oSubClassifierAlgorithmIdentifier.toString());
	l_pTempNode->setPCData(this->getTypeManager().getEnumerationEntryNameFromValue(OVTK_TypeId_ClassificationAlgorithm, m_oSubClassifierAlgorithmIdentifier.toUInteger()).toASCIIString());
	l_pOneVsAllNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sSubClassifierCountNodeName);
	l_pTempNode->setPCData(l_sClassCountes.str().c_str());
	l_pOneVsAllNode->addChild(l_pTempNode);

	XML::IXMLNode *l_pSubClassifersNode = XML::createNode(c_sSubClassifiersNodeName);

	//We now add configuration of each subclassifiers
	for(size_t i = 0; i<m_oSubClassifierList.size(); ++i)
	{
		l_pSubClassifersNode->addChild(getClassifierConfiguration(m_oSubClassifierList[i]));
	}
	l_pOneVsAllNode->addChild(l_pSubClassifersNode);

	return l_pOneVsAllNode;
}

boolean CAlgorithmClassifierOneVsAll::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	XML::IXMLNode *l_pTempNode = pConfigurationNode->getChildByName(c_sSubClassifierIdentifierNodeName);
	CIdentifier l_oIdentifier;
	l_oIdentifier.fromString(l_pTempNode->getAttribute(c_sAlgorithmIdAttribute));
	if(m_oSubClassifierAlgorithmIdentifier.toUInteger() != l_oIdentifier)
	{
		while(!m_oSubClassifierList.empty())
		{
			this->removeClassifierAtBack();
		}
		if(!this->setSubClassifierIdentifier(l_oIdentifier)){
			//if the sub classifier doesn't have comparison function it is an error
			return false;
		}
	}

	l_pTempNode = pConfigurationNode->getChildByName(c_sSubClassifierCountNodeName);
	std::stringstream l_sCountData(l_pTempNode->getPCData());
	uint64 l_iClassCount;
	l_sCountData >> l_iClassCount;

	while(l_iClassCount != getClassCount())
	{
		if(l_iClassCount < getClassCount())
		{
			this->removeClassifierAtBack();
		}
		else
		{
			if(!this->addNewClassifierAtBack())
			{
				return false;
			}
		}
	}

	return loadSubClassifierConfiguration(pConfigurationNode->getChildByName(c_sSubClassifiersNodeName));
}

uint32 CAlgorithmClassifierOneVsAll::getOutputProbabilityVectorLength()
{
	TParameterHandler < IMatrix* > op_pProbabilityValues(m_oSubClassifierList[0]->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ProbabilityValues));
	return op_pProbabilityValues->getDimensionSize(0);
}

uint32 CAlgorithmClassifierOneVsAll::getOutputDistanceVectorLength()
{
	TParameterHandler < IMatrix* > op_pDistanceValues(m_oSubClassifierList[0]->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
	return op_pDistanceValues->getDimensionSize(0);
}

boolean CAlgorithmClassifierOneVsAll::loadSubClassifierConfiguration(XML::IXMLNode *pSubClassifiersNode)
{
	for( size_t i = 0; i < pSubClassifiersNode->getChildCount() ; ++i)
	{
		XML::IXMLNode *l_pSubClassifierNode = pSubClassifiersNode->getChild(i);
		TParameterHandler < XML::IXMLNode* > ip_pConfiguration(m_oSubClassifierList[i]->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_Configuration));
		ip_pConfiguration = l_pSubClassifierNode;
		if(!m_oSubClassifierList[i]->process(OVTK_Algorithm_Classifier_InputTriggerId_LoadConfiguration))
		{
			this->getLogManager() << LogLevel_Error << "Unable to load the configuration of the classifier " << static_cast<uint64>(i+1) << " \n";
			return false;
		}
	}
	return true;
}

uint32 CAlgorithmClassifierOneVsAll::getClassCount(void) const
{
	return m_oSubClassifierList.size();
}

boolean CAlgorithmClassifierOneVsAll::setSubClassifierIdentifier(const OpenViBE::CIdentifier &rId)
{
	m_oSubClassifierAlgorithmIdentifier = rId;
	m_fAlgorithmComparison = getClassificationComparisonFunction(rId);

	if(m_fAlgorithmComparison == NULL)
	{
		this->getLogManager() << LogLevel_Error << "Cannot find the comparison function for the sub classifier\n";
		return false;
	}
	return true;
}

