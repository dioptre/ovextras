
#include "ovpCAlgorithmClassifierOneVsOne.h"

#include <map>
#include <cmath>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <utility>
#include <iostream>
#include <system/Memory.h>


static const char* const c_sTypeNodeName = "OneVsOne";
static const char* const c_sSubClassifierIdentifierNodeName = "SubClassifierIdentifier";
static const char* const c_sAlgorithmIdAttribute = "algorithm-id";
static const char* const c_sSubClassifierCountNodeName = "SubClassifierCount";
static const char* const c_sSubClassifiersNodeName = "SubClassifiers";
static const char* const c_sSubClassifierNodeName = "SubClassifier";

static const char* const c_sFirstClassAtrributeName = "first-class";
static const char* const c_sSecondClassAttributeName = "second-class";

extern const char* const c_sClassifierRoot;

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

boolean CAlgorithmClassifierOneVsOne::initialize()
{
	TParameterHandler < XML::IXMLNode* > op_pConfiguration(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	op_pConfiguration=NULL;

	m_pDecisionStrategyAlgorithm = NULL;

	return CAlgorithmPairingStrategy::initialize();
}

boolean CAlgorithmClassifierOneVsOne::uninitialize(void)
{
	if(m_pDecisionStrategyAlgorithm != NULL)
	{
		m_pDecisionStrategyAlgorithm->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pDecisionStrategyAlgorithm);
	}

	return CAlgorithmPairingStrategy::uninitialize();
}



boolean CAlgorithmClassifierOneVsOne::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	if(m_pDecisionStrategyAlgorithm != NULL){
		m_pDecisionStrategyAlgorithm->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pDecisionStrategyAlgorithm);
	}
	//Create the decision strategy
	IAlgorithmProxy *l_pAlgoProxy = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ClassifierOneVsOne));
	l_pAlgoProxy->initialize();


	TParameterHandler< std::map<CString, CString>* > ip_pExtraParameters(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));
	CString l_pParameterName = l_pAlgoProxy->getInputParameterName(OVP_Algorithm_OneVsOneStrategy_InputParameterId_DecisionType);
	CIdentifier l_oDecisionClassIdentifier=this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_ClassificationPairwiseStrategy,
																											  ip_pExtraParameters->at(l_pParameterName));
	m_pDecisionStrategyAlgorithm = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(l_oDecisionClassIdentifier));
	m_pDecisionStrategyAlgorithm->initialize();

	l_pAlgoProxy->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*l_pAlgoProxy);

	//Calculate the amount of sample for each class
	std::map < float64, size_t > l_vClassLabels;
	for(uint32 i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		if(!l_vClassLabels.count(rFeatureVectorSet[i].getLabel()))
		{
			l_vClassLabels[rFeatureVectorSet[i].getLabel()] = 0;
		}
		l_vClassLabels[rFeatureVectorSet[i].getLabel()]++;
	}

	//Now let's train each classifier
	for(size_t l_iFirstClass=1 ; l_iFirstClass <= getClassAmount(); ++l_iFirstClass)
	{
		for(size_t l_iSecondClass = l_iFirstClass+1 ; l_iSecondClass <= getClassAmount() ; ++l_iSecondClass)
		{
			size_t l_iFeatureVectorSize=rFeatureVectorSet[0].getSize();
			size_t l_iFeatureCount = l_vClassLabels[(float64)l_iFirstClass] + l_vClassLabels[(float64)l_iSecondClass];

			IAlgorithmProxy* l_pSubClassifier = getSubClassifierDescriptor(l_iFirstClass, l_iSecondClass).m_pSubClassifierProxy;

			TParameterHandler < IMatrix* > ip_pFeatureVectorSet(l_pSubClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet));
			ip_pFeatureVectorSet->setDimensionCount(2);
			ip_pFeatureVectorSet->setDimensionSize(0, l_iFeatureCount);
			ip_pFeatureVectorSet->setDimensionSize(1, l_iFeatureVectorSize+1);

			float64* l_pFeatureVectorSetBuffer=ip_pFeatureVectorSet->getBuffer();
			for(size_t j=0; j<rFeatureVectorSet.getFeatureVectorCount(); j++)
			{
				float64 l_f64TempClass = rFeatureVectorSet[j].getLabel();
				if(l_f64TempClass == (float64)l_iFirstClass || l_f64TempClass == (float64)l_iSecondClass)
				{
					System::Memory::copy(
								l_pFeatureVectorSetBuffer,
								rFeatureVectorSet[j].getBuffer(),
								l_iFeatureVectorSize*sizeof(float64));

					if((size_t)l_f64TempClass == l_iFirstClass )
					{
						l_pFeatureVectorSetBuffer[l_iFeatureVectorSize]=1;
					}
					else
					{
						l_pFeatureVectorSetBuffer[l_iFeatureVectorSize]=2;
					}
					l_pFeatureVectorSetBuffer+=(l_iFeatureVectorSize+1);
				}
			}
			l_pSubClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Train);

		}
	}
	return true;
}

