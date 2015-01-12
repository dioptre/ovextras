#include "ovpCAlgorithmClassifierPShrinkageLDA.h"

#if defined TARGET_HAS_ThirdPartyEIGEN

#include <map>
#include <sstream>
#include <iostream>
#include <cmath>

#include <system/ovCMemory.h>
#include <xml/IXMLHandler.h>

#include <Eigen/Eigenvalues>

#include "../algorithms/ovpCAlgorithmConditionedCovariance.h"

static const char* const c_sTypeNodeName = "PShrinkage-LDA";
static const char* const c_sCreatorNodeName = "Creator";
static const char* const c_sClassesNodeName = "Classes";
static const char* const c_sCoeffNodeName = "Coefficients";
static const char* const c_sCoeffWNodeName = "CoefficientsW";
static const char* const c_sCoeffW0NodeName = "CoefficientsW0";

extern const char* const c_sClassifierRoot;

OpenViBE::int32 OpenViBEPlugins::Classification::getPShrinkageLDABestClassification(OpenViBE::IMatrix& rFirstClassificationValue, OpenViBE::IMatrix& rSecondClassificationValue)
{
	if(ov_float_equal(rFirstClassificationValue[0], ::fabs(rSecondClassificationValue[0])))
		return 0;
	else if(::fabs(rFirstClassificationValue[0]) > ::fabs(rSecondClassificationValue[0]))
		return -1;
	else
		return 1;
}

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

using namespace Eigen;

#define PLDA_DEBUG 1
#if PLDA_DEBUG
void CAlgorithmClassifierPShrinkageLDA::dumpMatrix(OpenViBE::Kernel::ILogManager &rMgr, const MatrixXdRowMajor &mat, const CString &desc)
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
void CAlgorithmClassifierPShrinkageLDA::dumpMatrix(OpenViBE::Kernel::ILogManager& /* rMgr */, const MatrixXdRowMajor& /*mat*/, const CString& /*desc*/) { };
#endif

boolean CAlgorithmClassifierPShrinkageLDA::initialize(void)
{
	// Initialize the Conditioned Covariance Matrix algorithm
	m_pCovarianceAlgorithm = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ConditionedCovariance));
	m_pCovarianceAlgorithm->initialize();

	// This is the weight parameter local to this module and automatically exposed to the GUI. Its redirected to the corresponding parameter of the cov alg.
	TParameterHandler< float64 > ip_f64Shrinkage(this->getInputParameter(OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_Shrinkage));
	ip_f64Shrinkage.setReferenceTarget(m_pCovarianceAlgorithm->getInputParameter(OVP_Algorithm_ConditionedCovariance_InputParameterId_Shrinkage));

	TParameterHandler < boolean > ip_bDiagonalCov(this->getInputParameter(OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_DiagonalCov));
	ip_bDiagonalCov = false;

	TParameterHandler < XML::IXMLNode* > op_pConfiguration(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	op_pConfiguration=NULL;

	return CAlgorithmClassifier::initialize();
}

boolean CAlgorithmClassifierPShrinkageLDA::uninitialize(void)
{
	m_pCovarianceAlgorithm->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_pCovarianceAlgorithm);

	return CAlgorithmClassifier::uninitialize();
}

