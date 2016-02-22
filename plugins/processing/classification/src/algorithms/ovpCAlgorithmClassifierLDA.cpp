#include "ovpCAlgorithmClassifierLDA.h"
#if defined TARGET_HAS_ThirdPartyEIGEN

#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <system/ovCMemory.h>
#include <xml/IXMLHandler.h>

#include <Eigen/Eigenvalues>

#include "../algorithms/ovpCAlgorithmConditionedCovariance.h"

namespace{
	const char* const c_sTypeNodeName = "LDA";
	const char* const c_sClassesNodeName = "Classes";
	const char* const c_sCoefficientsNodeName = "Weights";
	const char* const c_sBiasDistanceNodeName = "Bias-distance";
	const char* const c_sCoefficientProbabilityNodeName = "Coefficient-probability";
	const char* const c_sComputationHelpersConfigurationNode = "Class-config-list";
	const char* const c_sLDAConfigFileVersionAttributeName = "version";
}

extern const char* const c_sClassifierRoot;

OpenViBE::int32 OpenViBEPlugins::Classification::LDAClassificationCompare(OpenViBE::IMatrix& rFirstClassificationValue, OpenViBE::IMatrix& rSecondClassificationValue)
{
	//We first need to find the best classification of each.
	OpenViBE::float64* l_pClassificationValueBuffer = rFirstClassificationValue.getBuffer();
	const OpenViBE::float64 l_f64MaxFirst = *(std::max_element(l_pClassificationValueBuffer, l_pClassificationValueBuffer+rFirstClassificationValue.getBufferElementCount()));

	l_pClassificationValueBuffer = rSecondClassificationValue.getBuffer();
	const OpenViBE::float64 l_f64MaxSecond = *(std::max_element(l_pClassificationValueBuffer, l_pClassificationValueBuffer+rSecondClassificationValue.getBufferElementCount()));

	//Then we just compared them
	if(ov_float_equal(l_f64MaxFirst, l_f64MaxSecond))
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

#define LDA_DEBUG 0
#if LDA_DEBUG
void CAlgorithmClassifierLDA::dumpMatrix(OpenViBE::Kernel::ILogManager &rMgr, const MatrixXdRowMajor &mat, const CString &desc)
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
void CAlgorithmClassifierLDA::dumpMatrix(OpenViBE::Kernel::ILogManager& /* rMgr */, const MatrixXdRowMajor& /*mat*/, const CString& /*desc*/) { }
#endif

uint32 CAlgorithmClassifierLDA::getOutputProbabilityVectorLength()
{
	if(m_bv1Classification){
		return 1;
	} else{
		return m_vDiscriminantFunctions.size();
	}
}

uint32 CAlgorithmClassifierLDA::getOutputDistanceVectorLength()
{
	if(m_bv1Classification){
		return 1;
	} else{
		return m_vDiscriminantFunctions.size();
	}
}


boolean CAlgorithmClassifierLDA::initialize(void)
{
	m_bv1Classification = false;
	// Initialize the Conditioned Covariance Matrix algorithm
	m_pCovarianceAlgorithm = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ConditionedCovariance));
	m_pCovarianceAlgorithm->initialize();

	// This is the weight parameter local to this module and automatically exposed to the GUI. Its redirected to the corresponding parameter of the cov alg.
	TParameterHandler< float64 > ip_f64Shrinkage(this->getInputParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_Shrinkage));
	ip_f64Shrinkage.setReferenceTarget(m_pCovarianceAlgorithm->getInputParameter(OVP_Algorithm_ConditionedCovariance_InputParameterId_Shrinkage));

	TParameterHandler < boolean > ip_bDiagonalCov(this->getInputParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_DiagonalCov));
	ip_bDiagonalCov = false;

	TParameterHandler < XML::IXMLNode* > op_pConfiguration(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	op_pConfiguration=NULL;

	return CAlgorithmClassifier::initialize();
}

boolean CAlgorithmClassifierLDA::uninitialize(void)
{
	m_pCovarianceAlgorithm->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_pCovarianceAlgorithm);

	return CAlgorithmClassifier::uninitialize();
}

