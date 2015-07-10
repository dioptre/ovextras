#include "ovpCAlgorithmClassifierMLP.h"

//#if defined TARGET_HAS_ThirdPartyEIGEN

#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

#include <system/ovCMemory.h>
#include <xml/IXMLHandler.h>

#include <Eigen/Eigenvalues>
#include <Eigen/Dense>

//Need to be reachable from outside
const char* const c_sMLPEvaluationFunctionName = "Evaluation function";
const char* const c_sMLPTransfertFunctionName = "Transfert function";

namespace{
	const char* const c_sMLPTypeNodeName = "MLP";
	const char* const c_sMLPNeuronConfigurationNodeName = "Neuron-configuration";
	const char* const c_sMLPInputNeuronCountNodeName = "Input-neuron-count";
	const char* const c_sMLPHiddenNeuronCountNodeName = "Hidden-neuron-count";
	const char* const c_sMLPOutputNeuronCountNodeName = "Output-neuron-count";
	const char* const c_sMLPTransfertFunctionNodeName = "Transfert-function";
	const char* const c_sMLPMaximumNodeName= "Maximum";
	const char* const c_sMLPMinimumNodeName = "Minimum";
	const char* const c_sMLPInputBiasNodeName = "Input-bias";
	const char* const c_sMLPInputWeightNodeName = "Input-weight";
	const char* const c_sMLPHiddenBiasNodeName = "Hidden-bias";
	const char* const c_sMLPHiddenWeightNodeName = "Hidden-weight";
	const char* const c_sMLPClassLabelNodeName = "Class-label";
	const char* const c_sMLPIdentifierAttributeName = "id";
}

OpenViBE::int32 OpenViBEPlugins::Classification::MLPClassificationCompare(OpenViBE::IMatrix& rFirstClassificationValue, OpenViBE::IMatrix& rSecondClassificationValue)
{
	//We first need to find the best classification of each.
	OpenViBE::float64* l_pClassificationValueBuffer = rFirstClassificationValue.getBuffer();
	OpenViBE::float64 l_f64MaxFirst = *(std::max_element(l_pClassificationValueBuffer, l_pClassificationValueBuffer+rFirstClassificationValue.getBufferElementCount()));

	l_pClassificationValueBuffer = rSecondClassificationValue.getBuffer();
	OpenViBE::float64 l_f64MaxSecond = *(std::max_element(l_pClassificationValueBuffer, l_pClassificationValueBuffer+rSecondClassificationValue.getBufferElementCount()));

	//Then we just compared them
	if(!ov_float_equal(l_f64MaxFirst, l_f64MaxSecond))
	{
		return 0;
	}
	else if(l_f64MaxFirst > l_f64MaxSecond)
	{
		return -1;
	}
	return 1;
}


using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

using namespace Eigen;

void dumpMatrix(OpenViBE::Kernel::ILogManager &rMgr, const MatrixXd &mat, const CString &desc)
{
	rMgr << LogLevel_Info << desc << "\n";
	for(int i=0;i<mat.rows();i++) {
		rMgr << LogLevel_Info << "Row " << i << ": ";
		for(int j=0;j<mat.cols();j++) {
			rMgr << mat(i,j) << " ";
		}
		rMgr << "\n";
	}
}


boolean CAlgorithmClassifierMLP::initialize()
{
	TParameterHandler < int64 > ip_i64Hidden(this->getInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount));
	ip_i64Hidden = 3;

	TParameterHandler < XML::IXMLNode* > op_pConfiguration(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	op_pConfiguration=NULL;


	return true;
}

boolean CAlgorithmClassifierMLP::uninitialize()
{
	return true;
}