boolean CAlgorithmClassifierPShrinkageLDA::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	//Let's set the extra settings
	TParameterHandler < std::map<CString, CString>* > ip_pExtraParameter(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));
	std::map<CString, CString>* l_pExtraParameter = ip_pExtraParameter;

	IAlgorithmProxy *l_pAlgoProxy = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ClassifierPShrinkageLDA));
	l_pAlgoProxy->initialize();

	//Extract OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_Shrinkage
	CString l_pParameterName = l_pAlgoProxy->getInputParameterName(OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_Shrinkage);
	this->getFloat64Parameter(OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_Shrinkage, (*l_pExtraParameter)[l_pParameterName]);

	//Extract OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_DiagonalCov
	l_pParameterName = l_pAlgoProxy->getInputParameterName(OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_DiagonalCov);
	boolean l_pDiagonalCov = this->getBooleanParameter(OVP_Algorithm_ClassifierPShrinkageLDA_InputParameterId_DiagonalCov, (*l_pExtraParameter)[l_pParameterName]);

	l_pAlgoProxy->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*l_pAlgoProxy);


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
	std::map < float64, uint32 > l_vClassLabels;
	for(uint32 i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		l_vClassLabels[rFeatureVectorSet[i].getLabel()]++;
	}

	if(l_vClassLabels.size() != 2)
	{
		this->getLogManager() << LogLevel_Error << "A LDA classifier can only be trained with 2 classes, not more, not less - got " << (uint32)l_vClassLabels.size() << "\n";
		return false;
	}

	static const uint32 l_ui32nClasses = 2;

	// Get class labels
	m_f64Class1=l_vClassLabels.begin()->first;
	m_f64Class2=l_vClassLabels.rbegin()->first;

	// Get regularized covariances of all the classes
	const float64 l_f64Labels[] = {m_f64Class1,m_f64Class2};
	MatrixXd l_aCov[l_ui32nClasses];
	MatrixXd l_aMean[l_ui32nClasses];
	MatrixXd l_oGlobalCov = MatrixXd::Zero(l_ui32nCols,l_ui32nCols);

	for(uint32 l_ui32classIdx=0;l_ui32classIdx<l_ui32nClasses;l_ui32classIdx++)
	{
		const float64 l_f64Label = l_f64Labels[l_ui32classIdx];
		const uint32 l_ui32nExamplesInClass = l_vClassLabels[l_f64Label];

		// Copy all the data of the class to a feature matrix
		ip_pFeatureVectorSet->setDimensionCount(2);
		ip_pFeatureVectorSet->setDimensionSize(0, l_ui32nExamplesInClass);
		ip_pFeatureVectorSet->setDimensionSize(1, l_ui32nCols);
		float64 *l_pBuffer = ip_pFeatureVectorSet->getBuffer();
		for(uint32 i=0;i<l_ui32nRows;i++)
		{
			if(rFeatureVectorSet[i].getLabel() == l_f64Label)
			{
				System::Memory::copy(l_pBuffer, rFeatureVectorSet[i].getBuffer(), l_ui32nCols*sizeof(float64));
				l_pBuffer += l_ui32nCols;
			}
		}

		// Compute mean and cov
		if(!m_pCovarianceAlgorithm->process()) {
			this->getLogManager() << LogLevel_Error << "Covariance computation failed for class " << l_ui32classIdx << " ("<< l_f64Label << ")\n";
			return false;
		}

		// Get the results from the cov algorithm
		Map<MatrixXdRowMajor> l_oMeanMapper(op_pMean->getBuffer(), 1, l_ui32nCols);
		l_aMean[l_ui32classIdx] = l_oMeanMapper;
		Map<MatrixXdRowMajor> l_oCovMapper(op_pCovarianceMatrix->getBuffer(), l_ui32nCols, l_ui32nCols);
		l_aCov[l_ui32classIdx] = l_oCovMapper;

		if(l_pDiagonalCov)
		{
			for(uint32 i=0;i<l_ui32nCols;i++)
			{
				for(uint32 j=i+1;j<l_ui32nCols;j++)
				{
					l_aCov[l_ui32classIdx](i,j) = 0.0;
					l_aCov[l_ui32classIdx](j,i) = 0.0;
				}
			}
		}
		l_oGlobalCov += l_aCov[l_ui32classIdx];

//		dumpMatrix(this->getLogManager(), l_aMean[l_ui32classIdx], "Mean");
//		dumpMatrix(this->getLogManager(), l_aCov[l_ui32classIdx], "Shrinked cov");
	}

	l_oGlobalCov /= static_cast<float64>(l_ui32nClasses);

	const float64 l_f64Tolerance = 1e-10;
	// Get the pseudoinverse of the global cov using eigen decomposition for self-adjoint matrices
	SelfAdjointEigenSolver<MatrixXd> l_oEigenSolver;
	l_oEigenSolver.compute(l_oGlobalCov);
	VectorXd l_oEigenValues = l_oEigenSolver.eigenvalues();
	for(uint32 i=0;i<l_ui32nCols;i++) {
		if(l_oEigenValues(i) >= l_f64Tolerance) {
			l_oEigenValues(i) = 1.0/l_oEigenValues(i);
		}
	}

	// Build LDA model for 2 classes. This is a special case of the multiclass version.
	const MatrixXd l_oGlobalCovInv = l_oEigenSolver.eigenvectors() * l_oEigenValues.asDiagonal() * l_oEigenSolver.eigenvectors().inverse();

	const MatrixXd l_oClass1 = -0.5 * l_aMean[0] * l_oGlobalCovInv * l_aMean[0].transpose();
	const MatrixXd l_oClass2 = 0.5 * l_aMean[1] * l_oGlobalCovInv * l_aMean[1].transpose();
	// Catenate the bias term and the weights

	m_f64w0 = l_oClass1(0,0) + l_oClass2(0,0) +
			std::log(static_cast<double>(l_vClassLabels[m_f64Class1])/static_cast<double>(l_vClassLabels[m_f64Class2]));
	m_oW = (l_oGlobalCovInv * (l_aMean[0] - l_aMean[1]).transpose()).transpose();

	m_ui32NumCols = l_ui32nCols;

	// Debug output
	dumpMatrix(this->getLogManager(), l_oGlobalCov, "Global cov");
	dumpMatrix(this->getLogManager(), l_oEigenValues, "Eigenvalues");
	dumpMatrix(this->getLogManager(), l_oEigenSolver.eigenvectors(), "Eigenvectors");
	dumpMatrix(this->getLogManager(), l_oGlobalCovInv, "Global cov inverse");
	dumpMatrix(this->getLogManager(), m_oW, "Coeff");

	return true;
}