boolean CAlgorithmClassifierLDA::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	this->initializeExtraParameterMechanism();

	//We need to clear list because a instance of this class should support more that one training.
	m_vLabelList.clear();
	m_vDiscriminantFunctions.clear();

	const boolean l_bUseShrinkage = this->getBooleanParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_UseShrinkage);

	boolean l_pDiagonalCov;
	if(l_bUseShrinkage)
	{
		this->getFloat64Parameter(OVP_Algorithm_ClassifierLDA_InputParameterId_Shrinkage);
		l_pDiagonalCov = this->getBooleanParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_DiagonalCov);

	}
	else{
		//If we don't use shrinkage we need to set lambda to 0.
		TParameterHandler< float64 > ip_f64Shrinkage(this->getInputParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_Shrinkage));
		ip_f64Shrinkage = 0.0;

		TParameterHandler < boolean > ip_bDiagonalCov(this->getInputParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_DiagonalCov));
		ip_bDiagonalCov = false;
		l_pDiagonalCov = false;
	}
	this->uninitializeExtraParameterMechanism();



	// IO to the covariance alg
	TParameterHandler < OpenViBE::IMatrix* > op_pMean(m_pCovarianceAlgorithm->getOutputParameter(OVP_Algorithm_ConditionedCovariance_OutputParameterId_Mean));
	TParameterHandler < OpenViBE::IMatrix* > op_pCovarianceMatrix(m_pCovarianceAlgorithm->getOutputParameter(OVP_Algorithm_ConditionedCovariance_OutputParameterId_CovarianceMatrix));
	TParameterHandler < OpenViBE::IMatrix* > ip_pFeatureVectorSet(m_pCovarianceAlgorithm->getInputParameter(OVP_Algorithm_ConditionedCovariance_InputParameterId_FeatureVectorSet));

	const uint32 l_ui32nRows = rFeatureVectorSet.getFeatureVectorCount();
	const uint32 l_ui32nCols = (l_ui32nRows > 0 ? rFeatureVectorSet[0].getSize() : 0);
	this->getLogManager() << LogLevel_Debug << "Feature set input dims [" 
		<< rFeatureVectorSet.getFeatureVectorCount() << "x" << l_ui32nCols << "]\n";

	if(l_ui32nRows==0 || l_ui32nCols==0) {
		this->getLogManager() << LogLevel_Error << "Input data has a zero-size dimension, dims = [" << l_ui32nRows << "x" << l_ui32nCols << "]\n";
		return false;
	}

	// Count the classes
	std::map < float64, uint32 > l_vClassCounts;
	for(uint32 i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		l_vClassCounts[rFeatureVectorSet[i].getLabel()]++;
	}

	const uint32 l_ui32nClasses = l_vClassCounts.size();

	// Get class labels
	for(std::map < float64, uint32 >::iterator iter = l_vClassCounts.begin() ; iter != l_vClassCounts.end() ; ++iter)
	{
		m_vLabelList.push_back(iter->first);
		m_vDiscriminantFunctions.push_back(CAlgorithmLDADiscriminantFunction());
	}

	// Per-class means and a global covariance are used to form the LDA model
	MatrixXd* l_oPerClassMeans = new MatrixXd[l_ui32nClasses];
	MatrixXd l_oGlobalCov = MatrixXd::Zero(l_ui32nCols,l_ui32nCols);

	// We need the means per class
	for(uint32 l_ui32classIdx=0;l_ui32classIdx<l_ui32nClasses;l_ui32classIdx++) 
	{
		const float64 l_f64Label = m_vLabelList[l_ui32classIdx];
		const uint32 l_ui32nExamplesInClass = l_vClassCounts[l_f64Label];

		// Copy all the data of the class to a matrix
		CMatrix l_oClassData;
		l_oClassData.setDimensionCount(2);
		l_oClassData.setDimensionSize(0, l_ui32nExamplesInClass);
		l_oClassData.setDimensionSize(1, l_ui32nCols);
		float64 *l_pBuffer = l_oClassData.getBuffer();
		for(uint32 i=0;i<l_ui32nRows;i++)
		{
			if(rFeatureVectorSet[i].getLabel() == l_f64Label) 
			{
				System::Memory::copy(l_pBuffer, rFeatureVectorSet[i].getBuffer(), l_ui32nCols*sizeof(float64));
				l_pBuffer += l_ui32nCols;
			}
		}

		// Get the mean out of it
		Map<MatrixXdRowMajor> l_oDataMapper(l_oClassData.getBuffer(), l_ui32nExamplesInClass, l_ui32nCols);
		const MatrixXd l_oClassMean = l_oDataMapper.colwise().mean().transpose();
		l_oPerClassMeans[l_ui32classIdx] = l_oClassMean;
	}

	// We need a global covariance, use the regularized cov algorithm
	{
		ip_pFeatureVectorSet->setDimensionCount(2);
		ip_pFeatureVectorSet->setDimensionSize(0, l_ui32nRows);
		ip_pFeatureVectorSet->setDimensionSize(1, l_ui32nCols);
		float64 *l_pBuffer = ip_pFeatureVectorSet->getBuffer();

		// Insert all data as the input of the cov algorithm
		for(uint32 i=0;i<l_ui32nRows;i++)
		{
			System::Memory::copy(l_pBuffer, rFeatureVectorSet[i].getBuffer(), l_ui32nCols*sizeof(float64));
			l_pBuffer += l_ui32nCols;
		}
		
		// Compute cov
		if(!m_pCovarianceAlgorithm->process()) {
			this->getLogManager() << LogLevel_Error << "Global covariance computation failed\n";

			//Free memory before leaving
			delete[] l_oPerClassMeans;
			return false;
		}

		// Get the results from the cov algorithm
		Map<MatrixXdRowMajor> l_oCovMapper(op_pCovarianceMatrix->getBuffer(), l_ui32nCols, l_ui32nCols);
		l_oGlobalCov = l_oCovMapper;
	}

	//dumpMatrix(this->getLogManager(), l_aMean[l_ui32classIdx], "Mean");
	//dumpMatrix(this->getLogManager(), l_oGlobalCov, "Shrinked cov");

	if(l_pDiagonalCov)
	{
		for(uint32 i=0;i<l_ui32nCols;i++)
		{
			for(uint32 j=i+1;j<l_ui32nCols;j++)
			{
				l_oGlobalCov(i,j) = 0.0;
				l_oGlobalCov(j,i) = 0.0;
			}
		}
	}

	// Get the pseudoinverse of the global cov using eigen decomposition for self-adjoint matrices
	const float64 l_f64Tolerance = 1e-10;
	SelfAdjointEigenSolver<MatrixXd> l_oEigenSolver;
	l_oEigenSolver.compute(l_oGlobalCov);
	VectorXd l_oEigenValues = l_oEigenSolver.eigenvalues();
	for(uint32 i=0;i<l_ui32nCols;i++) {
		if(l_oEigenValues(i) >= l_f64Tolerance) {
			l_oEigenValues(i) = 1.0/l_oEigenValues(i);
		}
	}

	const MatrixXd l_oGlobalCovInv = l_oEigenSolver.eigenvectors() * l_oEigenValues.asDiagonal() * l_oEigenSolver.eigenvectors().inverse();	
	//We send the bias and the weight of each class to ComputationHelper
	for(size_t i = 0 ; i < getClassCount() ; ++i)
	{
		const float64 l_f64ExamplesInClass = l_vClassCounts[m_vLabelList[i]];
		const uint32 l_ui32TotalExamples = rFeatureVectorSet.getFeatureVectorCount();

		// This formula e.g. in Hastie, Tibshirani & Friedman: "Elements...", 2nd ed., p. 109
		const VectorXd l_oWeight = (l_oGlobalCovInv * l_oPerClassMeans[i]);
		const MatrixXd l_oInter = -0.5 * l_oPerClassMeans[i].transpose() * l_oGlobalCovInv * l_oPerClassMeans[i];
		const float64 l_f64Bias = l_oInter(0,0) + std::log(l_f64ExamplesInClass/l_ui32TotalExamples);

		// this->getLogManager() << LogLevel_Info << "Bias for " << i << " is " << l_f64Bias << ", from " << l_f64ExamplesInClass / l_ui32TotalExamples 
		// 	<< ", " << stuffInClass << "/" << featVectors 
		// 	<< "\n";
		// dumpMatrix(this->getLogManager(), l_oPerClassMeans[i], "Means");

		m_vDiscriminantFunctions[i].setWeight(l_oWeight);
		m_vDiscriminantFunctions[i].setBias(l_f64Bias);

	}

	m_ui32NumCols = l_ui32nCols;
	
	// Debug output
	//dumpMatrix(this->getLogManager(), l_oGlobalCov, "Global cov");
	//dumpMatrix(this->getLogManager(), l_oEigenValues, "Eigenvalues");
	//dumpMatrix(this->getLogManager(), l_oEigenSolver.eigenvectors(), "Eigenvectors");
	//dumpMatrix(this->getLogManager(), l_oGlobalCovInv, "Global cov inverse");
	//dumpMatrix(this->getLogManager(), m_oCoefficients, "Hyperplane weights");

	delete[] l_oPerClassMeans;
	return true;
}

