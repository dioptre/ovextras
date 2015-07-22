#include "ovpCAlgorithmClassifierMLP.h"

#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

#include <system/ovCMemory.h>
#include <xml/IXMLHandler.h>

#include <Eigen/Eigenvalues>
#include <Eigen/Dense>
#include <Eigen/Core>

//Need to be reachable from outside
const char* const c_sMLPEvaluationFunctionName = "Evaluation function";
const char* const c_sMLPTransfertFunctionName = "Transfert function";

namespace{
	const char* const c_sMLPTypeNodeName = "MLP";
	const char* const c_sMLPNeuronConfigurationNodeName = "Neuron-configuration";
	const char* const c_sMLPInputNeuronCountNodeName = "Input-neuron-count";
	const char* const c_sMLPHiddenNeuronCountNodeName = "Hidden-neuron-count";
	const char* const c_sMLPMaximumNodeName= "Maximum";
	const char* const c_sMLPMinimumNodeName = "Minimum";
	const char* const c_sMLPInputBiasNodeName = "Input-bias";
	const char* const c_sMLPInputWeightNodeName = "Input-weight";
	const char* const c_sMLPHiddenBiasNodeName = "Hidden-bias";
	const char* const c_sMLPHiddenWeightNodeName = "Hidden-weight";
	const char* const c_sMLPClassLabelNodeName = "Class-label";
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



#define MLP_DEBUG 0
#if MLP_DEBUG
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
#else
void dumpMatrix(OpenViBE::Kernel::ILogManager &, const MatrixXd &, const CString &) { };
#endif


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
	uint32 l_ui64HiddenNeuronCount = this->getInt64Parameter(OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount);
	this->uninitializeExtraParameterMechanism();

	if(l_ui64HiddenNeuronCount < 1)
	{
		this->getLogManager() << LogLevel_Error << "Invalid amount of neuron in the hidden layer. Fallback to default value (3)\n";
		l_ui64HiddenNeuronCount = 3;
	}

	m_f64Min = rFeatureVectorSet.getFeatureVector(0).getBuffer()[0];
	m_f64Max = m_f64Min;
	std::map < float64, uint32 > l_vClassLabels;
	std::map < float64, VectorXd > l_oClassGoal;
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

	//We generate the list of class
	for(std::map < float64, uint32 >::iterator iter = l_vClassLabels.begin() ; iter != l_vClassLabels.end() ; ++iter)
	{
		m_oLabelList.push_back(iter->first);
		iter->second *= 0.2;
	}

	const float64 l_f64Alpha = 0.01;
	const float64 l_f64Epsilon=0.000001;
	const uint32 l_ui32ClassCount = m_oLabelList.size();
	const uint64 l_ui64FeatureSize = rFeatureVectorSet.getFeatureVector(0).getSize();

	//Generate the target vector for each class
	for(size_t i=0; i < l_ui32ClassCount; ++i)
	{
		VectorXd l_oGoal = VectorXd::Zero(l_ui32ClassCount);
		//class 1 is at index 0
		l_oGoal[(uint64)m_oLabelList[i] -1 ] = 1.;
		l_oClassGoal[m_oLabelList[i]] = l_oGoal;
	}

	//We store each normalize vector we get for training. This not optimal in memory but avoid a lot of computation later
	std::vector <CEigenFeatureVector> m_oTrainingSet;
	std::vector <CEigenFeatureVector> m_oValidationSet;