boolean CAlgorithmClassifierMLP::train(const IFeatureVectorSet &rFeatureVectorSet)
{
	m_oLabelList.clear();

	this->initializeExtraParameterMechanism();
	m_i64HiddenNeuronCount = this->getInt64Parameter(OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount);
	m_oEvaluationFunctionIdentifier = this->getEnumerationParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_EvaluationFunction, OVP_TypeId_Enumeration_EvaluationFunction);
	m_oTransfertFunctionIdentifier = this->getEnumerationParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_TransfertFunction, OVP_TypeId_Enumeration_TransfertFunction);
	this->uninitializeExtraParameterMechanism();

	std::cout << m_oTransfertFunctionIdentifier.toString() << std::endl;
	std::cout << m_oEvaluationFunctionIdentifier.toString() << std::endl;

	if(m_i64HiddenNeuronCount < 1)
	{
		this->getLogManager() << LogLevel_Error << "Invalid amount of neuron in the hidden layer. Fallback to default value (3)\n";
		m_i64HiddenNeuronCount = 3;
	}

	m_f64Min = rFeatureVectorSet.getFeatureVector(0).getBuffer()[0];
	m_f64Max = rFeatureVectorSet.getFeatureVector(0).getBuffer()[0];

	std::map < float64, uint32 > l_vClassLabels;
	for(uint32 i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		l_vClassLabels[rFeatureVectorSet[i].getLabel()]++;
		for(size_t j = 0; j < rFeatureVectorSet[i].getSize(); ++j)
		{
			const float64 l_f64Value = rFeatureVectorSet.getFeatureVector(i).getBuffer()[j];
			if(m_f64Max < l_f64Value)
			{
				m_f64Max = l_f64Value;
			}
			else if(m_f64Min > l_f64Value)
			{
				m_f64Min = l_f64Value;
			}
		}
	}
	for(std::map < float64, uint32 >::iterator iter = l_vClassLabels.begin() ; iter != l_vClassLabels.end() ; ++iter)
	{
		m_oLabelList.push_back(iter->first);
	}
	std::cout << "Amount of class" << std::endl;

	const uint32 l_ui32ClassCount = m_oLabelList.size();
	const float64 l_f64Alpha = 0.1;
	const uint64 l_ui64FeatureSize = rFeatureVectorSet.getFeatureVector(0).getSize();
	const uint64 l_ui64FeatureCount = rFeatureVectorSet.getFeatureVectorCount();

	m_oInputWeight = MatrixXd::Random(m_i64HiddenNeuronCount, l_ui64FeatureSize);
	m_oInputBias = VectorXd::Random(m_i64HiddenNeuronCount);

	m_oHiddenWeight = MatrixXd::Random(l_ui32ClassCount, m_i64HiddenNeuronCount);
	m_oHiddenBias = VectorXd::Random(l_ui32ClassCount);

	const float64 l_f64BoundValue = 1./(l_ui64FeatureSize+1);
	std::cout << l_f64BoundValue << std::endl;

	//Let's restrain the weight between -1/(fan-in) and 1/(fin-in) to avoid saturation in the worst case
	m_oInputWeight*= l_f64BoundValue;
	m_oInputBias*= l_f64BoundValue;
	m_oHiddenWeight*= l_f64BoundValue;
	m_oHiddenBias*= l_f64BoundValue;



	dumpMatrix(this->getLogManager(), m_oHiddenWeight, "l_oHiddenWeight");
	dumpMatrix(this->getLogManager(), m_oHiddenBias, "l_oHiddenBias");
	dumpMatrix(this->getLogManager(), m_oInputWeight, "l_oInputWeight");
	dumpMatrix(this->getLogManager(), m_oInputBias, "l_oInputBias");

	for(size_t l_uiTrainingIteration = 0 ; l_uiTrainingIteration < 1000 ; ++l_uiTrainingIteration)
	{
		MatrixXd l_oDeltaInputWeight = MatrixXd::Zero(m_i64HiddenNeuronCount, l_ui64FeatureSize);
		VectorXd l_oDeltaInputBias = VectorXd::Zero(m_i64HiddenNeuronCount);
		MatrixXd l_oDeltaHiddenWeight = MatrixXd::Zero(l_ui32ClassCount, m_i64HiddenNeuronCount);
		VectorXd l_oDeltaHiddenBias = VectorXd::Zero(l_ui32ClassCount);

		float64 l_f64CumulativeError = 0;

		//std::cout << "Start real work" << std::endl;
		for(size_t i =0; i < l_ui64FeatureCount; ++i)
		{
			VectorXd l_oGoal = VectorXd::Zero(l_ui32ClassCount);
			//class 1 is at index 0
			//std::cout << "Feature Vector : " << rFeatureVectorSet.getFeatureVector(i).getLabel()<< std::endl;
			l_oGoal[(uint64)rFeatureVectorSet.getFeatureVector(i).getLabel() -1 ] = 1.;
			//dumpMatrix(this->getLogManager(), l_oGoal, "Goal");
			//std::cout << "Goal create" << std::endl;
			//We need to compute the output
			const Map<VectorXd> l_oFeatureVec(const_cast<float64*>(rFeatureVectorSet.getFeatureVector(i).getBuffer()), l_ui64FeatureSize);
			VectorXd l_oData = l_oFeatureVec;
			//we normalize and center data on 0 to avoid saturation
			for(size_t j =0; j < l_ui64FeatureSize; ++j)
			{
				l_oData[j] = 2 * (l_oData[j] - m_f64Min)/ (m_f64Max - m_f64Min) - 1;
			}
			//dumpMatrix(this->getLogManager(), l_oData, "Feature");
			//std::cout << "Compute Output" << std::endl;
			VectorXd m_oA1 = m_oInputBias + (m_oInputWeight * l_oData);
			//dumpMatrix(this->getLogManager(), m_oA1, "m_oA1");
			VectorXd m_oY1(m_i64HiddenNeuronCount);
			for(size_t j = 0; j < m_i64HiddenNeuronCount; ++j)
			{
				m_oY1[j] = tanh(m_oA1[j]);
			}
			//dumpMatrix(this->getLogManager(), m_oY1, "m_oY1");
			VectorXd m_oA2 = m_oHiddenBias + (m_oHiddenWeight * m_oY1);
			VectorXd m_oY2(l_ui32ClassCount);

			//std::cout <<"Let's apply tranfert function" << std::endl;
			if(m_oTransfertFunctionIdentifier == OVP_Algorithm_ClassifierMLP_Enumeration_TransfertFunction_Softmax)
			{
				for(size_t j = 0; j< l_ui32ClassCount ; ++j)
				{
					m_oY2[j] = exp(m_oA2[j]);
				}
				m_oY2 /= m_oY2.sum();
			}
			else if(m_oTransfertFunctionIdentifier == OVP_Algorithm_ClassifierMLP_Enumeration_TransfertFunction_Sigmoid)
			{
				for(size_t j = 0 ; j < l_ui32ClassCount ; ++j)
				{
					float64 l_f64ExpSum = 0.;
					for(size_t k = 0 ; k < l_ui32ClassCount ; ++k)
					{
						l_f64ExpSum += exp(m_oA2[k] - m_oA2[j]);
					}
					m_oY2[j] = 1/l_f64ExpSum;
				}
			}
			//dumpMatrix(this->getLogManager(), m_oA2, "Output");
			//std::cout << "let's apply evaluation" << std::endl;
			//Now we need to compute the error
			float64 l_f64Error = 0;
			if(m_oEvaluationFunctionIdentifier == OVP_Algorithm_ClassifierMLP_Enumeration_EvaluationFunction_Quadratic)
			{
				for(size_t j =0; j < l_ui32ClassCount; ++j)
				{
					l_f64Error += 0.5 * pow(m_oY2[j] - l_oGoal[j], 2);
				}
			}
			else if(m_oEvaluationFunctionIdentifier == OVP_Algorithm_ClassifierMLP_Enumeration_EvaluationFunction_MisClassification)
			{
				float64 l_f64Max = m_oY2[0];
				uint32 l_ui32ClassFound = 1;
				for(size_t j = 1; j< l_ui32ClassCount; ++j)
				{
					if(m_oY2[j] > l_f64Max)
					{
						l_f64Max = m_oY2[j];
						l_ui32ClassFound = j+1;
					}
				}
				if(l_ui32ClassFound != (uint32)rFeatureVectorSet.getFeatureVector(i).getLabel())
				{
					l_f64Error = 1;
				}
			}
			l_f64CumulativeError += l_f64Error;
			//dumpMatrix(this->getLogManager(), m_oY2, "m_oY2");
			//std::cout << "Error : " << l_f64Error << std::endl;

			//std::cout << "Compute delta" << std::endl;
			//Now we compute all deltas of output layer
			VectorXd l_oOutputDelta = m_oA2 - l_oGoal;

			for(size_t j = 0; j < l_ui32ClassCount; ++j)
			{
				for(size_t k = 0; k < (uint32)m_i64HiddenNeuronCount; ++k)
				{
					l_oDeltaHiddenWeight(j,k) -= l_f64Alpha * l_oOutputDelta[j] * m_oY1[k];
				}
				l_oDeltaHiddenBias[j] -= l_f64Alpha * l_oOutputDelta[j];
			}
			//dumpMatrix(this->getLogManager(), l_oDeltaHiddenWeight, "l_oDeltaHiddenWeight");

			//Now we take care of the hidden layer
			VectorXd l_oHiddenDelta = VectorXd::Zero(m_i64HiddenNeuronCount);
			for(size_t j=0; j < (size_t)m_i64HiddenNeuronCount; ++j)
			{
				for(size_t k = 0; k < l_ui32ClassCount; ++k)
				{
					l_oHiddenDelta[j] += l_oOutputDelta[k] * m_oHiddenWeight(k,j);
				}
				l_oHiddenDelta[j] *= (1 - pow(m_oY1[j], 2));
			}
			//dumpMatrix(this->getLogManager(), l_oHiddenDelta, "l_oHiddenDelta");

			for(size_t j =0; j < (size_t)m_i64HiddenNeuronCount; ++j)
			{
				for(size_t k =0; k < l_ui64FeatureSize; ++k)
				{
					l_oDeltaInputWeight(j, k) -= l_f64Alpha * l_oHiddenDelta[j] * l_oData[k];
				}
				l_oDeltaInputBias[j] -= l_f64Alpha * l_oHiddenDelta[j];
			}

		}

		//dumpMatrix(this->getLogManager(), l_oDeltaHiddenWeight, "l_oDeltaHiddenWeight");
//		dumpMatrix(this->getLogManager(), l_oDeltaHiddenBias, "l_oDeltaHiddenBias");
//		dumpMatrix(this->getLogManager(), l_oDeltaInputWeight, "l_oDeltaInputWeight");
//		dumpMatrix(this->getLogManager(), l_oDeltaInputBias, "l_oDeltaInputBias");

		//We finish the loop, let's apply deltas and restart
		m_oHiddenWeight += (l_oDeltaHiddenWeight / l_ui64FeatureCount);
		m_oHiddenBias += (l_oDeltaHiddenBias / l_ui64FeatureCount);
		m_oInputWeight += (l_oDeltaInputWeight / l_ui64FeatureCount);
		m_oInputBias += (l_oDeltaInputBias / l_ui64FeatureCount);

//		dumpMatrix(this->getLogManager(), l_oHiddenWeight, "l_oHiddenWeight");
//		dumpMatrix(this->getLogManager(), l_oHiddenBias, "l_oHiddenBias");
//		dumpMatrix(this->getLogManager(), l_oInputWeight, "l_oInputWeight");
//		dumpMatrix(this->getLogManager(), l_oInputBias, "l_oInputBias");

	}
			dumpMatrix(this->getLogManager(), m_oHiddenWeight, "l_oHiddenWeight");
			dumpMatrix(this->getLogManager(), m_oHiddenBias, "l_oHiddenBias");
			dumpMatrix(this->getLogManager(), m_oInputWeight, "l_oInputWeight");
			dumpMatrix(this->getLogManager(), m_oInputBias, "l_oInputBias");
	//exit(0);
	return true;
}

