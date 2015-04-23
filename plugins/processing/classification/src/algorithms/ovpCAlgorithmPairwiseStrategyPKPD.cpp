#define PKPD_DEBUG 0
#include "ovpCAlgorithmPairwiseStrategyPKPD.h"

#include <iostream>


#include <xml/IXMLNode.h>
#include <xml/IXMLHandler.h>

namespace{
	const char* const c_sTypeNodeName = "PairwiseDecision_PKDP";
}


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

boolean CAlgorithmPairwiseStrategyPKPD::parametrize()
{
	TParameterHandler < uint32 > ip_pClassAmount(this->getInputParameter(OVP_Algorithm_Classifier_Pairwise_InputParameter_AmountClass));
	m_ui32AmountClass = ip_pClassAmount;

	return true;
}



boolean CAlgorithmPairwiseStrategyPKPD::compute(std::vector< SClassificationInfo >& pClassificationValueList, OpenViBE::IMatrix* pProbabiltyVector)
{
	float64* l_pProbabiltyMatrix = new float64[m_ui32AmountClass * m_ui32AmountClass];

	//First we set the diagonal to 0
	for(size_t i = 0 ; i < m_ui32AmountClass ; ++i)
	{
		l_pProbabiltyMatrix[i*m_ui32AmountClass + i] = 0.;
	}

	for(size_t i = 0 ; i < pClassificationValueList.size() ; ++i)
	{
		SClassificationInfo& l_rTemp = pClassificationValueList[i];
		const uint32 l_f64FirstIndex = l_rTemp.m_f64FirstClass -1;
		const uint32 l_f64SecondIndex = l_rTemp.m_f64SecondClass -1;
		const float64* l_pValues = l_rTemp.m_pClassificationValue->getBuffer();
		l_pProbabiltyMatrix[l_f64FirstIndex * m_ui32AmountClass + l_f64SecondIndex] = l_pValues[0];
		l_pProbabiltyMatrix[l_f64SecondIndex * m_ui32AmountClass + l_f64FirstIndex] = 1 - l_pValues[0];
	}

#if PKPD_DEBUG
	for(OpenViBE::uint32 i = 0 ; i< m_ui32AmountClass ; ++i){

		for(OpenViBE::uint32 j = 0 ; j<m_ui32AmountClass ; ++j){
			std::cout << l_pProbabiltyMatrix[i*m_ui32AmountClass + j] << " ";
		}
		std::cout << std::endl;
	}
#endif

	float64* l_pProbVector = new float64[m_ui32AmountClass];
	float64 l_pProbVectorSum = 0;
	for(OpenViBE::uint32 l_ui32ClassIndex = 0 ; l_ui32ClassIndex < m_ui32AmountClass ; ++l_ui32ClassIndex)
	{
		float64 l_pTempSum = 0;
		for(OpenViBE::uint32 l_ui32SecondClass = 0 ; l_ui32SecondClass<m_ui32AmountClass ; ++l_ui32SecondClass)
		{
			if(l_ui32SecondClass != l_ui32ClassIndex)
			{
				l_pTempSum += 1/l_pProbabiltyMatrix[m_ui32AmountClass*l_ui32ClassIndex + l_ui32SecondClass];
			}
		}
		l_pProbVector[l_ui32ClassIndex] = 1 /(l_pTempSum - (m_ui32AmountClass -2));
		l_pProbVectorSum += l_pProbVector[l_ui32ClassIndex];
	}

	for(OpenViBE::uint32 i = 0; i<m_ui32AmountClass ; ++i)
	{
		l_pProbVector[i] /= l_pProbVectorSum;
	}

#if PKPD_DEBUG
	for(OpenViBE::uint32 i = 0; i<m_ui32AmountClass ; ++i)
	{
		std::cout << l_pProbVector[i] << " ";
	}
	std::cout << std::endl;
#endif

	pProbabiltyVector->setDimensionCount(1);
	pProbabiltyVector->setDimensionSize(0,m_ui32AmountClass);

	for(OpenViBE::uint32 i = 0 ; i<m_ui32AmountClass ; ++i)
	{
		pProbabiltyVector->getBuffer()[i] = l_pProbVector[i];
	}

	delete[] l_pProbabiltyMatrix;
	delete[] l_pProbVector;
	return true;
}

XML::IXMLNode* CAlgorithmPairwiseStrategyPKPD::saveConfiguration()
{
	XML::IXMLNode* l_pRootNode = XML::createNode(c_sTypeNodeName);
	return l_pRootNode;
}

boolean CAlgorithmPairwiseStrategyPKPD::loadConfiguration(XML::IXMLNode& rNode)
{
	return true;
}