	//We don't need to make a shuffle it has already be made by the trainer box
	//We store 20% of the full feature vector set for validation part of the training process
	for(size_t i=0; i < rFeatureVectorSet.getFeatureVectorCount(); ++i)
	{
		if(l_vClassLabels[rFeatureVectorSet.getFeatureVector(i).getLabel()] > 0)
		{
			const Map<VectorXd> l_oFeatureVec(const_cast<float64*>(rFeatureVectorSet.getFeatureVector(i).getBuffer()), l_ui64FeatureSize);
			VectorXd l_oData = l_oFeatureVec;
			for(size_t j =0; j < l_ui64FeatureSize; ++j)
			{
				//Normalization
				l_oData[j] = 2 * (l_oData[j] - m_f64Min)/ (m_f64Max - m_f64Min) - 1;
			}
			m_oValidationSet.push_back(CEigenFeatureVector(rFeatureVectorSet.getFeatureVector(i).getLabel(), l_oData));
			--l_vClassLabels[rFeatureVectorSet.getFeatureVector(i).getLabel()];
		}
		else{
			const Map<VectorXd> l_oFeatureVec(const_cast<float64*>(rFeatureVectorSet.getFeatureVector(i).getBuffer()), l_ui64FeatureSize);
			VectorXd l_oData = l_oFeatureVec;
			for(size_t j =0; j < l_ui64FeatureSize; ++j)
			{
				//Normalization
				l_oData[j] = 2 * (l_oData[j] - m_f64Min)/ (m_f64Max - m_f64Min) - 1;
			}
			m_oTrainingSet.push_back(CEigenFeatureVector(rFeatureVectorSet.getFeatureVector(i).getLabel(), l_oData));
		}
	}

	const uint64 l_ui64FeatureCount = m_oTrainingSet.size();
	const float64 l_f64BoundValue = 1./(l_ui64FeatureSize+1);
	float64 l_f64PreviousError = std::numeric_limits<float64>::max();
	float64 l_f64CumulativeError = 0;

	//Let's generate randomly weights and biases
	m_oInputWeight = MatrixXd::Random(l_ui64HiddenNeuronCount, l_ui64FeatureSize);
	m_oInputBias = VectorXd::Random(l_ui64HiddenNeuronCount);

	m_oHiddenWeight = MatrixXd::Random(l_ui32ClassCount, l_ui64HiddenNeuronCount);
	m_oHiddenBias = VectorXd::Random(l_ui32ClassCount);

	//Let's restrain the weight between -1/(fan-in) and 1/(fan-in) to avoid saturation in the worst case
	m_oInputWeight*= l_f64BoundValue;
	m_oInputBias*= l_f64BoundValue;
	m_oHiddenWeight*= l_f64BoundValue;
	m_oHiddenBias*= l_f64BoundValue;

	MatrixXd l_oDeltaInputWeight = MatrixXd::Zero(l_ui64HiddenNeuronCount, l_ui64FeatureSize);
	VectorXd l_oDeltaInputBias = VectorXd::Zero(l_ui64HiddenNeuronCount);
	MatrixXd l_oDeltaHiddenWeight = MatrixXd::Zero(l_ui32ClassCount, l_ui64HiddenNeuronCount);
	VectorXd l_oDeltaHiddenBias = VectorXd::Zero(l_ui32ClassCount);

