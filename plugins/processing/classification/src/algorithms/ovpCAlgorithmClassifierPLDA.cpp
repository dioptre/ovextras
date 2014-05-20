
#include "ovpCAlgorithmClassifierPLDA.h"

#if defined TARGET_HAS_ThirdPartyITPP

#include <map>
#include <sstream>

#include <cmath>

#include <xml/IXMLNode.h>
#include <xml/IXMLHandler.h>

static const char* const c_sTypeNodeName = "PLDA";
static const char* const c_sClassesNodeName = "Classes";
static const char* const c_sInvClassNodeName = "Invariant";
static const char* const c_sCoeffNodeName = "Coefficients";
static const char* const c_sCoeffWNodeName = "CoefficientsW";
static const char* const c_sCoeffW0NodeName = "CoefficientsW0";

extern const char* const c_sClassifierRoot;

OpenViBE::int32 OpenViBEPlugins::Classification::getPLDABestClassification(OpenViBE::IMatrix& rFirstClassificationValue, OpenViBE::IMatrix& rSecondClassificationValue)
{
	if(rFirstClassificationValue[0] > rSecondClassificationValue[0])
		return -1;
	else if(rFirstClassificationValue[0] == rSecondClassificationValue[0])
		return 0;
	else
		return 1;
}

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;


boolean CAlgorithmClassifierPLDA::initialize(void)
{
	TParameterHandler < XML::IXMLNode* > op_pConfiguration(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	op_pConfiguration=NULL;

	return CAlgorithmClassifier::initialize();
}

boolean CAlgorithmClassifierPLDA::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	uint32 i;
	std::map < float64, size_t > l_vClassLabels;
	for(uint32 i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		if(!l_vClassLabels.count(rFeatureVectorSet[i].getLabel()))
		{
			l_vClassLabels[rFeatureVectorSet[i].getLabel()] = 0;
		}
		l_vClassLabels[rFeatureVectorSet[i].getLabel()]++;
	}

	if(l_vClassLabels.size() != 2)
	{
		this->getLogManager() << LogLevel_Error << "A LDA classifier can only be trained with 2 classes, not more, not less - got " << (uint32)l_vClassLabels.size() << "\n";
		return false;
	}

	uint32 l_ui32NumberOfFeatures=rFeatureVectorSet[0].getSize();
	uint32 l_ui32NumberOfFeatureVectors=rFeatureVectorSet.getFeatureVectorCount();

	m_f64Class1=l_vClassLabels.begin()->first;
	m_f64Class2=l_vClassLabels.rbegin()->first;

	itpp::vec l_oMeanFeatureVector1(l_ui32NumberOfFeatures);
	itpp::vec l_oMeanFeatureVector2(l_ui32NumberOfFeatures);

	l_oMeanFeatureVector1.zeros();
	l_oMeanFeatureVector2.zeros();

	for(i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		const IFeatureVector& l_rFeatureVector=rFeatureVectorSet[i];

		float64 l_f64Label=l_rFeatureVector.getLabel();

		if(l_f64Label==m_f64Class1)
		{
			l_oMeanFeatureVector1+=itpp::vec(l_rFeatureVector.getBuffer(), l_rFeatureVector.getSize());
		}

		if(l_f64Label==m_f64Class2)
		{
			l_oMeanFeatureVector2+=itpp::vec(l_rFeatureVector.getBuffer(), l_rFeatureVector.getSize());
		}
	}

	l_oMeanFeatureVector1/=(double)l_vClassLabels[m_f64Class1];
	l_oMeanFeatureVector2/=(double)l_vClassLabels[m_f64Class2];

	itpp::vec l_oDiff;
	itpp::mat l_oSigma(l_ui32NumberOfFeatures, l_ui32NumberOfFeatures);
	l_oSigma.zeros();

	for(i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		const IFeatureVector& l_rFeatureVector=rFeatureVectorSet[i];

		float64 l_f64Label=l_rFeatureVector.getLabel();

		if(l_f64Label==m_f64Class1)
		{
			l_oDiff=itpp::vec(l_rFeatureVector.getBuffer(), l_rFeatureVector.getSize()) - l_oMeanFeatureVector1;
			l_oSigma+=itpp::outer_product(l_oDiff, l_oDiff) / (l_ui32NumberOfFeatureVectors-2.);
		}

		if(l_f64Label==m_f64Class2)
		{
			l_oDiff=itpp::vec(l_rFeatureVector.getBuffer(), l_rFeatureVector.getSize()) - l_oMeanFeatureVector2;
			l_oSigma+=itpp::outer_product(l_oDiff, l_oDiff) / (l_ui32NumberOfFeatureVectors-2.);
		}
	}
	itpp::mat l_oSigmaInverse = itpp::inv(l_oSigma);

	m_f64w0 = -0.5 * (l_oMeanFeatureVector1.transpose() * l_oSigmaInverse * l_oMeanFeatureVector1)[0]
			+ 0.5*(l_oMeanFeatureVector2.transpose() * l_oSigmaInverse * l_oMeanFeatureVector2)[0] +
			std::log((double)l_vClassLabels[m_f64Class1]/(double)l_vClassLabels[m_f64Class2]);

	m_oW = l_oSigmaInverse * (l_oMeanFeatureVector1 - l_oMeanFeatureVector2);

	return true;
}