boolean CAlgorithmClassifierOneVsOne::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
{
//	std::cout << "Starting Classification" << std::endl;
	const OpenViBE::uint32 l_ui32AmountClass = this->getClassAmount();
	uint32 l_ui32FeatureVectorSize=rFeatureVector.getSize();

	TParameterHandler<IMatrix*> ip_pProbabilityMatrix = m_pDecisionStrategyAlgorithm->getInputParameter(OVP_Algorithm_Classifier_InputParameter_ProbabilityMatrix);
	IMatrix * l_pProbabilityMatrix = (IMatrix*)ip_pProbabilityMatrix;

	l_pProbabilityMatrix->setDimensionCount(2);
	l_pProbabilityMatrix->setDimensionSize(0, l_ui32AmountClass);
	l_pProbabilityMatrix->setDimensionSize(1, l_ui32AmountClass);

	for(OpenViBE::uint32 i =0; i < l_ui32AmountClass ; ++i)
	{
		l_pProbabilityMatrix->getBuffer()[i*l_ui32AmountClass + i] = 0.0;
	}

	for(OpenViBE::uint32 i =0; i< l_ui32AmountClass ; ++i)
	{
		for(OpenViBE::uint32 j = i+1 ; j< l_ui32AmountClass ; ++j)
		{
			IAlgorithmProxy * l_pTempProxy = this->getSubClassifierDescriptor(i+1, j+1).m_pSubClassifierProxy;
			TParameterHandler < IMatrix* > ip_pFeatureVector(l_pTempProxy->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVector));
			TParameterHandler < IMatrix* > op_pClassificationValues(l_pTempProxy->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
			ip_pFeatureVector->setDimensionCount(1);
			ip_pFeatureVector->setDimensionSize(0, l_ui32FeatureVectorSize);

			float64* l_pFeatureVectorBuffer=ip_pFeatureVector->getBuffer();
			System::Memory::copy(
						l_pFeatureVectorBuffer,
						rFeatureVector.getBuffer(),
						l_ui32FeatureVectorSize*sizeof(float64));
			l_pTempProxy->process(OVP_Algorithm_Classifier_Pairwise_InputTriggerId_Classifiy);

			//We have only probability here
			float64 l_f64Prob = op_pClassificationValues->getBuffer()[0];
			l_pProbabilityMatrix->getBuffer()[i*l_ui32AmountClass + j] = l_f64Prob;
			l_pProbabilityMatrix->getBuffer()[j*l_ui32AmountClass + i] = 1-l_f64Prob;
		}
	}
	m_pDecisionStrategyAlgorithm->process(OVP_Algorithm_Classifier_Pairwise_InputTriggerId_Classifiy);

	TParameterHandler<IMatrix*> op_pProbabilityVector = m_pDecisionStrategyAlgorithm->getOutputParameter(OVP_Algorithm_Classifier_OutputParameter_ProbabilityVector);
	float64 l_f64MaxProb = -1;
	int32 l_i32IndexSelectedClass = -1;

	for(uint32 i = 0 ; i < l_ui32AmountClass ; ++i)
	{
		float64 l_f64TempProb = op_pProbabilityVector->getBuffer()[i];
		if(l_f64TempProb > l_f64MaxProb)
		{
			l_i32IndexSelectedClass = i+1;
			l_f64MaxProb = l_f64TempProb;
		}
	}


	rf64Class = (float64)l_i32IndexSelectedClass;
	rClassificationValues.setSize(1);
	rClassificationValues[0]=l_f64MaxProb;
	return true;
}