boolean CAlgorithmClassifierPShrinkageLDA::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues, IVector& rProbabilityValue)
{
	const uint32 l_ui32nCols = m_ui32NumCols;

	if(rFeatureVector.getSize() != l_ui32nCols)
	{
		this->getLogManager() << LogLevel_Warning << "Feature vector size " << rFeatureVector.getSize() << " and hyperplane parameter size " << l_ui32nCols << " do not match\n";
		return false;
	}

	const Map<MatrixXdRowMajor> l_oFeatureVec(const_cast<float64*>(rFeatureVector.getBuffer()), 1, rFeatureVector.getSize());

	// Catenate 1.0 to match the bias term
	MatrixXd l_oWeights(1, l_ui32nCols);
	l_oWeights.block(0,0,1, l_ui32nCols) = l_oFeatureVec;

	std::cout << l_oWeights.cols() << " " << m_oW.cols() << std::endl;

	const float64 l_f64a =(m_oW * l_oWeights.transpose()).col(0)(0) + m_f64w0;
	const float64 l_f64P1 = 1 / (1 + exp(-l_f64a));

	rClassificationValues.setSize(1);
	rClassificationValues[0]= l_f64P1;
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

void CAlgorithmClassifierPShrinkageLDA::generateConfigurationNode(void)
{
	// Write the classifier to an .xml
	std::stringstream l_sClasses;
	std::stringstream l_sCoefficientsW;
	std::stringstream l_sCoefficientsW0;

	l_sClasses << m_f64Class1 << " " << m_f64Class2;
	l_sCoefficientsW0 << std::scientific << m_f64w0;
	l_sCoefficientsW << std::scientific << m_oW(0,0);

	for(size_t i=1; i<m_ui32NumCols; i++)
	{
		l_sCoefficientsW << " " << m_oW(0,i);
	}

	XML::IXMLNode *l_pCreatorNode = XML::createNode(c_sCreatorNodeName);
	l_pCreatorNode->setPCData("PShrinkageLDA");
	XML::IXMLNode *l_pClassesNode = XML::createNode(c_sClassesNodeName);
	l_pClassesNode->setPCData(l_sClasses.str().c_str());

	XML::IXMLNode *l_pAlgorithmNode  = XML::createNode(c_sTypeNodeName);
	l_pAlgorithmNode->addChild(l_pCreatorNode);
	l_pAlgorithmNode->addChild(l_pClassesNode);

	XML::IXMLNode *l_pCoeffNode = XML::createNode(c_sCoeffNodeName);
	XML::IXMLNode *l_pTempNode = XML::createNode(c_sCoeffW0NodeName);
	l_pTempNode->setPCData(l_sCoefficientsW0.str().c_str());
	l_pCoeffNode->addChild(l_pTempNode);

	l_pTempNode = XML::createNode(c_sCoeffWNodeName);
	l_pTempNode->setPCData(l_sCoefficientsW.str().c_str());
	l_pCoeffNode->addChild(l_pTempNode);

	l_pAlgorithmNode->addChild(l_pCoeffNode);

	m_pConfigurationNode = XML::createNode(c_sClassifierRoot);
	m_pConfigurationNode->addChild(l_pAlgorithmNode);
}

XML::IXMLNode* CAlgorithmClassifierPShrinkageLDA::saveConfiguration(void)
{
	generateConfigurationNode();
	return m_pConfigurationNode;
}

boolean CAlgorithmClassifierPShrinkageLDA::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	m_f64Class1=0;
	m_f64Class2=0;

	loadClassesFromNode(pConfigurationNode->getChild(0)->getChildByName("Classes"));
	loadCoefficientsFromNode(pConfigurationNode->getChild(0)->getChildByName("Coefficients"));

	return true;
}

void CAlgorithmClassifierPShrinkageLDA::loadClassesFromNode(XML::IXMLNode *pNode)
{
	std::stringstream l_sData(pNode->getPCData());

	l_sData >> m_f64Class1;
	l_sData >> m_f64Class2;
}

void CAlgorithmClassifierPShrinkageLDA::loadCoefficientsFromNode(XML::IXMLNode *pNode)
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

	m_oW.resize(1,l_vCoefficientsW.size());
	m_ui32NumCols = l_vCoefficientsW.size();
	for(size_t i=0; i<l_vCoefficientsW.size(); i++)
	{
		m_oW(0,i)=l_vCoefficientsW[i];
	}
}

#endif // TARGET_HAS_ThirdPartyEIGEN
