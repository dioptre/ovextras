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
#include <Eigen/Core>

//Need to be reachable from outside
const char* const c_sMLPEvaluationFunctionName = "Evaluation function";
const char* const c_sMLPTransfertFunctionName = "Transfert function";

namespace{
	const char* const c_sMLPTypeNodeName = "MLP";
	const char* const c_sMLPNeuronConfigurationNodeName = "Neuron-configuration";
	const char* const c_sMLPInputNeuronCountNodeName = "Input-neuron-count";
	const char* const c_sMLPHiddenNeuronCountNodeName = "Hidden-neuron-count";
	const char* const c_sMLPOutputNeuronCountNodeName = "Output-neuron-count";
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
	this->uninitializeExtraParameterMechanism();

	if(m_i64HiddenNeuronCount < 1)
	{
		this->getLogManager() << LogLevel_Error << "Invalid amount of neuron in the hidden layer. Fallback to default value (3)\n";
		m_i64HiddenNeuronCount = 3;
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

	//Now we need to generate the training set and the validation set. We take 20% of datas for the validation set
	for(std::map < float64, uint32 >::iterator iter = l_vClassLabels.begin() ; iter != l_vClassLabels.end() ; ++iter)
	{
		m_oLabelList.push_back(iter->first);
		iter->second *= 0.2;
		std::cout << iter->second << std::endl;
	}

	const float64 l_f64Alpha = 0.01;
	const float64 l_f64Epsilon=0.000001;
	const uint32 l_ui32ClassCount = m_oLabelList.size();
	const uint64 l_ui64FeatureSize = rFeatureVectorSet.getFeatureVector(0).getSize();

	for(size_t i=0; i < l_ui32ClassCount; ++i)
	{
		VectorXd l_oGoal = VectorXd::Zero(l_ui32ClassCount);
		//class 1 is at index 0
		l_oGoal[(uint64)m_oLabelList[i] -1 ] = 1.;
		l_oClassGoal[m_oLabelList[i]] = l_oGoal;
	}

	std::vector <CEigenFeatureVector> m_oTrainingSet;
	std::vector <CEigenFeatureVector> m_oValidationSet;

	//We don't need to make a shuffle it has already be made by the trainer box
	for(size_t i=0; i < rFeatureVectorSet.getFeatureVectorCount(); ++i)
	{
		if(l_vClassLabels[rFeatureVectorSet.getFeatureVector(i).getLabel()] > 0)
		{
			const Map<VectorXd> l_oFeatureVec(const_cast<float64*>(rFeatureVectorSet.getFeatureVector(i).getBuffer()), l_ui64FeatureSize);
			VectorXd l_oData = l_oFeatureVec;
			for(size_t j =0; j < l_ui64FeatureSize; ++j)
			{
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
				l_oData[j] = 2 * (l_oData[j] - m_f64Min)/ (m_f64Max - m_f64Min) - 1;
			}
			m_oTrainingSet.push_back(CEigenFeatureVector(rFeatureVectorSet.getFeatureVector(i).getLabel(), l_oData));
		}
	}

	const uint64 l_ui64FeatureCount = m_oTrainingSet.size();

	m_oInputWeight = MatrixXd::Random(m_i64HiddenNeuronCount, l_ui64FeatureSize);
	m_oInputBias = VectorXd::Random(m_i64HiddenNeuronCount);

	m_oHiddenWeight = MatrixXd::Random(l_ui32ClassCount, m_i64HiddenNeuronCount);
	m_oHiddenBias = VectorXd::Random(l_ui32ClassCount);

	const float64 l_f64BoundValue = 1./(l_ui64FeatureSize+1);
	float64 l_f64PreviousError = std::numeric_limits<float64>::max();
	float64 l_f64CumulativeError = 0;


	//Let's restrain the weight between -1/(fan-in) and 1/(fan-in) to avoid saturation in the worst case
	m_oInputWeight*= l_f64BoundValue;
	m_oInputBias*= l_f64BoundValue;
	m_oHiddenWeight*= l_f64BoundValue;
	m_oHiddenBias*= l_f64BoundValue;

	MatrixXd l_oDeltaInputWeight = MatrixXd::Zero(m_i64HiddenNeuronCount, l_ui64FeatureSize);
	VectorXd l_oDeltaInputBias = VectorXd::Zero(m_i64HiddenNeuronCount);
	MatrixXd l_oDeltaHiddenWeight = MatrixXd::Zero(l_ui32ClassCount, m_i64HiddenNeuronCount);
	VectorXd l_oDeltaHiddenBias = VectorXd::Zero(l_ui32ClassCount);

	VectorXd m_oA1, m_oY1, m_oA2;

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
			//We don't need to compute Y2 (we use the identity for training

			//Now we compute all deltas of output layer
			VectorXd l_oOutputDelta = m_oA2 - l_oGoal;
			for(size_t j = 0; j < l_ui32ClassCount; ++j)
			{
				for(size_t k = 0; k < (uint32)m_i64HiddenNeuronCount; ++k)
				{
					l_oDeltaHiddenWeight(j,k) -= l_oOutputDelta[j] * m_oY1[k];
				}
			}
			l_oDeltaHiddenBias.noalias() -= l_oOutputDelta;

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

			for(size_t j =0; j < (size_t)m_i64HiddenNeuronCount; ++j)
			{
				for(size_t k =0; k < l_ui64FeatureSize; ++k)
				{
					l_oDeltaInputWeight(j, k) -= l_oHiddenDelta[j] * l_oData[k];
				}
			}
			l_oDeltaInputBias.noalias() -= l_oHiddenDelta;

		}
		//std::cout << "Error : " << l_f64CumulativeError / l_ui64FeatureCount << std::endl;