boolean CAlgorithmClassifierLDA::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues, IVector& rProbabilityValue)
{
	if(m_bv1Classification)
	{
		const Map<MatrixXdRowMajor> l_oFeatureVec(const_cast<float64*>(rFeatureVector.getBuffer()), 1, rFeatureVector.getSize());

		const uint32 l_ui32nColsWithBiasTerm = m_oCoefficients.size();

		if(rFeatureVector.getSize()+1!=l_ui32nColsWithBiasTerm)
		{
			this->getLogManager() << LogLevel_Warning << "Feature vector size " << rFeatureVector.getSize() << " + 1 and hyperplane parameter size " << l_ui32nColsWithBiasTerm << " do not match\n";
			return false;
		}

		// Catenate 1.0 to match the bias term
		MatrixXd l_oWeights(1, l_ui32nColsWithBiasTerm);
		l_oWeights(0,0) = 1.0;
		l_oWeights.block(0,1,1,l_ui32nColsWithBiasTerm-1) = l_oFeatureVec;

		const float64 l_f64Result = (l_oWeights*m_oCoefficients.transpose()).col(0)(0);

		rClassificationValues.setSize(1);
		rClassificationValues[0]= -l_f64Result;

		const float64 l_f64a =(m_oWeights * l_oFeatureVec.transpose()).col(0)(0) + m_f64w0;
		const float64 l_f64P1 = 1 / (1 + exp(-l_f64a));

		rProbabilityValue.setSize(1);
		rProbabilityValue[0] = l_f64P1;

		if(l_f64P1 >= 0.5)
		{
			rf64Class=m_vLabelList[0];
		}
		else
		{
			rf64Class=m_vLabelList[1];
		}
	}
	else
	{
		if(m_vDiscriminantFunctions.empty())
		{
			this->getLogManager() << LogLevel_Error << "LDA discriminant function list is empty\n";
			return false;
		}

		if(rFeatureVector.getSize() != m_vDiscriminantFunctions[0].getWeightVectorSize())
		{
			this->getLogManager() << LogLevel_Error << "Classifier expected " << m_vDiscriminantFunctions[0].getWeightVectorSize() << " features, got " << rFeatureVector.getSize() << "\n";
			return false;
		}

		const Map<VectorXd> l_oFeatureVec(const_cast<float64*>(rFeatureVector.getBuffer()), rFeatureVector.getSize());
		const VectorXd l_oWeights = l_oFeatureVec;
		const uint32 l_ui32ClassCount = getClassCount();

		float64 *l_pValueArray = new float64[l_ui32ClassCount];
		float64 *l_pProbabilityValue = new float64[l_ui32ClassCount];
		//We ask for all computation helper to give the corresponding class value
		for(size_t i = 0; i < l_ui32ClassCount ; ++i)
		{
			l_pValueArray[i] = m_vDiscriminantFunctions[i].getValue(l_oWeights);
		}

		//p(Ck | x) = exp(ak) / sum[j](exp (aj))
		// with aj = (Weight for class j).transpose() * x + (Bias for class j)

		//Exponential can lead to nan results, so we reduce the computation and instead compute
		// p(Ck | x) = 1 / sum[j](exp(aj - ak))

		//All ak are given by computation helper
		for(size_t i = 0 ; i < l_ui32ClassCount ; ++i)
		{
			float64 l_f64ExpSum = 0.;
			for(size_t j = 0 ; j < l_ui32ClassCount ; ++j)
			{
				l_f64ExpSum += exp(l_pValueArray[j] - l_pValueArray[i]);
			}
			l_pProbabilityValue[i] = 1/l_f64ExpSum;
		}

		//Then we just found the highest score and took it as results
		uint32 l_ui32ClassIndex = std::distance(l_pValueArray, std::max_element(l_pValueArray, l_pValueArray+l_ui32ClassCount));

		rClassificationValues.setSize(l_ui32ClassCount);
		rProbabilityValue.setSize(l_ui32ClassCount);

		for(size_t i = 0 ; i < l_ui32ClassCount ; ++i)
		{
			rClassificationValues[i] = l_pValueArray[i];
			rProbabilityValue[i] = l_pProbabilityValue[i];
		}
		rf64Class = m_vLabelList[l_ui32ClassIndex];

		delete[] l_pValueArray;
		delete[] l_pProbabilityValue;
	}
	return true;
}