	VectorXd m_oA1, m_oY1, m_oA2;
	//A1 is the value compute in hidden neuron before applying tanh
	//Y1 is the output vector of hidden layer
	//A2 is the value compute by output neuron before applying transfert function
	//Y2 is the value of output after the transfert function (softmax)
	for(size_t l_uiTrainingIteration = 0 ; l_uiTrainingIteration < 100000 ; ++l_uiTrainingIteration)
	{
		l_oDeltaInputWeight.setZero();
		l_oDeltaInputBias.setZero();
		l_oDeltaHiddenWeight.setZero();
		l_oDeltaHiddenBias.setZero();

		for(size_t i =0; i < l_ui64FeatureCount; ++i)
		{
			VectorXd& l_oGoal = l_oClassGoal[m_oTrainingSet[i].first];

			//We need to compute the output
			VectorXd& l_oData = m_oTrainingSet[i].second;

			m_oA1.noalias() = m_oInputBias + (m_oInputWeight * l_oData);
			m_oY1.noalias() = m_oA1.unaryExpr(std::ptr_fun(tanh));
			m_oA2.noalias() = m_oHiddenBias + (m_oHiddenWeight * m_oY1);
			//We don't need to compute Y2 (we use the identity for training)

			//Now we compute all deltas of output layer
			VectorXd l_oOutputDelta = m_oA2 - l_oGoal;
			for(size_t j = 0; j < l_ui32ClassCount; ++j)
			{
				for(size_t k = 0; k < (uint32)l_ui64HiddenNeuronCount; ++k)
				{
					l_oDeltaHiddenWeight(j,k) -= l_oOutputDelta[j] * m_oY1[k];
				}
			}
			l_oDeltaHiddenBias.noalias() -= l_oOutputDelta;

			//Now we take care of the hidden layer
			VectorXd l_oHiddenDelta = VectorXd::Zero(l_ui64HiddenNeuronCount);
			for(size_t j=0; j < (size_t)l_ui64HiddenNeuronCount; ++j)
			{
				for(size_t k = 0; k < l_ui32ClassCount; ++k)
				{
					l_oHiddenDelta[j] += l_oOutputDelta[k] * m_oHiddenWeight(k,j);
				}
				l_oHiddenDelta[j] *= (1 - pow(m_oY1[j], 2));
			}

			for(size_t j =0; j < (size_t)l_ui64HiddenNeuronCount; ++j)
			{
				for(size_t k =0; k < l_ui64FeatureSize; ++k)
				{
					l_oDeltaInputWeight(j, k) -= l_oHiddenDelta[j] * l_oData[k];
				}
			}
			l_oDeltaInputBias.noalias() -= l_oHiddenDelta;

		}
		//We finish the loop, let's apply deltas
		m_oHiddenWeight.noalias() += l_oDeltaHiddenWeight / l_ui64FeatureCount * l_f64Alpha;
		m_oHiddenBias.noalias() += l_oDeltaHiddenBias / l_ui64FeatureCount * l_f64Alpha;
		m_oInputWeight.noalias() += l_oDeltaInputWeight / l_ui64FeatureCount * l_f64Alpha;
		m_oInputBias.noalias() += l_oDeltaInputBias / l_ui64FeatureCount * l_f64Alpha;

		dumpMatrix(this->getLogManager(), m_oHiddenWeight, "m_oHiddenWeight");
		dumpMatrix(this->getLogManager(), m_oHiddenBias, "m_oHiddenBias");
		dumpMatrix(this->getLogManager(), m_oInputWeight, "m_oInputWeight");
		dumpMatrix(this->getLogManager(), m_oInputBias, "m_oInputBias");

		//Now we compute the cumulative error in the validation set
		l_f64CumulativeError=0;
		for(size_t i=0; i < m_oValidationSet.size(); ++i)
		{
			VectorXd& l_oGoal = l_oClassGoal[m_oValidationSet[i].first];
			VectorXd& l_oData = m_oValidationSet[i].second;
			//we normalize and center data on 0 to avoid saturation

			m_oA1.noalias() = m_oInputBias + (m_oInputWeight * l_oData);
			m_oA2.noalias() = m_oHiddenBias + (m_oHiddenWeight * m_oA1.unaryExpr(std::ptr_fun(tanh)));
			//We don't need to compute Y2

			//Now we need to compute the error
			for(size_t j =0; j < l_ui32ClassCount; ++j)
			{
				l_f64CumulativeError += 0.5 * pow(m_oA2[j] - l_oGoal[j], 2);
			}
		}
		l_f64CumulativeError /= m_oValidationSet.size();
		//If the delta of error is under Epsilon we consider that the training is over
		if(l_f64PreviousError - l_f64CumulativeError < l_f64Epsilon)
		{
			break;
		}
		l_f64PreviousError = l_f64CumulativeError;
	}
	dumpMatrix(this->getLogManager(), m_oHiddenWeight, "l_oHiddenWeight");
	dumpMatrix(this->getLogManager(), m_oHiddenBias, "l_oHiddenBias");
	dumpMatrix(this->getLogManager(), m_oInputWeight, "l_oInputWeight");
	dumpMatrix(this->getLogManager(), m_oInputBias, "l_oInputBias");
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

	const uint32 l_ui32ClassCount = m_oLabelList.size();
	const uint32 l_ui32HiddenNeuronCount = (int64)m_oInputWeight.rows();

	VectorXd m_oA1 = m_oInputBias + (m_oInputWeight * l_oData);