		//dumpMatrix(this->getLogManager(), l_oDeltaHiddenWeight, "l_oDeltaHiddenWeight");
//		dumpMatrix(this->getLogManager(), l_oDeltaHiddenBias, "l_oDeltaHiddenBias");
//		dumpMatrix(this->getLogManager(), l_oDeltaInputWeight, "l_oDeltaInputWeight");
//		dumpMatrix(this->getLogManager(), l_oDeltaInputBias, "l_oDeltaInputBias");

		//We finish the loop, let's apply deltas and restart
		m_oHiddenWeight.noalias() += l_oDeltaHiddenWeight / l_ui64FeatureCount * l_f64Alpha;
		m_oHiddenBias.noalias() += l_oDeltaHiddenBias / l_ui64FeatureCount * l_f64Alpha;
		m_oInputWeight.noalias() += l_oDeltaInputWeight / l_ui64FeatureCount * l_f64Alpha;
		m_oInputBias.noalias() += l_oDeltaInputBias / l_ui64FeatureCount * l_f64Alpha;

//		dumpMatrix(this->getLogManager(), l_oHiddenWeight, "l_oHiddenWeight");
//		dumpMatrix(this->getLogManager(), l_oHiddenBias, "l_oHiddenBias");
//		dumpMatrix(this->getLogManager(), l_oInputWeight, "l_oInputWeight");
//		dumpMatrix(this->getLogManager(), l_oInputBias, "l_oInputBias");

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
		if(l_f64PreviousError - l_f64CumulativeError < l_f64Epsilon)
		{
			std::cout << l_uiTrainingIteration << " "<< l_f64PreviousError << " " <<  l_f64CumulativeError << std::endl;
			break;
		}
		l_f64PreviousError = l_f64CumulativeError;
	}
//	dumpMatrix(this->getLogManager(), m_oHiddenWeight, "l_oHiddenWeight");
//	dumpMatrix(this->getLogManager(), m_oHiddenBias, "l_oHiddenBias");
//	dumpMatrix(this->getLogManager(), m_oInputWeight, "l_oInputWeight");
//	dumpMatrix(this->getLogManager(), m_oInputBias, "l_oInputBias");
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

	const uint32 l_ui32ClassCount = m_oHiddenBias.size();
	VectorXd m_oA1 = m_oInputBias + (m_oInputWeight * l_oData);

	VectorXd m_oY1(m_i64HiddenNeuronCount);
	for(size_t j = 0; j < m_i64HiddenNeuronCount; ++j)
	{
		m_oY1[j] = tanh(m_oA1[j]);
	}

	VectorXd m_oA2 = m_oHiddenBias + (m_oHiddenWeight * m_oY1);
	VectorXd m_oY2(l_ui32ClassCount);

	for(size_t j = 0; j< l_ui32ClassCount ; ++j)
	{
		m_oY2[j] = exp(m_oA2[j]);
	}
	m_oY2 /= m_oY2.sum();

	rDistanceValue.setSize(l_ui32ClassCount);
	rProbabilityValue.setSize(l_ui32ClassCount);

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

	XML::IXMLNode* l_pResultNode = XML::createNode(c_sClassifierRoot);
	l_pResultNode->addChild(l_pRootNode);
	return l_pResultNode;
}

boolean CAlgorithmClassifierMLP::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	XML::IXMLNode* l_pMLPNode = pConfigurationNode->getChild(0);
	m_oLabelList.clear();
	std::stringstream l_sData(l_pMLPNode->getChildByName(c_sMLPClassLabelNodeName)->getPCData());
	float64 l_f64Temp;
	while(l_sData >> l_f64Temp)
	{
		m_oLabelList.push_back(l_f64Temp);
	}

	XML::IXMLNode* l_pNeuronConfigurationNode = l_pMLPNode->getChildByName(c_sMLPNeuronConfigurationNodeName);
	loadData(l_pNeuronConfigurationNode->getChildByName(c_sMLPHiddenNeuronCountNodeName), m_i64HiddenNeuronCount);
	int64 l_i64FeatureSize, l_i64ClassCount;
	loadData(l_pNeuronConfigurationNode->getChildByName(c_sMLPInputNeuronCountNodeName), l_i64FeatureSize);
	loadData(l_pNeuronConfigurationNode->getChildByName(c_sMLPOutputNeuronCountNodeName), l_i64ClassCount);

	loadData(l_pMLPNode->getChildByName(c_sMLPMaximumNodeName), m_f64Max);
	loadData(l_pMLPNode->getChildByName(c_sMLPMinimumNodeName), m_f64Min);

	loadData(l_pMLPNode->getChildByName(c_sMLPInputWeightNodeName),m_oInputWeight, m_i64HiddenNeuronCount, l_i64FeatureSize);
	loadData(l_pMLPNode->getChildByName(c_sMLPInputBiasNodeName), m_oInputBias);
	loadData(l_pMLPNode->getChildByName(c_sMLPHiddenWeightNodeName),m_oHiddenWeight, l_i64ClassCount, m_i64HiddenNeuronCount);
	loadData(l_pMLPNode->getChildByName(c_sMLPHiddenBiasNodeName), m_oHiddenBias);

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

void CAlgorithmClassifierMLP::loadData(XML::IXMLNode *pNode, CIdentifier &rIdentifier)
{
	rIdentifier.fromString(pNode->getAttribute(c_sMLPIdentifierAttributeName));
}

//#endif