boolean CAlgorithmClassifierPLDA::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
{
	if(rFeatureVector.getSize()!=(unsigned int)m_oW.size())
	{
		this->getLogManager() << LogLevel_Warning << "Feature vector size " << rFeatureVector.getSize() << " and hyperplane parameter size " << (uint32) m_oW.size() << " does not match\n";
		return false;
	}

	itpp::vec l_oFeatures(rFeatureVector.getBuffer(), rFeatureVector.getSize());

	float64 l_f64a = (m_oW.transpose() * l_oFeatures)[0] + m_f64w0;
	float64 l_f64P1 = 1 / (1 + exp(-l_f64a));

	rClassificationValues.setSize(1);
	rClassificationValues[0]=l_f64P1;

	if(l_f64P1 >= 0.5)
	{
		rf64Class=m_f64Class1;
	}
	else
	{
		rf64Class=m_f64Class2;
	}
	return true;
}

void CAlgorithmClassifierPLDA::generateConfigurationNode(void)
{
	std::stringstream l_sClasses;
	std::stringstream l_sCoefficientsW;
	std::stringstream l_sCoefficientsW0;

	l_sClasses << m_f64Class1 << " " << m_f64Class2;
	l_sCoefficientsW0 << std::scientific << m_f64w0;
	l_sCoefficientsW << std::scientific << m_oW[0];

	for(int i=1; i<m_oW.size(); i++)
	{
		l_sCoefficientsW << " " << m_oW[i];
	}

	XML::IXMLNode *l_pAlgorithmNode  = XML::createNode(c_sTypeNodeName);

	XML::IXMLNode *l_pTempNode = XML::createNode(c_sClassesNodeName);
	l_pTempNode->setPCData(l_sClasses.str().c_str());
	l_pAlgorithmNode->addChild(l_pTempNode);

	//Create coeff Node
	XML::IXMLNode *l_pCoeffNode = XML::createNode(c_sCoeffNodeName);
	l_pTempNode = XML::createNode(c_sCoeffW0NodeName);
	l_pTempNode->setPCData(l_sCoefficientsW0.str().c_str());
	l_pCoeffNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sCoeffWNodeName);
	l_pTempNode->setPCData(l_sCoefficientsW.str().c_str());
	l_pCoeffNode->addChild(l_pTempNode);

	l_pAlgorithmNode->addChild(l_pCoeffNode);

	m_pConfigurationNode = XML::createNode(c_sClassifierRoot);
	m_pConfigurationNode->addChild(l_pAlgorithmNode);
}

XML::IXMLNode* CAlgorithmClassifierPLDA::saveConfiguration(void)
{
	this->generateConfigurationNode();
	return m_pConfigurationNode;
}

boolean CAlgorithmClassifierPLDA::loadConfiguration(XML::IXMLNode *pConfiguratioNode)
{
	m_f64Class1=0;
	m_f64Class2=0;

	loadClassesFromNode(pConfiguratioNode->getChild(0)->getChildByName(c_sClassesNodeName));
	loadCoefficientsFromNode(pConfiguratioNode->getChild(0)->getChildByName(c_sCoeffNodeName));

	return true;
}

void CAlgorithmClassifierPLDA::loadClassesFromNode(XML::IXMLNode *pNode)
{
	std::stringstream l_sData(pNode->getPCData());

	l_sData >> m_f64Class1;
	l_sData >> m_f64Class2;
}

void CAlgorithmClassifierPLDA::loadCoefficientsFromNode(XML::IXMLNode *pNode)
{

	std::stringstream l_sDataCoeffW0(pNode->getChildByName(c_sCoeffW0NodeName)->getPCData());
	std::stringstream l_sDataCoeffW(pNode->getChildByName(c_sCoeffWNodeName)->getPCData());

	float64 l_f64Value;

	l_sDataCoeffW0 >> m_f64w0;

	std::vector < float64 > l_vCoefficientsW;
	while(!l_sDataCoeffW.eof())
	{
		l_sDataCoeffW >> l_f64Value;
		l_vCoefficientsW.push_back(l_f64Value);
	}

	m_oW.set_size(l_vCoefficientsW.size());
	for(size_t i=0; i<l_vCoefficientsW.size(); i++)
	{
		m_oW[i]=l_vCoefficientsW[i];
	}
}

#endif // TARGET_HAS_ThirdPartyITPP