	VectorXd m_oY1(l_ui32HiddenNeuronCount);
	for(size_t j = 0; j < l_ui32HiddenNeuronCount; ++j)
	{
		m_oY1[j] = tanh(m_oA1[j]);
	}

	VectorXd m_oA2 = m_oHiddenBias + (m_oHiddenWeight * m_oY1);
	VectorXd m_oY2(l_ui32ClassCount);

	//The final transfert function is the softmax
	for(size_t j = 0; j< l_ui32ClassCount ; ++j)
	{
		m_oY2[j] = exp(m_oA2[j]);
	}
	m_oY2 /= m_oY2.sum();

	rDistanceValue.setSize(l_ui32ClassCount);
	rProbabilityValue.setSize(l_ui32ClassCount);

	//We use A2 as the classification values output, and the Y2 as the probability
	float64 l_f64Max = m_oY2[0];
	uint32 l_ui32ClassFound = 0;
	rDistanceValue[0] = m_oA2[0];
	rProbabilityValue[0] = m_oY2[0];
	for(size_t i = 1; i< l_ui32ClassCount; ++i)
	{
		if(m_oY2[i] > l_f64Max)
		{
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
	for(size_t i = 0; i< (uint32)m_oHiddenBias.size() ; ++i)
	{
		l_sClasses << m_oLabelList[i] << " ";
	}
	XML::IXMLNode* l_pClassLabelNode = XML::createNode(c_sMLPClassLabelNodeName);
	l_pClassLabelNode->setPCData(l_sClasses.str().c_str());
	l_pRootNode->addChild(l_pClassLabelNode);

	XML::IXMLNode* l_pConfiguration = XML::createNode(c_sMLPNeuronConfigurationNodeName);

	//The input and output neuron count are not mandatory but they facilitate a lot the loading process
	l_pTempNode = XML::createNode(c_sMLPInputNeuronCountNodeName);
	dumpData(l_pTempNode, (int64)m_oInputWeight.cols());
	l_pConfiguration->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sMLPHiddenNeuronCountNodeName);
	dumpData(l_pTempNode, (int64)m_oInputWeight.rows());
	l_pConfiguration->addChild(l_pTempNode);
	l_pRootNode->addChild(l_pConfiguration);

	l_pTempNode = XML::createNode(c_sMLPMinimumNodeName);
	dumpData(l_pTempNode, m_f64Min);
	l_pRootNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sMLPMaximumNodeName);
	dumpData(l_pTempNode, m_f64Max);
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

	int64 l_i64FeatureSize, l_i64HiddenNeuronCount;
	XML::IXMLNode* l_pNeuronConfigurationNode = pConfigurationNode->getChildByName(c_sMLPNeuronConfigurationNodeName);

	loadData(l_pNeuronConfigurationNode->getChildByName(c_sMLPHiddenNeuronCountNodeName), l_i64HiddenNeuronCount);
	loadData(l_pNeuronConfigurationNode->getChildByName(c_sMLPInputNeuronCountNodeName), l_i64FeatureSize);

	loadData(pConfigurationNode->getChildByName(c_sMLPMaximumNodeName), m_f64Max);
	loadData(pConfigurationNode->getChildByName(c_sMLPMinimumNodeName), m_f64Min);

	loadData(pConfigurationNode->getChildByName(c_sMLPInputWeightNodeName),m_oInputWeight, l_i64HiddenNeuronCount, l_i64FeatureSize);
	loadData(pConfigurationNode->getChildByName(c_sMLPInputBiasNodeName), m_oInputBias);
	loadData(pConfigurationNode->getChildByName(c_sMLPHiddenWeightNodeName),m_oHiddenWeight, m_oLabelList.size(), l_i64HiddenNeuronCount);
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

void CAlgorithmClassifierMLP::loadData(XML::IXMLNode *pNode, MatrixXd &rMatrix, OpenViBE::int64 ui32RowCount, OpenViBE::int64 ui32ColCount)
{
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
	std::stringstream l_sData(pNode->getPCData());
	l_sData >> f64Value;
}
