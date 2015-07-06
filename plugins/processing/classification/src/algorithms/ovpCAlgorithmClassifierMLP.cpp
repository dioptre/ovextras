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
	this->initializeExtraParameterMechanism();
	m_i64HiddenNeuronCount = this->getInt64Parameter(OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount);
	m_oEvaluationFunctionIdentifier = this->getEnumerationParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_EvaluationFunction, OVP_TypeId_Enumeration_EvaluationFunction);
	m_oTransfertFunctionIdentifier = this->getEnumerationParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_TransfertFunction, OVP_TypeId_Enumeration_TransfertFunction);
	this->uninitializeExtraParameterMechanism();

	if(m_i64HiddenNeuronCount < 1)
	{
		this->getLogManager() << LogLevel_Error << "Invalid amount of neuron in the hidden layer. Fallback to default value (3)\n";
		m_i64HiddenNeuronCount = 3;
	}

	std::map < float64, uint32 > l_vClassLabels;
	for(uint32 i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		l_vClassLabels[rFeatureVectorSet[i].getLabel()]++;
	}

	const uint32 l_ui32ClassCount = l_vClassLabels.size();
	const float64 l_f64Alpha = 0.001;
	const uint64 l_ui64FeatureSize = rFeatureVectorSet.getFeatureVector(0).getSize();
	const uint64 l_ui64FeatureCount = rFeatureVectorSet.getFeatureVectorCount();


	MatrixXd l_oInputWeight = MatrixXd::Random(m_i64HiddenNeuronCount, l_ui64FeatureSize);
	VectorXd l_oInputBias = VectorXd::Random(m_i64HiddenNeuronCount);

	MatrixXd l_oHiddenWeight = MatrixXd::Random(l_ui32ClassCount, m_i64HiddenNeuronCount);
	VectorXd l_oHiddenBias = VectorXd::Random(l_ui32ClassCount);

	dumpMatrix(this->getLogManager(), l_oHiddenWeight, "l_oHiddenWeight");
	dumpMatrix(this->getLogManager(), l_oHiddenBias, "l_oHiddenBias");
	dumpMatrix(this->getLogManager(), l_oInputWeight, "l_oInputWeight");
	dumpMatrix(this->getLogManager(), l_oInputBias, "l_oInputBias");

	VectorXd l_oData(l_ui64FeatureSize);

	for(size_t l_uiTrainingIteration = 0 ; l_uiTrainingIteration < 5000 ; ++l_uiTrainingIteration)
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
			l_oData = l_oFeatureVec;
			//dumpMatrix(this->getLogManager(), l_oData, "Feature");
			//std::cout << "Compute Output" << std::endl;
			VectorXd m_oA1 = l_oInputBias + (l_oInputWeight * l_oData);
			//dumpMatrix(this->getLogManager(), m_oA1, "m_oA1");
			VectorXd m_oY1(m_i64HiddenNeuronCount);
			for(size_t j = 0; j < m_i64HiddenNeuronCount; ++j)
			{
				m_oY1[j] = tanh(m_oA1[j]);
			}
			//dumpMatrix(this->getLogManager(), m_oY1, "m_oY1");
			VectorXd m_oA2 = l_oHiddenBias + (l_oHiddenWeight * m_oY1);
			VectorXd m_oY2(l_ui32ClassCount);

			//std::cout <<"Let's apply tranfert function" << std::endl;
			if(m_oTransfertFunctionIdentifier == OVP_Algorithm_ClassifierMLP_Enumeration_TransfertFunction_Identity)
			{
				m_oY2 = m_oA2;
			}
			else if(m_oTransfertFunctionIdentifier == OVP_Algorithm_ClassifierMLP_Enumeration_TransfertFunction_Softmax)
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
						l_ui32ClassFound = j;
					}
				}
				if(l_ui32ClassFound != (uint32)rFeatureVectorSet.getFeatureVector(i).getLabel())
				{
					l_f64Error = 1;
				}
			}
			l_f64CumulativeError += l_f64Error;
			//std::cout << "Error : " << l_f64Error << std::endl;

			//std::cout << "Compute delta" << std::endl;
			//Now we compute all deltas of output layer
			VectorXd l_oOutputDelta = m_oY2 - l_oGoal;

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
					l_oHiddenDelta[j] += l_oOutputDelta[k] * l_oHiddenWeight(k,j);
				}
				l_oHiddenDelta[j] *= 1 - pow(m_oY1[j], 2);
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

//		dumpMatrix(this->getLogManager(), l_oDeltaHiddenWeight, "l_oDeltaHiddenWeight");
//		dumpMatrix(this->getLogManager(), l_oDeltaHiddenBias, "l_oDeltaHiddenBias");
//		dumpMatrix(this->getLogManager(), l_oDeltaInputWeight, "l_oDeltaInputWeight");
//		dumpMatrix(this->getLogManager(), l_oDeltaInputBias, "l_oDeltaInputBias");

		//We finish the loop, let's apply deltas and restart
		l_oHiddenWeight += (l_oDeltaHiddenWeight / l_ui64FeatureCount);
		l_oHiddenBias += (l_oDeltaHiddenBias / l_ui64FeatureCount);
		l_oInputWeight += (l_oDeltaInputWeight / l_ui64FeatureCount);
		l_oInputBias += (l_oDeltaInputBias / l_ui64FeatureCount);

//		dumpMatrix(this->getLogManager(), l_oHiddenWeight, "l_oHiddenWeight");
//		dumpMatrix(this->getLogManager(), l_oHiddenBias, "l_oHiddenBias");
//		dumpMatrix(this->getLogManager(), l_oInputWeight, "l_oInputWeight");
//		dumpMatrix(this->getLogManager(), l_oInputBias, "l_oInputBias");

	}
	exit(0);
	return true;
}

boolean CAlgorithmClassifierMLP::classify(const IFeatureVector &rFeatureVector, float64 &rf64Class, IVector &rDistanceValue, IVector &rProbabilityValue)
{
	return true;
}

XML::IXMLNode *CAlgorithmClassifierMLP::saveConfiguration()
{
	return NULL;
}

boolean CAlgorithmClassifierMLP::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	return true;
}

//#endif
