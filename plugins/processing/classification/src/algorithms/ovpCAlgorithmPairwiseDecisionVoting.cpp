#define VOTING_DEBUG 0
#include "ovpCAlgorithmPairwiseDecisionVoting.h"

#include <iostream>


#include <xml/IXMLNode.h>
#include <xml/IXMLHandler.h>

namespace{
	const char* const c_sTypeNodeName = "PairwiseDecision_Voting";
}

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

boolean CAlgorithmPairwiseDecisionVoting::initialize()
{
	return true;
}

boolean CAlgorithmPairwiseDecisionVoting::uninitialize()
{
	return true;
}

boolean CAlgorithmPairwiseDecisionVoting::parameterize()
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



boolean CAlgorithmPairwiseDecisionVoting::compute(std::vector< SClassificationInfo >& pClassificationValueList, OpenViBE::IMatrix* pProbabilityVector)
{
	if(m_ui32ClassCount<2) {
		this->getLogManager() << LogLevel_Error << "Algorithm needs at least 2 classes. Has parameterize() been called?\n";
		return false;
	}

#if VOTING_DEBUG
	std::cout << pClassificationValueList.size() << std::endl;

	for(OpenViBE::uint32 i = 0 ; i< pClassificationValueList.size() ; ++i){
		std::cout << pClassificationValueList[i].m_f64FirstClass << " " << pClassificationValueList[i].m_f64SecondClass << std::endl;
		std::cout << pClassificationValueList[i].m_f64ClassLabel;
		std::cout << std::endl;
	}
#endif

	uint32* l_pWinCount = new uint32[m_ui32ClassCount];
	for(size_t i = 0 ; i < m_ui32ClassCount ; ++i)
	{
		l_pWinCount[i] = 0;
	}

	for(uint32 i =0 ; i < pClassificationValueList.size() ; ++i)
	{
		SClassificationInfo & l_rTemp = pClassificationValueList[i];
		if(l_rTemp.m_f64ClassLabel == 1)
		{
			++(l_pWinCount[(uint32)(l_rTemp.m_f64FirstClass-1)]);
		}
		else
		{
			++(l_pWinCount[(uint32)(l_rTemp.m_f64SecondClass-1)]);
		}

	}

#if VOTING_DEBUG
	for(size_t i = 0; i < m_ui32ClassCount ;  ++i)
	{
		std::cout << ((float64)l_pWinCount[i])/pClassificationValueList.size() <<  " ";
	}
	std::cout << std::endl;
#endif

	pProbabilityVector->setDimensionCount(1);
	pProbabilityVector->setDimensionSize(0,m_ui32ClassCount);

	for(OpenViBE::uint32 i = 0 ; i<m_ui32ClassCount ; ++i)
	{
		pProbabilityVector->getBuffer()[i] = ((float64)l_pWinCount[i])/pClassificationValueList.size();
	}

	delete[] l_pWinCount;
	return true;
}

XML::IXMLNode* CAlgorithmPairwiseDecisionVoting::saveConfiguration()
{
	XML::IXMLNode* l_pRootNode = XML::createNode(c_sTypeNodeName);
	return l_pRootNode;
}

boolean CAlgorithmPairwiseDecisionVoting::loadConfiguration(XML::IXMLNode& rNode)
{
	return true;
}
