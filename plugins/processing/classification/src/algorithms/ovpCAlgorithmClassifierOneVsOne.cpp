
#include "ovpCAlgorithmClassifierOneVsOne.h"

#include <map>
#include <sstream>
#include <cstring>
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

	return CAlgorithmPairingStrategy::initialize();
}

boolean CAlgorithmClassifierOneVsOne::uninitialize(void)
{
	return true;
}



boolean CAlgorithmClassifierOneVsOne::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	return true;
}

boolean CAlgorithmClassifierOneVsOne::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
{
	return true;
}

boolean CAlgorithmClassifierOneVsOne::designArchitecture(OpenViBE::CIdentifier &rId, int64 &rClassAmount)
{
	return true;
}

XML::IXMLNode* CAlgorithmClassifierOneVsOne::getClassifierConfiguration(IAlgorithmProxy* rClassifier)
{
	return NULL;
}

void CAlgorithmClassifierOneVsOne::generateConfigurationNode(void)
{
}

XML::IXMLNode* CAlgorithmClassifierOneVsOne::saveConfiguration(void)
{
	return NULL;
}

boolean CAlgorithmClassifierOneVsOne::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	return true;
}

void CAlgorithmClassifierOneVsOne::loadSubClassifierConfiguration(XML::IXMLNode *pSubClassifiersNode)
{
}

uint32 CAlgorithmClassifierOneVsOne::getClassAmount(void) const
{
	//We use a formula because the list as the reponsability ot the count of subClassifier and by extention of amount of classes
	uint32 l_ui32DeltaCarre = 1+8*m_oSubClassifierDescriptorList.size();
	return (1+l_ui32DeltaCarre/l_ui32DeltaCarre)/2;
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