uint32 CAlgorithmClassifierLDA::getClassCount()
{
	return m_vLabelList.size();
}

XML::IXMLNode* CAlgorithmClassifierLDA::saveConfiguration(void)
{
	XML::IXMLNode *l_pAlgorithmNode  = XML::createNode(c_sTypeNodeName);
	l_pAlgorithmNode->addAttribute(c_sLDAConfigFileVersionAttributeName, "1");

	// Write the classifier to an .xml
	std::stringstream l_sClasses;

	for(size_t i = 0; i< getClassCount() ; ++i)
	{
		l_sClasses << m_vLabelList[i] << " ";
	}

	//Only new version should be recorded so we don't need to test
	XML::IXMLNode *l_pHelpersConfiguration = XML::createNode(c_sComputationHelpersConfigurationNode);
	for(size_t i = 0; i < m_vDiscriminantFunctions.size() ; ++i)
	{
		l_pHelpersConfiguration->addChild(m_vDiscriminantFunctions[i].getConfiguration());
	}

	XML::IXMLNode *l_pTempNode = XML::createNode(c_sClassesNodeName);
	l_pTempNode->setPCData(l_sClasses.str().c_str());
	l_pAlgorithmNode->addChild(l_pTempNode);
	l_pAlgorithmNode->addChild(l_pHelpersConfiguration);

	return l_pAlgorithmNode;
}

