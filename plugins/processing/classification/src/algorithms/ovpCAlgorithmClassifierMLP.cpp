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
const char* const c_sMLPTransferFunctionName = "Transfer function";

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

	TParameterHandler < float64 > ip_f64Alpha(this->getInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Alpha));
	ip_f64Alpha = 0.01;
	TParameterHandler < float64 > ip_f64Epsilon(this->getInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Epsilon));
	ip_f64Epsilon = 0.000001;
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
	uint32 l_ui32HiddenNeuronCount = static_cast<uint32>(this->getInt64Parameter(OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount));
	float64 l_f64Alpha = this->getFloat64Parameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Alpha);
	float64 l_f64Epsilon = this->getFloat64Parameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Epsilon);
	this->uninitializeExtraParameterMechanism();

	if(l_ui32HiddenNeuronCount < 1)
	{
		this->getLogManager() << LogLevel_Error << "Invalid amount of neuron in the hidden layer. Fallback to default value (3)\n";
		l_ui32HiddenNeuronCount = 3;
	}
	if(l_f64Alpha <= 0)
	{
		this->getLogManager() << LogLevel_Error << "Invalid value for learning coefficient (" << l_f64Alpha << "). Fallback to default value (0.01)\n";
		l_f64Alpha = 0.01;
	}
	if(l_f64Epsilon <= 0)
	{
		this->getLogManager() << LogLevel_Error << "Invalid value for stop learning condition (" << l_f64Epsilon << "). Fallback to default value (0.000001)\n";
		l_f64Epsilon = 0.000001;
	}

	std::map < float64, uint32 > l_vClassCount;
	std::map < float64, VectorXd > l_oTargetList;
	//We need to compute the min and the max of data in order to normalize and center them
	for(uint32 i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		l_vClassCount[rFeatureVectorSet[i].getLabel()]++;
	}
	uint32 l_ui32ValidationElementCount = 0;

	//We generate the list of class
	for(std::map < float64, uint32 >::iterator iter = l_vClassCount.begin() ; iter != l_vClassCount.end() ; ++iter)
	{
		//We keep 20% percent of the training set for the validation for each class
		l_ui32ValidationElementCount += static_cast<uint32>(iter->second * 0.2);
		m_oLabelList.push_back(iter->first);
		iter->second = static_cast<uint32>(iter->second * 0.2);
	}

	const uint32 l_ui32ClassCount = m_oLabelList.size();
	const uint32 l_ui32FeatureSize = rFeatureVectorSet.getFeatureVector(0).getSize();

	//Generate the target vector for each class. To save time and memory, we compute only one vector per class
	//Vector tagret looks like following [0 0 1 0] for class 3 (if 4 classes)
	for(size_t i=0; i < l_ui32ClassCount; ++i)
	{
		VectorXd l_oTarget = VectorXd::Zero(l_ui32ClassCount);
		//class 1 is at index 0
		l_oTarget[static_cast<uint32>(m_oLabelList[i]) -1 ] = 1.;
		l_oTargetList[m_oLabelList[i]] = l_oTarget;
	}

	//We store each normalize vector we get for training. This not optimal in memory but avoid a lot of computation later
	//List of the class of the feature vectors store in the same order are they are in validation/training set(to be able to get the target)
	std::vector <float64> l_oTrainingSet;
	std::vector <float64> l_oValidationSet;
	MatrixXd l_oTrainingDataMatrix(l_ui32FeatureSize, rFeatureVectorSet.getFeatureVectorCount() - l_ui32ValidationElementCount);
	MatrixXd l_oValidationDataMatrix(l_ui32FeatureSize, l_ui32ValidationElementCount);

	//We don't need to make a shuffle it has already be made by the trainer box
	//We store 20% of the feature vectors for validation
	int32 l_i32ValidationIndex =0, l_i32TrainingIndex =0;
	for(size_t i=0; i < rFeatureVectorSet.getFeatureVectorCount(); ++i)
	{
		const Map<VectorXd> l_oFeatureVec(const_cast<float64*>(rFeatureVectorSet.getFeatureVector(i).getBuffer()), l_ui32FeatureSize);
		VectorXd l_oData = l_oFeatureVec;
		if(l_vClassCount[rFeatureVectorSet.getFeatureVector(i).getLabel()] > 0)
		{
			l_oValidationDataMatrix.col(l_i32ValidationIndex++) = l_oData;
			l_oValidationSet.push_back(rFeatureVectorSet.getFeatureVector(i).getLabel());
			--l_vClassCount[rFeatureVectorSet.getFeatureVector(i).getLabel()];
		}
		else{
			l_oTrainingDataMatrix.col(l_i32TrainingIndex++) = l_oData;
			l_oTrainingSet.push_back(rFeatureVectorSet.getFeatureVector(i).getLabel());
		}
	}

	//We now get the min and the max of the training set for normalization
	m_f64Max = l_oTrainingDataMatrix.maxCoeff();
	m_f64Min = l_oTrainingDataMatrix.minCoeff();
	//Normalization of the data. We need to do it to avoid saturation of tanh.
	for(int32 i=0; i < l_oTrainingDataMatrix.cols(); ++i)
	{
		for(int32 j=0; j < l_oTrainingDataMatrix.rows(); ++j)
		{
			l_oTrainingDataMatrix(j, i) = 2 * (l_oTrainingDataMatrix(j,i) - m_f64Min)/ (m_f64Max - m_f64Min) - 1;
		}
	}
	for(int32 i=0; i < l_oValidationDataMatrix.cols(); ++i)
	{
		for(int32 j=0; j < l_oValidationDataMatrix.rows(); ++j)
		{
			l_oValidationDataMatrix(j, i) = 2 * (l_oValidationDataMatrix(j,i) - m_f64Min)/ (m_f64Max - m_f64Min) - 1;
		}
	}

	const uint32 l_ui32FeatureCount = l_oTrainingSet.size();
	const float64 l_f64BoundValue = 1./(l_ui32FeatureSize+1);
	float64 l_f64PreviousError = std::numeric_limits<float64>::max();
	float64 l_f64CumulativeError = 0;

	//Let's generate randomly weights and biases
	//We restrain the weight between -1/(fan-in) and 1/(fan-in) to avoid saturation in the worst case
	m_oInputWeight = MatrixXd::Random(l_ui32HiddenNeuronCount, l_ui32FeatureSize) * l_f64BoundValue;
	m_oInputBias = VectorXd::Random(l_ui32HiddenNeuronCount) * l_f64BoundValue;

	m_oHiddenWeight = MatrixXd::Random(l_ui32ClassCount, l_ui32HiddenNeuronCount) * l_f64BoundValue;
	m_oHiddenBias = VectorXd::Random(l_ui32ClassCount) * l_f64BoundValue;

	MatrixXd l_oDeltaInputWeight = MatrixXd::Zero(l_ui32HiddenNeuronCount, l_ui32FeatureSize);
	VectorXd l_oDeltaInputBias = VectorXd::Zero(l_ui32HiddenNeuronCount);
	MatrixXd l_oDeltaHiddenWeight = MatrixXd::Zero(l_ui32ClassCount, l_ui32HiddenNeuronCount);
	VectorXd l_oDeltaHiddenBias = VectorXd::Zero(l_ui32ClassCount);

	MatrixXd m_oY1, m_oA2;
	//A1 is the value compute in hidden neuron before applying tanh
	//Y1 is the output vector of hidden layer
	//A2 is the value compute by output neuron before applying transfer function
	//Y2 is the value of output after the transfer function (softmax)
	while(1)
	{
		l_oDeltaInputWeight.setZero();
		l_oDeltaInputBias.setZero();
		l_oDeltaHiddenWeight.setZero();
		l_oDeltaHiddenBias.setZero();
		//The first cast of tanh has to been explicit for windows compilation
		m_oY1.noalias() = ((m_oInputWeight * l_oTrainingDataMatrix).colwise() + m_oInputBias).unaryExpr(std::ptr_fun<float64, float64>((float64 (*)(float64))tanh));
		m_oA2.noalias() = (m_oHiddenWeight * m_oY1).colwise() + m_oHiddenBias;
		for(size_t i =0; i < l_ui32FeatureCount; ++i)
		{
			const VectorXd& l_oTarget = l_oTargetList[l_oTrainingSet[i]];
			const VectorXd& l_oData = l_oTrainingDataMatrix.col(i);

			//Now we compute all deltas of output layer
			VectorXd l_oOutputDelta = m_oA2.col(i) - l_oTarget;
			for(size_t j = 0; j < l_ui32ClassCount; ++j)
			{
				for(size_t k = 0; k < l_ui32HiddenNeuronCount; ++k)
				{
					l_oDeltaHiddenWeight(j,k) -= l_oOutputDelta[j] * m_oY1.col(i)[k];
				}
			}
			l_oDeltaHiddenBias.noalias() -= l_oOutputDelta;

			//Now we take care of the hidden layer
			VectorXd l_oHiddenDelta = VectorXd::Zero(l_ui32HiddenNeuronCount);
			for(size_t j=0; j < l_ui32HiddenNeuronCount; ++j)
			{
				for(size_t k = 0; k < l_ui32ClassCount; ++k)
				{
					l_oHiddenDelta[j] += l_oOutputDelta[k] * m_oHiddenWeight(k,j);
				}
				l_oHiddenDelta[j] *= (1 - pow(m_oY1.col(i)[j], 2));
			}

			for(size_t j =0; j < l_ui32HiddenNeuronCount; ++j)
			{
				for(size_t k =0; k < l_ui32FeatureSize; ++k)
				{
					l_oDeltaInputWeight(j, k) -= l_oHiddenDelta[j] * l_oData[k];
				}
			}
			l_oDeltaInputBias.noalias() -= l_oHiddenDelta;

		}
		//We finish the loop, let's apply deltas
		m_oHiddenWeight.noalias() += l_oDeltaHiddenWeight / l_ui32FeatureCount * l_f64Alpha;
		m_oHiddenBias.noalias() += l_oDeltaHiddenBias / l_ui32FeatureCount * l_f64Alpha;
		m_oInputWeight.noalias() += l_oDeltaInputWeight / l_ui32FeatureCount * l_f64Alpha;
		m_oInputBias.noalias() += l_oDeltaInputBias / l_ui32FeatureCount * l_f64Alpha;

		dumpMatrix(this->getLogManager(), m_oHiddenWeight, "m_oHiddenWeight");
		dumpMatrix(this->getLogManager(), m_oHiddenBias, "m_oHiddenBias");
		dumpMatrix(this->getLogManager(), m_oInputWeight, "m_oInputWeight");
		dumpMatrix(this->getLogManager(), m_oInputBias, "m_oInputBias");

		//Now we compute the cumulative error in the validation set
		l_f64CumulativeError=0;
		//We don't compute Y2 because we train on the identity
		m_oA2.noalias() = (m_oHiddenWeight * ((m_oInputWeight * l_oValidationDataMatrix).colwise() + m_oInputBias).unaryExpr(std::ptr_fun<float64, float64>(tanh))).colwise()
							+ m_oHiddenBias;
		for(size_t i=0; i < l_oValidationSet.size(); ++i)
		{
			const VectorXd& l_oTarget = l_oTargetList[l_oValidationSet[i]];
			const VectorXd& l_oIdentityResult = m_oA2.col(i);

			//Now we need to compute the error
			for(size_t j =0; j < l_ui32ClassCount; ++j)
			{
				l_f64CumulativeError += 0.5 * pow(l_oIdentityResult[j] - l_oTarget[j], 2);
			}
		}
		l_f64CumulativeError /= l_oValidationSet.size();
		//If the delta of error is under Epsilon we consider that the training is over
		if(l_f64PreviousError - l_f64CumulativeError < l_f64Epsilon)
		{
			break;
		}
		l_f64PreviousError = l_f64CumulativeError;
		//std::cout << l_f64PreviousError << std::endl;
	}
	dumpMatrix(this->getLogManager(), m_oHiddenWeight, "l_oHiddenWeight");
	dumpMatrix(this->getLogManager(), m_oHiddenBias, "l_oHiddenBias");
	dumpMatrix(this->getLogManager(), m_oInputWeight, "l_oInputWeight");
	dumpMatrix(this->getLogManager(), m_oInputBias, "l_oInputBias");
	return true;
}