boolean CAlgorithmClassifierMLP::classify(const IFeatureVector &rFeatureVector, float64 &rf64Class, IVector &rDistanceValue, IVector &rProbabilityValue)
{
	const Map<VectorXd> l_oFeatureVec(const_cast<float64*>(rFeatureVector.getBuffer()), rFeatureVector.getSize());
	VectorXd l_oData = l_oFeatureVec;
	//we normalize and center data on 0 to avoid saturation
	for(size_t j =0; j < rFeatureVector.getSize(); ++j)
	{
		l_oData[j] = 2 * (l_oData[j] - m_f64Min)/ (m_f64Max - m_f64Min) - 1;
	}
	//dumpMatrix(this->getLogManager(), l_oData, "l_oData");
	const uint32 l_ui32ClassCount = m_oHiddenBias.size();
	VectorXd m_oA1 = m_oInputBias + (m_oInputWeight * l_oData);
	//dumpMatrix(this->getLogManager(), m_oA1, "m_oA1");
	VectorXd m_oY1(m_i64HiddenNeuronCount);
	for(size_t j = 0; j < m_i64HiddenNeuronCount; ++j)
	{
		m_oY1[j] = tanh(m_oA1[j]);
	}
	//dumpMatrix(this->getLogManager(), m_oY1, "m_oY1");
	VectorXd m_oA2 = m_oHiddenBias + (m_oHiddenWeight * m_oY1);
	VectorXd m_oY2(l_ui32ClassCount);

	if(m_oTransfertFunctionIdentifier == OVP_Algorithm_ClassifierMLP_Enumeration_TransfertFunction_Softmax)
	{
		for(size_t j = 0; j< l_ui32ClassCount ; ++j)
		{
			m_oY2[j] = exp(m_oA2[j]);
		}
		m_oY2 /= m_oY2.sum();
	}
	else if(m_oTransfertFunctionIdentifier == OVP_Algorithm_ClassifierMLP_Enumeration_TransfertFunction_Sigmoid)
	{
		for(size_t j = 0 ; j < l_ui32ClassCount ; ++j)
		{
			float64 l_f64ExpSum = 0.;
			for(size_t k = 0 ; k < l_ui32ClassCount ; ++k)
			{
				l_f64ExpSum += exp(m_oA2[k] - m_oA2[j]);
			}
			m_oY2[j] = 1/l_f64ExpSum;
		}
	}
	//dumpMatrix(this->getLogManager(), m_oY2, "m_oY2");
	rDistanceValue.setSize(l_ui32ClassCount);
	rProbabilityValue.setSize(l_ui32ClassCount);

	float64 l_f64Max = m_oY2[0];
	uint32 l_ui32ClassFound = 0;
	rDistanceValue[0] = m_oA2[0];
	rProbabilityValue[0] = m_oY2[0];
	for(size_t i = 1; i< l_ui32ClassCount; ++i)
	{
		//std::cout << "Is " << m_oY2[i] << " greater than " << l_f64Max << std::endl;
		if(m_oY2[i] > l_f64Max)
		{
			//std::cout << "Yes" << std::endl;
			l_f64Max = m_oY2[i];
			l_ui32ClassFound = i;
		}
		rDistanceValue[i] = m_oA2[i];
		rProbabilityValue[i] = m_oY2[i];
	}

	rf64Class = m_oLabelList[l_ui32ClassFound];

	return true;
}



