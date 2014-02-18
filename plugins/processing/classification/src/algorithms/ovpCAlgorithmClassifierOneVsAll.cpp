#include "ovpCAlgorithmClassifierOneVsAll.h"

#include <map>
#include <sstream>
#include <cstring>
#include <string>
#include <utility>
#include <iostream>


using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Local;

using namespace OpenViBEToolkit;


boolean CAlgorithmClassifierOneVsAll::uninitialize(void)
{
	while(!m_oSubClassifierList.empty())
	{
		this->getLogManager() << LogLevel_Warning << "Release classifier " << m_oSubClassifierList.size() <<"\n";
		this->removeClassifierAtBack();
	}
	return true;
}



boolean CAlgorithmClassifierOneVsAll::train(const IFeatureVectorSet& rFeatureVectorSet)
{

	uint32 i;
	std::map < float64, uint64 > l_vClassLabels;
	for(i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		l_vClassLabels[rFeatureVectorSet[i].getLabel()]++;
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
		l_oClassificationVector.push_back(std::pair< float64, IMatrix*>(op_f64ClassificationStateClass, op_pClassificationValues));
	}

	//Now, we determine the best classification
	std::pair<float64, IMatrix*> best = std::pair<float64, IMatrix*>(-1, NULL);
	IAlgorithmProxy* l_pSubClassifier = this->m_oSubClassifierList[0];
	rf64Class = -1;

	for(uint32 l_iClassificationCount = 0; l_iClassificationCount < l_oClassificationVector.size() ; ++l_iClassificationCount)
	{
		std::pair<float64, IMatrix*> l_pTemp = l_oClassificationVector[l_iClassificationCount];
		if(l_pTemp.first==1)
		{
			if(best.second == NULL)
			{
				best = l_pTemp;
				rf64Class = ((float64)l_iClassificationCount)+1;
			}
			else
			{
				if(((CAlgorithmClassifier*)l_pSubClassifier)->getBestClassification(*(best.second), *(l_pTemp.second)))
				{
					best = l_pTemp;
					rf64Class = ((float64)l_iClassificationCount)+1;
				}
			}
		}

	}

	//If no one recognize the class, let's take a random one
	if(rf64Class == -1)
	{
		uint32 l_iClassificationCount = rand()%l_oClassificationVector.size();
		rf64Class=1+l_iClassificationCount;
		best = l_oClassificationVector[l_iClassificationCount];
	}
	rClassificationValues.setSize(best.second->getBufferElementCount());
	System::Memory::copy(rClassificationValues.getBuffer(), best.second->getBuffer(), best.second->getBufferElementCount()*sizeof(float64));
	return true;
}

uint32 CAlgorithmClassifierOneVsAll::getBestClassification(IMatrix& rFirstClassificationValue, IMatrix& rSecondClassificationValue)
{
	return 0;
}

void CAlgorithmClassifierOneVsAll::addNewClassifierAtBack(void)
{
	IAlgorithmProxy* l_pSubClassifier = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(this->m_oSubClassifierAlgorithmIdentifier));
	l_pSubClassifier->initialize();
	this->m_oSubClassifierList.push_back(l_pSubClassifier);
	++(this->m_iAmountClass);
}

void CAlgorithmClassifierOneVsAll::removeClassifierAtBack(void)
{
	IAlgorithmProxy* l_pSubClassifier = m_oSubClassifierList.back();
	l_pSubClassifier->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*l_pSubClassifier);
	this->m_oSubClassifierList.pop_back();
	--(this->m_iAmountClass);
}

