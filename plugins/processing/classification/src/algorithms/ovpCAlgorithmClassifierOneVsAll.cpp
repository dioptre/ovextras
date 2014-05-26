#include "ovpCAlgorithmClassifierOneVsAll.h"

#include <map>
#include <sstream>
#include <cstring>
#include <string>
#include <utility>
#include <iostream>
#include <system/Memory.h>


static const char* const c_sTypeNodeName = "OneVsAll";
static const char* const c_sSubClassifierIdentifierNodeName = "SubClassifierIdentifier";
static const char* const c_sAlgorithmIdAttribute = "algorithm-id";
static const char* const c_sSubClassifierCountNodeName = "SubClassifierCount";
static const char* const c_sSubClassifiersNodeName = "SubClassifiers";
static const char* const c_sSubClassifierNodeName = "SubClassifier";

extern const char* const c_sClassifierRoot;

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

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
	return true;
}



boolean CAlgorithmClassifierOneVsAll::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	const uint32 l_ui32AmountClass = m_oSubClassifierList.size();
	std::map < float64, size_t > l_vClassLabels;
	for(uint32 i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		if(!l_vClassLabels.count(rFeatureVectorSet[i].getLabel()))
		{
			l_vClassLabels[rFeatureVectorSet[i].getLabel()] = 0;
		}
		l_vClassLabels[rFeatureVectorSet[i].getLabel()]++;
	}

	if(l_vClassLabels.size() != l_ui32AmountClass)
	{
		this->getLogManager() << LogLevel_Error << "There is sample for " << (uint32)l_vClassLabels.size() << " classes but expected for " << l_ui32AmountClass << ".\n";
		return false;
	}

	//We send new set of data to each classifer. They will all use two different classes 1 and 2. 1 is for the class it should recognize, 2 for the others
	for(uint32 l_iClassifierCounter = 1 ; l_iClassifierCounter <= m_oSubClassifierList.size() ; ++l_iClassifierCounter )
	{
		uint32 l_ui32FeatureVectorSize=rFeatureVectorSet[0].getSize();

		TParameterHandler < IMatrix* > ip_pFeatureVectorSet(m_oSubClassifierList[l_iClassifierCounter-1]->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet));
		ip_pFeatureVectorSet->setDimensionCount(2);
		ip_pFeatureVectorSet->setDimensionSize(0, rFeatureVectorSet.getFeatureVectorCount());
		ip_pFeatureVectorSet->setDimensionSize(1, l_ui32FeatureVectorSize+1);

		float64* l_pFeatureVectorSetBuffer=ip_pFeatureVectorSet->getBuffer();
		for(size_t j=0; j<rFeatureVectorSet.getFeatureVectorCount(); j++)
		{
			System::Memory::copy(
						l_pFeatureVectorSetBuffer,
						rFeatureVectorSet[j].getBuffer(),
						l_ui32FeatureVectorSize*sizeof(float64));

			//Modify the class of each featureVector
			float64 l_f64Class = rFeatureVectorSet[j].getLabel();
			if((uint32)l_f64Class == l_iClassifierCounter)
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

boolean CAlgorithmClassifierOneVsAll::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
{
	std::vector< std::pair < float64, IMatrix*> > l_oClassificationVector;

	uint32 l_ui32FeatureVectorSize=rFeatureVector.getSize();

	for(uint32 l_iClassifierCounter = 0 ; l_iClassifierCounter < m_oSubClassifierList.size() ; ++l_iClassifierCounter )
	{
		IAlgorithmProxy* l_pSubClassifier = this->m_oSubClassifierList[l_iClassifierCounter];
		TParameterHandler < IMatrix* > ip_pFeatureVector(l_pSubClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVector));
		TParameterHandler < float64 > op_f64ClassificationStateClass(l_pSubClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Class));
		TParameterHandler < IMatrix* > op_pClassificationValues(l_pSubClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
		ip_pFeatureVector->setDimensionCount(1);
		ip_pFeatureVector->setDimensionSize(0, l_ui32FeatureVectorSize);

		float64* l_pFeatureVectorBuffer=ip_pFeatureVector->getBuffer();
		System::Memory::copy(
					l_pFeatureVectorBuffer,
					rFeatureVector.getBuffer(),
					l_ui32FeatureVectorSize*sizeof(float64));
		l_pSubClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Classify);
		//this->getLogManager() << LogLevel_Info << l_iClassifierCounter << " " << (float64)op_f64ClassificationStateClass << " " << (*op_pClassificationValues)[0] << "\n";
		l_oClassificationVector.push_back(std::pair< float64, IMatrix*>((float64)op_f64ClassificationStateClass, (IMatrix*)op_pClassificationValues));
	}

	//Now, we determine the best classification
	std::pair<float64, IMatrix*> best = std::pair<float64, IMatrix*>(-1.0, (IMatrix*)NULL);
	rf64Class = -1;

	for(uint32 l_iClassificationCount = 0; l_iClassificationCount < l_oClassificationVector.size() ; ++l_iClassificationCount)
	{
		std::pair<float64, IMatrix*>&   l_pTemp = l_oClassificationVector[l_iClassificationCount];
		if(l_pTemp.first==1)
		{
			if(best.second == NULL)
			{
				best = l_pTemp;
				rf64Class = ((float64)l_iClassificationCount)+1;
			}
			else
			{
				if((*m_fAlgorithmComparison)((*best.second), *(l_pTemp.second)) < 0)
				{
					best = l_pTemp;
					rf64Class = ((float64)l_iClassificationCount)+1;
				}
			}
		}
	}

	//If no one recognize the class, let's take a random one
	//FIXME should take the most relevant
	if(rf64Class == -1)
	{
		for(uint32 l_iClassificationCount = 0; l_iClassificationCount < l_oClassificationVector.size() ; ++l_iClassificationCount)
		{
			std::pair<float64, IMatrix*>& l_pTemp = l_oClassificationVector[l_iClassificationCount];
			if(best.second == NULL)
			{
				best = l_pTemp;
				rf64Class = ((float64)l_iClassificationCount)+1;
			}
			else
			{
				if((*m_fAlgorithmComparison)((*best.second), *(l_pTemp.second)) > 0)
				{
					best = l_pTemp;
					rf64Class = ((float64)l_iClassificationCount)+1;
				}
			}
		}
	}

	if(best.second == NULL)
	{
		return false;
	}
	rClassificationValues.setSize(best.second->getBufferElementCount());
	System::Memory::copy(rClassificationValues.getBuffer(), best.second->getBuffer(), best.second->getBufferElementCount()*sizeof(float64));
	return true;
}