boolean CAlgorithmClassifierOneVsOne::designArchitecture(OpenViBE::CIdentifier &rId, int64 &rClassAmount)
{
	if(!setSubClassifierIdentifier(rId))
	{
		return false;
	}

	//Now let's instantiate all the sub_classifier
	for(int64 l_iFirstClass=1 ; l_iFirstClass <= rClassAmount; ++l_iFirstClass)
	{
		for(int64 l_iSecondClass = l_iFirstClass+1 ; l_iSecondClass <= rClassAmount ; ++l_iSecondClass)
		{
			IAlgorithmProxy* l_pSubClassifier = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(this->m_oSubClassifierAlgorithmIdentifier));
			l_pSubClassifier->initialize();

			//Set a references to the extra parameters input of the pairing strategy
			TParameterHandler< std::map<CString, CString>* > ip_pExtraParameters(l_pSubClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));
			ip_pExtraParameters.setReferenceTarget(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));

			SSubClassifierDescriptor l_pTempDesc = {(float64)l_iFirstClass, (float64)l_iSecondClass, l_pSubClassifier};
			this->m_oSubClassifierDescriptorList.push_back(l_pTempDesc);
		}
	}
	return true;
}

XML::IXMLNode* CAlgorithmClassifierOneVsOne::getClassifierConfiguration(SSubClassifierDescriptor &rDescriptor)
{
	XML::IXMLNode * l_pRes = XML::createNode(c_sSubClassifierNodeName);

	std::stringstream l_sFirstClass, l_sSecondClass;
	l_sFirstClass << rDescriptor.m_f64FirstClass;
	l_sSecondClass << rDescriptor.m_f64SecondClass;
	l_pRes->addAttribute(c_sFirstClassAtrributeName, l_sFirstClass.str().c_str());
	l_pRes->addAttribute(c_sSecondClassAttributeName, l_sSecondClass.str().c_str());

	TParameterHandler < XML::IXMLNode* > op_pConfiguration(rDescriptor.m_pSubClassifierProxy->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	rDescriptor.m_pSubClassifierProxy->process(OVTK_Algorithm_Classifier_InputTriggerId_SaveConfiguration);
	l_pRes->addChild((XML::IXMLNode*)op_pConfiguration);

	return l_pRes;
}

void CAlgorithmClassifierOneVsOne::generateConfigurationNode(void)
{
	std::stringstream l_sAmountClasses;
	l_sAmountClasses << getClassAmount();

	std::stringstream l_sClassIdentifier;
	l_sClassIdentifier << this->m_oSubClassifierAlgorithmIdentifier.toUInteger();

	m_pConfigurationNode = XML::createNode(c_sClassifierRoot);

	XML::IXMLNode *l_pOneVsOneNode = XML::createNode(c_sTypeNodeName);

	XML::IXMLNode *l_pTempNode = XML::createNode(c_sSubClassifierIdentifierNodeName);
	l_pTempNode->addAttribute(c_sAlgorithmIdAttribute,l_sClassIdentifier.str().c_str());
	l_pTempNode->setPCData(this->getTypeManager().getEnumerationEntryNameFromValue(OVTK_TypeId_ClassificationAlgorithm, m_oSubClassifierAlgorithmIdentifier.toUInteger()).toASCIIString());
	l_pOneVsOneNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sSubClassifierCountNodeName);
	l_pTempNode->setPCData(l_sAmountClasses.str().c_str());
	l_pOneVsOneNode->addChild(l_pTempNode);

	XML::IXMLNode *l_pSubClassifersNode = XML::createNode(c_sSubClassifiersNodeName);
	for(size_t i = 0; i<m_oSubClassifierDescriptorList.size(); ++i)
	{
		l_pSubClassifersNode->addChild(getClassifierConfiguration(m_oSubClassifierDescriptorList[i]));
	}
	l_pOneVsOneNode->addChild(l_pSubClassifersNode);

	m_pConfigurationNode->addChild(l_pOneVsOneNode);
}

XML::IXMLNode* CAlgorithmClassifierOneVsOne::saveConfiguration(void)
{
	generateConfigurationNode();
	return m_pConfigurationNode;
}