boolean CAlgorithmClassifierOneVsAll::designArchitecture(OpenViBE::CIdentifier &rId, uint64 &rClassAmount)
{
	m_oSubClassifierAlgorithmIdentifier = CIdentifier(rId);

	for(uint64 i = 1 ; i <= rClassAmount ; ++i)
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

XML::IXMLNode* CAlgorithmClassifierOneVsAll::saveConfiguration(void)
{
	std::stringstream l_sAmountClasses;
	l_sAmountClasses << this->m_iAmountClass;

	std::stringstream l_sClassIdentifier;
	l_sClassIdentifier << this->m_oSubClassifierAlgorithmIdentifier.toUInteger();

	m_pConfigurationNode = XML::createNode("OpenViBE-Classifier");

	XML::IXMLNode *l_pOneVsAllNode = XML::createNode("OneVsAll");

	XML::IXMLNode *l_pTempNode = XML::createNode("SubClassifierIdentifier");
	l_pTempNode->setPCData(l_sClassIdentifier.str().c_str());
	l_pOneVsAllNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode("SubClassifierCount");
	l_pTempNode->setPCData(l_sAmountClasses.str().c_str());
	l_pOneVsAllNode->addChild(l_pTempNode);

	XML::IXMLNode *l_pSubClassifersNode = XML::createNode("SubClassifiers");

	for(uint64 i = 0; i<m_oSubClassifierList.size(); ++i)
	{
		l_pTempNode = XML::createNode("SubClassifier");
		l_pTempNode->addChild(getClassifierConfiguration(m_oSubClassifierList[i]));
		l_pSubClassifersNode->addChild(l_pTempNode);
	}
	l_pOneVsAllNode->addChild(l_pSubClassifersNode);

	m_pConfigurationNode->addChild(l_pOneVsAllNode);

	std::cout << m_pConfigurationNode->getXML() << std::endl;
	return m_pConfigurationNode;
}

boolean CAlgorithmClassifierOneVsAll::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	this->m_iClassCounter = 0;

	XML::IXMLNode *l_pOneVsAllNode = pConfigurationNode->getChild(0);
	std::cout << l_pOneVsAllNode->getXML() << std::cout;

	XML::IXMLNode *l_pTempNode = l_pOneVsAllNode->getChildByName("SubClassifierIdentifier");
	std::stringstream l_sIdentifierData(l_pTempNode->getPCData());
	uint64 l_iIdentifier;
	l_sIdentifierData >> l_iIdentifier;
	if(m_oSubClassifierAlgorithmIdentifier.toUInteger() != l_iIdentifier)
	{
		while(!m_oSubClassifierList.empty())
		{
			this->removeClassifierAtBack();
		}
		m_oSubClassifierAlgorithmIdentifier = CIdentifier(l_iIdentifier);
	}

	l_pTempNode = l_pOneVsAllNode->getChildByName("SubClassifierCount");
	std::stringstream l_sCountData(l_pTempNode->getPCData());
	uint64 l_iAmountClass;
	l_sCountData >> l_iAmountClass;

	if(l_iAmountClass < this->m_iAmountClass)
	{
		while(this->m_iAmountClass > l_iAmountClass)
		{
			this->removeClassifierAtBack();
		}
	}
	else if(l_iAmountClass > this->m_iAmountClass)
	{
		while(this->m_iAmountClass < l_iAmountClass)
		{
			this->addNewClassifierAtBack();
		}
	}

	l_pTempNode = l_pOneVsAllNode->getChildByName("SubClassifiers");

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

void CAlgorithmClassifierOneVsAll::write(const char* sString)
{
	m_oConfiguration.append((const uint8*)sString, ::strlen(sString));
}

void CAlgorithmClassifierOneVsAll::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{
	m_vNode.push(sName);
}

void CAlgorithmClassifierOneVsAll::processChildData(const char* sData)
{


	if(m_vNode.top() == CString("SubClassifier"))
	{
		//We should be able to load configuration from scratch or to load it in an existing configuration
		//((CAlgorithmClassifier*)m_oSubClassifierList[this->m_iClassCounter])->loadConfiguration(m_oConfiguration);
		++(this->m_iClassCounter);
	}
	else if(m_vNode.top() == CString("SubClassifierIdentifier"))
	{
		std::stringstream l_sData(sData);
		uint64 l_iIdentifier;
		l_sData >> l_iIdentifier;
		//We are in this case if we create the configuration of if we change the algorithm previously used by the pairing strategy
		if(m_oSubClassifierAlgorithmIdentifier.toUInteger() != l_iIdentifier)
		{
			while(!m_oSubClassifierList.empty())
			{
				this->removeClassifierAtBack();
			}
			m_oSubClassifierAlgorithmIdentifier = CIdentifier(l_iIdentifier);
		}
	}
	else if(m_vNode.top() == CString("SubClassifierCount"))
	{
		std::stringstream l_sData(sData);
		uint64 l_iAmountClass;
		l_sData >> l_iAmountClass;

		if(l_iAmountClass < this->m_iAmountClass)
		{
			while(this->m_iAmountClass > l_iAmountClass)
			{
				this->removeClassifierAtBack();
			}
		}
		else if(l_iAmountClass > this->m_iAmountClass)
		{
			while(this->m_iAmountClass < l_iAmountClass)
			{
				this->addNewClassifierAtBack();
			}
		}
	}

}

void CAlgorithmClassifierOneVsAll::closeChild(void)
{
	m_vNode.pop();
}