boolean CAlgorithmClassifierMLP::classify(const IFeatureVector &rFeatureVector, float64 &rf64Class, IVector &rDistanceValue, IVector &rProbabilityValue)
{
	if(rFeatureVector.getSize() != m_oInputWeight.cols())
	{
		this->getLogManager() << LogLevel_Error << "Classifier expected " << m_oInputWeight.cols() << " features, got " << rFeatureVector.getSize() << "\n";
		return false;
	}

	const Map<VectorXd> l_oFeatureVec(const_cast<float64*>(rFeatureVector.getBuffer()), rFeatureVector.getSize());
	VectorXd l_oData = l_oFeatureVec;
	//we normalize and center data on 0 to avoid saturation
	for(size_t j =0; j < rFeatureVector.getSize(); ++j)
	{
		l_oData[j] = 2 * (l_oData[j] - m_f64Min)/ (m_f64Max - m_f64Min) - 1;
	}

	const uint32 l_ui32ClassCount = m_oLabelList.size();

	VectorXd m_oA2 = m_oHiddenBias + (m_oHiddenWeight * (m_oInputBias + (m_oInputWeight * l_oData)).unaryExpr(std::ptr_fun<float64, float64>(tanh)));

	//The final transfer function is the softmax
	VectorXd m_oY2 = m_oA2.unaryExpr(std::ptr_fun<float64, float64>(exp));
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
	for(int32 i = 0; i< m_oHiddenBias.size() ; ++i)
	{
		l_sClasses << m_oLabelList[i] << " ";
	}
	XML::IXMLNode* l_pClassLabelNode = XML::createNode(c_sMLPClassLabelNodeName);
	l_pClassLabelNode->setPCData(l_sClasses.str().c_str());
	l_pRootNode->addChild(l_pClassLabelNode);

	XML::IXMLNode* l_pConfiguration = XML::createNode(c_sMLPNeuronConfigurationNodeName);

	//The input and output neuron count are not mandatory but they facilitate a lot the loading process
	l_pTempNode = XML::createNode(c_sMLPInputNeuronCountNodeName);
	dumpData(l_pTempNode, static_cast<int64>(m_oInputWeight.cols()));
	l_pConfiguration->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sMLPHiddenNeuronCountNodeName);
	dumpData(l_pTempNode, static_cast<int64>(m_oInputWeight.rows()));
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