//Extract a float64 from the PCDATA of a node
float64 getFloatFromNode(XML::IXMLNode *pNode)
{
	std::stringstream l_sData(pNode->getPCData());
	float64 res;
	l_sData >> res;

	return res;
}

boolean CAlgorithmClassifierLDA::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	//If the attribute exist, we deal with the new version
	if(pConfigurationNode->hasAttribute(c_sLDAConfigFileVersionAttributeName))
	{
		m_bv1Classification = false;
	}
	else
	{
		m_bv1Classification = true;
	}

	m_vLabelList.clear();
	m_vDiscriminantFunctions.clear();

	XML::IXMLNode* l_pTempNode;

	if((l_pTempNode = pConfigurationNode->getChildByName(c_sClassesNodeName)) == NULL)
	{
		return false;
	}
	loadClassesFromNode(l_pTempNode);

	if(m_bv1Classification)
	{
		if((l_pTempNode = pConfigurationNode->getChildByName(c_sCoefficientsNodeName)) == NULL)
		{
			return false;
		}
		loadCoefficientsFromNode(l_pTempNode);

		if((l_pTempNode = pConfigurationNode->getChildByName(c_sBiasDistanceNodeName)) == NULL)
		{
			return false;
		}
		m_f64BiasDistance = getFloatFromNode(l_pTempNode);

		if((l_pTempNode = pConfigurationNode->getChildByName(c_sCoefficientProbabilityNodeName)) == NULL)
		{
			return false;
		}
		m_f64w0 = getFloatFromNode(l_pTempNode);

		//Now we initialize the coefficients vector according to Weights and bias (distance)
		m_oCoefficients.resize(1, m_oWeights.cols()+1 );
		m_oCoefficients(0,0) = m_f64BiasDistance;
		m_oCoefficients.block(0,1,1,m_oWeights.cols()) = m_oWeights;
	}

	else
	{
		//We send corresponding data to the computation helper
		XML::IXMLNode* l_pConfigsNode = pConfigurationNode->getChildByName(c_sComputationHelpersConfigurationNode);

		for(size_t i = 0 ; i < l_pConfigsNode->getChildCount() ; ++i)
		{
			m_vDiscriminantFunctions.push_back(CAlgorithmLDADiscriminantFunction());
			m_vDiscriminantFunctions[i].loadConfiguration(l_pConfigsNode->getChild(i));
		}

	}
	return true;
}

void CAlgorithmClassifierLDA::loadClassesFromNode(XML::IXMLNode *pNode)
{
	std::stringstream l_sData(pNode->getPCData());
	float64 l_f64Temp;
	while(l_sData >> l_f64Temp)
	{
		m_vLabelList.push_back(l_f64Temp);
	}
}

//Load the weight vector
void CAlgorithmClassifierLDA::loadCoefficientsFromNode(XML::IXMLNode *pNode)
{
	std::stringstream l_sData(pNode->getPCData());

	std::vector < float64 > l_vCoefficients;
	float64 l_f64Value;
	while(l_sData >> l_f64Value)
	{
		l_vCoefficients.push_back(l_f64Value);
	}

	m_oWeights.resize(1,l_vCoefficients.size());
	m_ui32NumCols  = l_vCoefficients.size();
	for(size_t i=0; i<l_vCoefficients.size(); i++)
	{
		m_oWeights(0,i)=l_vCoefficients[i];
	}
}

#endif // TARGET_HAS_ThirdPartyEIGEN