XML::IXMLNode *CAlgorithmClassifierMLP::saveConfiguration()
{
	XML::IXMLNode* l_pTempNode;
	XML::IXMLNode* l_pRootNode = XML::createNode(c_sMLPTypeNodeName);


	std::stringstream l_sClasses;
	for(size_t i = 0; i< m_oHiddenBias.size() ; ++i)
	{
		l_sClasses << m_oLabelList[i] << " ";
	}
	XML::IXMLNode* l_pClassLabelNode = XML::createNode(c_sMLPClassLabelNodeName);
	l_pClassLabelNode->setPCData(l_sClasses.str().c_str());
	l_pRootNode->addChild(l_pClassLabelNode);

	XML::IXMLNode* l_pConfiguration = XML::createNode(c_sMLPNeuronConfigurationNodeName);

	l_pTempNode = XML::createNode(c_sMLPInputNeuronCountNodeName);
	dumpData(l_pTempNode, (int64)m_oInputWeight.cols());
	l_pConfiguration->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sMLPHiddenNeuronCountNodeName);
	dumpData(l_pTempNode, m_i64HiddenNeuronCount);
	l_pConfiguration->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sMLPOutputNeuronCountNodeName);
	dumpData(l_pTempNode, (int64)m_oHiddenBias.size());
	l_pConfiguration->addChild(l_pTempNode);
	l_pRootNode->addChild(l_pConfiguration);

	l_pTempNode = XML::createNode(c_sMLPMinimumNodeName);
	dumpData(l_pTempNode, m_f64Min);
	l_pRootNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sMLPMaximumNodeName);
	dumpData(l_pTempNode, m_f64Max);
	l_pRootNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sMLPTransfertFunctionNodeName);
	dumpData(l_pTempNode, m_oTransfertFunctionIdentifier, OVP_TypeId_Enumeration_TransfertFunction);
	l_pRootNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sMLPInputWeightNodeName);
	dumpData(l_pTempNode, m_oInputWeight);
	l_pRootNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sMLPInputBiasNodeName);
	dumpData(l_pTempNode, m_oInputBias);
	l_pRootNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sMLPHiddenBiasNodeName);
	dumpData(l_pTempNode, m_oHiddenBias);
	l_pRootNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sMLPHiddenWeightNodeName);
	dumpData(l_pTempNode, m_oHiddenWeight);
	l_pRootNode->addChild(l_pTempNode);

	return l_pRootNode;
}