void CAlgorithmClassifierOneVsAll::addNewClassifierAtBack(void)
{
	IAlgorithmProxy* l_pSubClassifier = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(this->m_oSubClassifierAlgorithmIdentifier));
	l_pSubClassifier->initialize();

	//Set a references to the extra parameters input of the pairing strategy
	TParameterHandler< std::map<CString, CString>* > ip_pExtraParameters(l_pSubClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));
	ip_pExtraParameters.setReferenceTarget(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));

	this->m_oSubClassifierList.push_back(l_pSubClassifier);
}

void CAlgorithmClassifierOneVsAll::removeClassifierAtBack(void)
{
	IAlgorithmProxy* l_pSubClassifier = m_oSubClassifierList.back();
	l_pSubClassifier->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*l_pSubClassifier);
	this->m_oSubClassifierList.pop_back();
}

boolean CAlgorithmClassifierOneVsAll::designArchitecture(OpenViBE::CIdentifier &rId, int64 &rClassAmount)
{
	if(!this->setSubClassifierIdentifier(rId))
	{
		return false;
	}
	for(int64 i = 0 ; i < rClassAmount ; ++i)
	{
		this->addNewClassifierAtBack();
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

void CAlgorithmClassifierOneVsAll::generateConfigurationNode(void)
{
	std::stringstream l_sAmountClasses;
	l_sAmountClasses << getClassAmount();

	m_pConfigurationNode = XML::createNode(c_sClassifierRoot);

	XML::IXMLNode *l_pOneVsAllNode = XML::createNode(c_sTypeNodeName);

	XML::IXMLNode *l_pTempNode = XML::createNode(c_sSubClassifierIdentifierNodeName);
	l_pTempNode->addAttribute(c_sAlgorithmIdAttribute,this->m_oSubClassifierAlgorithmIdentifier.toString());
	l_pTempNode->setPCData(this->getTypeManager().getEnumerationEntryNameFromValue(OVTK_TypeId_ClassificationAlgorithm, m_oSubClassifierAlgorithmIdentifier.toUInteger()).toASCIIString());
	l_pOneVsAllNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sSubClassifierCountNodeName);
	l_pTempNode->setPCData(l_sAmountClasses.str().c_str());
	l_pOneVsAllNode->addChild(l_pTempNode);

	XML::IXMLNode *l_pSubClassifersNode = XML::createNode(c_sSubClassifiersNodeName);

	for(size_t i = 0; i<m_oSubClassifierList.size(); ++i)
	{
		l_pTempNode = XML::createNode(c_sSubClassifierNodeName);
		l_pTempNode->addChild(getClassifierConfiguration(m_oSubClassifierList[i]));
		l_pSubClassifersNode->addChild(l_pTempNode);
	}
	l_pOneVsAllNode->addChild(l_pSubClassifersNode);

	m_pConfigurationNode->addChild(l_pOneVsAllNode);
}

XML::IXMLNode* CAlgorithmClassifierOneVsAll::saveConfiguration(void)
{
	generateConfigurationNode();
	return m_pConfigurationNode;
}

boolean CAlgorithmClassifierOneVsAll::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	XML::IXMLNode *l_pOneVsAllNode = pConfigurationNode->getChild(0);

	XML::IXMLNode *l_pTempNode = l_pOneVsAllNode->getChildByName(c_sSubClassifierIdentifierNodeName);
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

	l_pTempNode = l_pOneVsAllNode->getChildByName(c_sSubClassifierCountNodeName);
	std::stringstream l_sCountData(l_pTempNode->getPCData());
	uint64 l_iAmountClass;
	l_sCountData >> l_iAmountClass;

	while(l_iAmountClass != getClassAmount())
	{
		if(l_iAmountClass < getClassAmount())
		{
			this->removeClassifierAtBack();
		}
		else
		{
			this->addNewClassifierAtBack();
		}
	}

	loadSubClassifierConfiguration(l_pOneVsAllNode->getChildByName(c_sSubClassifiersNodeName));

	return true;
}

void CAlgorithmClassifierOneVsAll::loadSubClassifierConfiguration(XML::IXMLNode *pSubClassifiersNode)
{
	for( uint32 i = 0; i < pSubClassifiersNode->getChildCount() ; ++i)
	{
		XML::IXMLNode *l_pSubClassifierNode = pSubClassifiersNode->getChild(i);
		TParameterHandler < XML::IXMLNode* > ip_pConfiguration(m_oSubClassifierList[i]->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_Configuration));
		ip_pConfiguration = l_pSubClassifierNode->getChild(0);
		m_oSubClassifierList[i]->process(OVTK_Algorithm_Classifier_InputTriggerId_LoadConfiguration);
	}
}

uint32 CAlgorithmClassifierOneVsAll::getClassAmount(void) const
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