boolean CAlgorithmClassifierOneVsOne::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	XML::IXMLNode *l_pOneVsOneNode = pConfigurationNode->getChild(0);

	XML::IXMLNode *l_pTempNode = l_pOneVsOneNode->getChildByName(c_sSubClassifierIdentifierNodeName);
	std::stringstream l_sIdentifierData(l_pTempNode->getAttribute(c_sAlgorithmIdAttribute));
	uint64 l_iIdentifier;
	l_sIdentifierData >> l_iIdentifier;
	if(m_oSubClassifierAlgorithmIdentifier.toUInteger() != l_iIdentifier)
	{
		while(!m_oSubClassifierDescriptorList.empty())
		{
			IAlgorithmProxy* l_pSubClassifier = m_oSubClassifierDescriptorList.back().m_pSubClassifierProxy;
			l_pSubClassifier->uninitialize();
			this->getAlgorithmManager().releaseAlgorithm(*l_pSubClassifier);
			this->m_oSubClassifierDescriptorList.pop_back();
		}
		if(!this->setSubClassifierIdentifier(l_iIdentifier)){
			//if the sub classifier doesn't have comparison function it is an error
			return false;
		}
	}

	l_pTempNode = l_pOneVsOneNode->getChildByName(c_sSubClassifierCountNodeName);
	std::stringstream l_sCountData(l_pTempNode->getPCData());
	uint64 l_iAmountClass;
	l_sCountData >> l_iAmountClass;

	while(l_iAmountClass != getClassAmount())
	{
		if(l_iAmountClass < getClassAmount())
		{
			IAlgorithmProxy* l_pSubClassifier = m_oSubClassifierDescriptorList.back().m_pSubClassifierProxy;
			l_pSubClassifier->uninitialize();
			this->getAlgorithmManager().releaseAlgorithm(*l_pSubClassifier);
			this->m_oSubClassifierDescriptorList.pop_back();
		}
		else
		{
			IAlgorithmProxy* l_pSubClassifier = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(this->m_oSubClassifierAlgorithmIdentifier));
			l_pSubClassifier->initialize();

			//Set a references to the extra parameters input of the pairing strategy
			TParameterHandler< std::map<CString, CString>* > ip_pExtraParameters(l_pSubClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));
			ip_pExtraParameters.setReferenceTarget(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));

			SSubClassifierDescriptor l_pTempDesc = {1, 2, l_pSubClassifier};
			this->m_oSubClassifierDescriptorList.push_back(l_pTempDesc);
		}
	}

	loadSubClassifierConfiguration(l_pOneVsOneNode->getChildByName(c_sSubClassifiersNodeName));
	return true;
}

void CAlgorithmClassifierOneVsOne::loadSubClassifierConfiguration(XML::IXMLNode *pSubClassifiersNode)
{
	for( uint32 i = 0; i < pSubClassifiersNode->getChildCount() ; ++i)
	{
		XML::IXMLNode *l_pSubClassifierNode = pSubClassifiersNode->getChild(i);
		IAlgorithmProxy* l_pSubClassifier = m_oSubClassifierDescriptorList[i].m_pSubClassifierProxy;
		TParameterHandler < XML::IXMLNode* > ip_pConfiguration(l_pSubClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_Configuration));
		ip_pConfiguration = l_pSubClassifierNode->getChild(0);
		l_pSubClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_LoadConfiguration);

		//Now we have to restore classes name
		std::stringstream l_sFirstClass(l_pSubClassifierNode->getAttribute(c_sFirstClassAtrributeName));
		l_sFirstClass >> m_oSubClassifierDescriptorList[i].m_f64FirstClass;
		std::stringstream l_sSecondClass(l_pSubClassifierNode->getAttribute(c_sSecondClassAttributeName));
		l_sSecondClass >> m_oSubClassifierDescriptorList[i].m_f64SecondClass;
	}
}

uint32 CAlgorithmClassifierOneVsOne::getClassAmount(void) const
{
	//We use a formula because the list as the reponsability ot the count of subClassifier and by extention of amount of classes
	uint32 l_ui32DeltaCarre = 1+8*m_oSubClassifierDescriptorList.size();
	return (int)(1+::sqrt((double)l_ui32DeltaCarre))/2;
}

//The function take int because we don't take the "label" of the class but only the numero of declaration
SSubClassifierDescriptor &CAlgorithmClassifierOneVsOne::getSubClassifierDescriptor(const uint32 ui32FirstClass, const uint32 ui32SecondClass)
{
	uint32 ui32Max, ui32Min;
	if(ui32FirstClass > ui32SecondClass)
	{
		ui32Max = ui32FirstClass;
		ui32Min = ui32SecondClass;
	}
	else
	{
		ui32Max = ui32FirstClass;
		ui32Min = ui32SecondClass;
	}
	//TODO explanation of formula
	uint32 index = getClassAmount()*(ui32Max-1) - ((ui32Max -1)*ui32Max)/2 + ui32Min -ui32Max -1;
	return this->m_oSubClassifierDescriptorList[index];
}

boolean CAlgorithmClassifierOneVsOne::setSubClassifierIdentifier(const OpenViBE::CIdentifier &rId)
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