boolean CAlgorithmClassifierMLP::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	m_oLabelList.clear();
	std::stringstream l_sData(pConfigurationNode->getChildByName(c_sMLPClassLabelNodeName)->getPCData());
	float64 l_f64Temp;
	while(l_sData >> l_f64Temp)
	{
		m_oLabelList.push_back(l_f64Temp);
	}

	XML::IXMLNode* l_pNeuronConfigurationNode = pConfigurationNode->getChildByName(c_sMLPNeuronConfigurationNodeName);
	loadData(l_pNeuronConfigurationNode->getChildByName(c_sMLPHiddenNeuronCountNodeName), m_i64HiddenNeuronCount);
	int64 l_i64FeatureSize, l_i64ClassCount;
	loadData(l_pNeuronConfigurationNode->getChildByName(c_sMLPInputNeuronCountNodeName), l_i64FeatureSize);
	loadData(l_pNeuronConfigurationNode->getChildByName(c_sMLPOutputNeuronCountNodeName), l_i64ClassCount);

	loadData(pConfigurationNode->getChildByName(c_sMLPTransfertFunctionNodeName), m_oTransfertFunctionIdentifier);

	loadData(pConfigurationNode->getChildByName(c_sMLPInputWeightNodeName),m_oInputWeight, m_i64HiddenNeuronCount, l_i64FeatureSize);
	loadData(pConfigurationNode->getChildByName(c_sMLPInputBiasNodeName), m_oInputBias);
	loadData(pConfigurationNode->getChildByName(c_sMLPHiddenWeightNodeName),m_oHiddenWeight, l_i64ClassCount, m_i64HiddenNeuronCount);
	loadData(pConfigurationNode->getChildByName(c_sMLPHiddenBiasNodeName), m_oHiddenBias);

	return true;
}

