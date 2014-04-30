#define PKPD_DEBUG 0
#include "ovpCAlgorithmPairwiseStrategyPKPD.h"

#include <iostream>


#include <xml/IXMLNode.h>
#include <xml/IXMLHandler.h>

static const char* const c_sTypeNodeName = "PairwiseDecision_PKDP";


using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

boolean CAlgorithmPairwiseStrategyPKPD::initialize()
{
	return true;
}

boolean CAlgorithmPairwiseStrategyPKPD::uninitialize()
{
	return true;
}



boolean CAlgorithmPairwiseStrategyPKPD::classify()
{
	TParameterHandler<IMatrix *> ip_pProbabilityMatrix = this->getInputParameter(OVP_Algorithm_Classifier_InputParameter_ProbabilityMatrix);
	IMatrix * l_pProbabilityMatrix = (IMatrix *)ip_pProbabilityMatrix;

	OpenViBE::uint32 l_ui32AmountClass = l_pProbabilityMatrix->getDimensionSize(0);

#if PKPD_DEBUG
	std::cout << l_pProbabilityMatrix->getDimensionSize(0) << std::endl;

	for(OpenViBE::uint32 i = 0 ; i< l_ui32AmountClass ; ++i){

		for(OpenViBE::uint32 j = 0 ; j<l_ui32AmountClass ; ++j){
			std::cout << l_pProbabilityMatrix->getBuffer()[i*l_ui32AmountClass + j] << " ";
		}
		std::cout << std::endl;
	}
#endif

	float64* l_pMatrixBuffer = l_pProbabilityMatrix->getBuffer();
	float64* l_pProbVector = new float64[l_ui32AmountClass];
	float64 l_pProbVectorSum = 0;
	for(OpenViBE::uint32 l_ui32ClassIndex = 0 ; l_ui32ClassIndex < l_ui32AmountClass ; ++l_ui32ClassIndex)
	{
		float64 l_pTempSum = 0;
		for(OpenViBE::uint32 l_ui32SecondClass = 0 ; l_ui32SecondClass<l_ui32AmountClass ; ++l_ui32SecondClass)
		{
			if(l_ui32SecondClass != l_ui32ClassIndex)
			{
				l_pTempSum += 1/l_pMatrixBuffer[l_ui32AmountClass*l_ui32ClassIndex + l_ui32SecondClass];
			}
		}
		l_pProbVector[l_ui32ClassIndex] = 1 /(l_pTempSum - (l_ui32AmountClass -2));
		l_pProbVectorSum += l_pProbVector[l_ui32ClassIndex];
	}

	for(OpenViBE::uint32 i = 0; i<l_ui32AmountClass ; ++i)
	{
		l_pProbVector[i] /= l_pProbVectorSum;
	}

#if PKPD_DEBUG
	for(OpenViBE::uint32 i = 0; i<l_ui32AmountClass ; ++i)
	{
		std::cout << l_pProbVector[i] << " ";
	}
	std::cout << std::endl;
#endif

	TParameterHandler<IMatrix*> op_pProbablityVector = this->getOutputParameter(OVP_Algorithm_Classifier_OutputParameter_ProbabilityVector);
	op_pProbablityVector->setDimensionCount(1);
	op_pProbablityVector->setDimensionSize(0,l_ui32AmountClass);

	for(OpenViBE::uint32 i = 0 ; i<l_ui32AmountClass ; ++i)
	{
		op_pProbablityVector->getBuffer()[i] = l_pProbVector[i];
	}

	delete[] l_pProbVector;
	return true;
}

boolean CAlgorithmPairwiseStrategyPKPD::saveConfiguration()
{
	TParameterHandler < XML::IXMLNode* > op_pConfiguration(this->getOutputParameter(OVP_Algorithm_Classifier_Pairwise_OutputParameterId_Configuration));
	XML::IXMLNode* l_pRootNode = XML::createNode(c_sTypeNodeName);
	op_pConfiguration = l_pRootNode;
	return true;
}

boolean CAlgorithmPairwiseStrategyPKPD::loadConfiguration()
{
	return true;
}
