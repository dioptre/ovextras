#define HT_DEBUG 0
#include "ovpCAlgorithmPairwiseDecisionHT.h"

#include <iostream>


#include <xml/IXMLNode.h>
#include <xml/IXMLHandler.h>

static const char* const c_sTypeNodeName = "PairwiseDecision_HT";


using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

boolean CAlgorithmPairwiseDecisionHT::initialize()
{
	return true;
}

boolean CAlgorithmPairwiseDecisionHT::uninitialize()
{
	return true;
}



boolean CAlgorithmPairwiseDecisionHT::compute(OpenViBE::IMatrix* pSubClassifierMatrix, OpenViBE::IMatrix* pProbabiltyVector)
{
	TParameterHandler<IMatrix*> ip_pRepartitionSetVector = this->getInputParameter(OVP_Algorithm_Classifier_Pairwise_InputParameterId_SetRepartition);

	OpenViBE::uint32 l_ui32AmountClass = pSubClassifierMatrix->getDimensionSize(0);

#if HT_DEBUG
	for(OpenViBE::uint32 i = 0 ; i< l_ui32AmountClass ; ++i){
			std::cout << ip_pRepartitionSetVector->getBuffer()[i] << " ";
	}
	std::cout << std::endl;
#endif

#if HT_DEBUG
	std::cout << l_ui32AmountClass << std::endl;

	for(OpenViBE::uint32 i = 0 ; i< l_ui32AmountClass ; ++i){

		for(OpenViBE::uint32 j = 0 ; j<l_ui32AmountClass ; ++j){
			std::cout << pSubClassifierMatrix->getBuffer()[i*l_ui32AmountClass + j] << " ";
		}
		std::cout << std::endl;
	}
#endif

#if HT_DEBUG
	for(OpenViBE::uint32 i = 0; i<l_ui32AmountClass ; ++i)
	{
		std::cout << l_pProbVector[i] << " ";
	}
	std::cout << std::endl;
#endif

	pProbabiltyVector->setDimensionCount(1);
	pProbabiltyVector->setDimensionSize(0,l_ui32AmountClass);

	return true;
}

XML::IXMLNode* CAlgorithmPairwiseDecisionHT::saveConfiguration()
{
	XML::IXMLNode* l_pRootNode = XML::createNode(c_sTypeNodeName);
	return l_pRootNode;
}

boolean CAlgorithmPairwiseDecisionHT::loadConfiguration(XML::IXMLNode& rNode)
{
	return true;
}