void CAlgorithmClassifierMLP::dumpData(XML::IXMLNode *pNode, MatrixXd &rMatrix)
{
	std::stringstream l_sData;

	l_sData << std::scientific;
	for(size_t i=0; i< (size_t)rMatrix.rows(); i++)
	{
		for(size_t j =0; j < (size_t)rMatrix.cols(); ++j)
		{
			l_sData << " " << rMatrix(i,j);
		}
	}

	pNode->setPCData(l_sData.str().c_str());
}

void CAlgorithmClassifierMLP::dumpData(XML::IXMLNode *pNode, VectorXd &rVector)
{
	std::stringstream l_sData;

	l_sData << std::scientific;
	for(size_t i=0; i< (size_t)rVector.size(); i++)
	{
		l_sData << " " << rVector[i];
	}

	pNode->setPCData(l_sData.str().c_str());
}

void CAlgorithmClassifierMLP::dumpData(XML::IXMLNode *pNode, int64 i64Value)
{
	std::stringstream l_sData;
	l_sData << i64Value;
	pNode->setPCData(l_sData.str().c_str());
}

void CAlgorithmClassifierMLP::dumpData(XML::IXMLNode *pNode, float64 f64Value)
{
	std::stringstream l_sData;
	l_sData << std::scientific;
	l_sData << f64Value;
	pNode->setPCData(l_sData.str().c_str());
}

