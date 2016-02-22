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

boolean CAlgorithmPairwiseStrategyPKPD::parameterize()
{
	TParameterHandler < uint64 > ip_pClassCount(this->getInputParameter(OVP_Algorithm_Classifier_Pairwise_InputParameter_ClassCount));
	m_ui32ClassCount = static_cast<uint32>(ip_pClassCount);

	if(m_ui32ClassCount<2) 
	{
		this->getLogManager() << LogLevel_Error << "Algorithm needs at least 2 classes.\n";
		return false;
	}

	return true;
}



boolean CAlgorithmPairwiseStrategyPKPD::compute(std::vector< SClassificationInfo >& pClassificationValueList, OpenViBE::IMatrix* pProbabilityVector)
{
	if(m_ui32ClassCount<2) {
		this->getLogManager() << LogLevel_Error << "Algorithm needs at least 2 classes. Has parameterize() been called?\n";
		return false;
	}

	float64* l_pProbabilityMatrix = new float64[m_ui32ClassCount * m_ui32ClassCount];

	//First we set the diagonal to 0
	for(size_t i = 0 ; i < m_ui32ClassCount ; ++i)
	{
		l_pProbabilityMatrix[i*m_ui32ClassCount + i] = 0.;
	}

	for(size_t i = 0 ; i < pClassificationValueList.size() ; ++i)
	{
		SClassificationInfo& l_rTemp = pClassificationValueList[i];
		const uint32 l_f64FirstIndex = static_cast<uint32>(l_rTemp.m_f64FirstClass) -1;
		const uint32 l_f64SecondIndex = static_cast<uint32>(l_rTemp.m_f64SecondClass) -1;
		const float64* l_pValues = l_rTemp.m_pClassificationValue->getBuffer();
		l_pProbabilityMatrix[l_f64FirstIndex * m_ui32ClassCount + l_f64SecondIndex] = l_pValues[0];
		l_pProbabilityMatrix[l_f64SecondIndex * m_ui32ClassCount + l_f64FirstIndex] = 1 - l_pValues[0];
	}

#if PKPD_DEBUG
	for(OpenViBE::uint32 i = 0 ; i< m_ui32ClassCount ; ++i){

		for(OpenViBE::uint32 j = 0 ; j<m_ui32ClassCount ; ++j){
			std::cout << l_pProbabilityMatrix[i*m_ui32ClassCount + j] << " ";
		}
		std::cout << std::endl;
	}
#endif

	float64* l_pProbVector = new float64[m_ui32ClassCount];
	float64 l_pProbVectorSum = 0;
	for(OpenViBE::uint32 l_ui32ClassIndex = 0 ; l_ui32ClassIndex < m_ui32ClassCount ; ++l_ui32ClassIndex)
	{
		float64 l_pTempSum = 0;
		for(OpenViBE::uint32 l_ui32SecondClass = 0 ; l_ui32SecondClass<m_ui32ClassCount ; ++l_ui32SecondClass)
		{
			if(l_ui32SecondClass != l_ui32ClassIndex)
			{
				l_pTempSum += 1/l_pProbabilityMatrix[m_ui32ClassCount*l_ui32ClassIndex + l_ui32SecondClass];
			}
		}
		l_pProbVector[l_ui32ClassIndex] = 1 /(l_pTempSum - (m_ui32ClassCount -2));
		l_pProbVectorSum += l_pProbVector[l_ui32ClassIndex];
	}

	for(OpenViBE::uint32 i = 0; i<m_ui32ClassCount ; ++i)
	{
		l_pProbVector[i] /= l_pProbVectorSum;
	}

#if PKPD_DEBUG
	for(OpenViBE::uint32 i = 0; i<m_ui32ClassCount ; ++i)
	{
		std::cout << l_pProbVector[i] << " ";
	}
	std::cout << std::endl;
#endif

	pProbabilityVector->setDimensionCount(1);
	pProbabilityVector->setDimensionSize(0,m_ui32ClassCount);

	for(OpenViBE::uint32 i = 0 ; i<m_ui32ClassCount ; ++i)
	{
		pProbabilityVector->getBuffer()[i] = l_pProbVector[i];
	}

	delete[] l_pProbabilityMatrix;
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
