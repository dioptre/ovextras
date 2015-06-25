#include "ovpCAlgorithmLDADiscriminantFunction.h"
#if defined TARGET_HAS_ThirdPartyEIGEN

#include <sstream>
#include <vector>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

using namespace Eigen;


namespace{
	const char* const c_sBaseNodeName     = "Class-config";
	const char* const c_sWeightNodeName   = "Weights";
	const char* const c_sBiasNodeName     = "Bias";
}

CAlgorithmLDADiscriminantFunction::CAlgorithmLDADiscriminantFunction():m_f64Bias(0)
{
}

void CAlgorithmLDADiscriminantFunction::setWeight(VectorXd &rWeigth)
{
	m_oWeight = rWeigth;
}

void CAlgorithmLDADiscriminantFunction::setBias(float64 f64Bias)
{
	m_f64Bias = f64Bias;
}

float64 CAlgorithmLDADiscriminantFunction::getValue(VectorXd &rFeatureVector)
{
	return (m_oWeight.transpose() * rFeatureVector)(0) + m_f64Bias;
}

uint32 CAlgorithmLDADiscriminantFunction::getWeightVectorSize()
{
	return m_oWeight.size();
}

boolean CAlgorithmLDADiscriminantFunction::loadConfiguration(XML::IXMLNode *pConfiguration)
{
	std::stringstream l_sBias(pConfiguration->getChildByName(c_sBiasNodeName)->getPCData());
	l_sBias >> m_f64Bias;

	std::stringstream l_sData(pConfiguration->getChildByName(c_sWeightNodeName)->getPCData());
	std::vector < float64 > l_vCoefficients;
	while(!l_sData.eof())
	{
		float64 l_f64Value;
		l_sData >> l_f64Value;
		l_vCoefficients.push_back(l_f64Value);
	}

	m_oWeight.resize(l_vCoefficients.size());
	for(size_t i=0; i<l_vCoefficients.size(); i++)
	{
		m_oWeight(i,0)=l_vCoefficients[i];
	}
	return true;
}

XML::IXMLNode *CAlgorithmLDADiscriminantFunction::getConfiguration(void)
{
	XML::IXMLNode* l_pRootNode = XML::createNode(c_sBaseNodeName);

	std::stringstream l_sWeigths;
	std::stringstream l_sBias;


	l_sWeigths << std::scientific;
	for(int32 i=0; i<m_oWeight.size(); i++)
	{
		l_sWeigths << " " << m_oWeight(i,0);
	}

	l_sBias << m_f64Bias;

	XML::IXMLNode* l_pTempNode = XML::createNode(c_sWeightNodeName);
	l_pTempNode->setPCData(l_sWeigths.str().c_str());
	l_pRootNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sBiasNodeName);
	l_pTempNode->setPCData(l_sBias.str().c_str());
	l_pRootNode->addChild(l_pTempNode);

	return l_pRootNode;
}

#endif