void CAlgorithmClassifierMLP::dumpData(XML::IXMLNode *pNode, CIdentifier &rIdentifier, const CIdentifier &rEnumerationIdentifier)
{
	pNode->addAttribute(c_sMLPIdentifierAttributeName,rIdentifier.toString().toASCIIString());
	pNode->setPCData(this->getTypeManager().getEnumerationEntryNameFromValue(rEnumerationIdentifier, rIdentifier.toUInteger()).toASCIIString());
}

void CAlgorithmClassifierMLP::loadData(XML::IXMLNode *pNode, MatrixXd &rMatrix, OpenViBE::int64 ui32RowCount, OpenViBE::int64 ui32ColCount)
{
	//std::cout << "Load matrix" << ui32RowCount << ui32ColCount << std::endl;
	rMatrix = MatrixXd(ui32RowCount, ui32ColCount);
	std::stringstream l_sData(pNode->getPCData());

	std::vector < float64 > l_vCoefficients;
	float64 l_f64Value;
	while(l_sData >> l_f64Value)
	{
		l_vCoefficients.push_back(l_f64Value);
	}

	size_t index=0;
	for(size_t i =0; i < ui32RowCount; ++i)
	{
		for(size_t j=0; j < ui32ColCount; ++j)
		{
			rMatrix(i,j) = l_vCoefficients[index];
			++index;
		}

	}
}

void CAlgorithmClassifierMLP::loadData(XML::IXMLNode *pNode, VectorXd &rVector)
{
	std::stringstream l_sData(pNode->getPCData());
	std::vector < float64 > l_vCoefficients;
	float64 l_f64Value;
	while(l_sData >> l_f64Value)
	{
		l_vCoefficients.push_back(l_f64Value);
	}
	rVector = VectorXd(l_vCoefficients.size());

	for(size_t i=0; i < l_vCoefficients.size(); ++i)
	{
		rVector[i] = l_vCoefficients[i];
	}
}

void CAlgorithmClassifierMLP::loadData(XML::IXMLNode *pNode, int64 &i64Value)
{
	std::stringstream l_sData(pNode->getPCData());
	l_sData >> i64Value;
}

void CAlgorithmClassifierMLP::loadData(XML::IXMLNode *pNode, float64 &f64Value)
{
}

void CAlgorithmClassifierMLP::loadData(XML::IXMLNode *pNode, CIdentifier &rIdentifier)
{
	rIdentifier.fromString(pNode->getAttribute(c_sMLPIdentifierAttributeName));
}

//#endif
