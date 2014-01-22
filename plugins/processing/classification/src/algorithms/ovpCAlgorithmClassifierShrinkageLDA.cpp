#include "ovpCAlgorithmClassifierShrinkageLDA.h"

#if defined TARGET_HAS_ThirdPartyEIGEN

#include <map>
#include <sstream>

#include <system/Memory.h>

#include <Eigen/Eigenvalues>

#include "../algorithms/ovpCAlgorithmConditionedCovariance.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

using namespace Eigen;

#define LDA_DEBUG 0
#if LDA_DEBUG
void CAlgorithmClassifierShrinkageLDA::dumpMatrix(OpenViBE::Kernel::ILogManager &rMgr, const MatrixXdRowMajor &mat, const CString &desc)
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
void CAlgorithmClassifierShrinkageLDA::dumpMatrix(OpenViBE::Kernel::ILogManager& /* rMgr */, const MatrixXdRowMajor& /*mat*/, const CString& /*desc*/) { };
#endif

boolean CAlgorithmClassifierShrinkageLDA::initialize(void)
{
	// Initialize the Conditioned Covariance Matrix algorithm
	m_pCovarianceAlgorithm = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ConditionedCovariance));
	m_pCovarianceAlgorithm->initialize();

	// This is the weight parameter local to this module and automatically exposed to the GUI. Its redirected to the corresponding parameter of the cov alg.
	TParameterHandler< float64 > ip_f64Shrinkage(this->getInputParameter(OVP_Algorithm_ClassifierShrinkageLDA_InputParameterId_Shrinkage));
	ip_f64Shrinkage.setReferenceTarget(m_pCovarianceAlgorithm->getInputParameter(OVP_Algorithm_ConditionedCovariance_InputParameterId_Shrinkage));

	TParameterHandler < boolean > ip_bDiagonalCov(this->getInputParameter(OVP_Algorithm_ClassifierShrinkageLDA_InputParameterId_DiagonalCov));
	ip_bDiagonalCov = false;

	return CAlgorithmClassifier::initialize();
}

boolean CAlgorithmClassifierShrinkageLDA::uninitialize(void)
{
	m_pCovarianceAlgorithm->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_pCovarianceAlgorithm);

	return CAlgorithmClassifier::uninitialize();
}

boolean CAlgorithmClassifierShrinkageLDA::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	TParameterHandler< boolean > ip_bDiagonalCov(getInputParameter(OVP_Algorithm_ClassifierShrinkageLDA_InputParameterId_DiagonalCov));

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

		if(ip_bDiagonalCov) 
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

		dumpMatrix(this->getLogManager(), l_aMean[l_ui32classIdx], "Mean");
		dumpMatrix(this->getLogManager(), l_aCov[l_ui32classIdx], "Shrinked cov");
	}

	l_oGlobalCov /= (double)l_ui32nClasses;

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

	// Build LDA model for 2 classes. This is a special case of the multiclass version.
	const MatrixXd l_oGlobalCovInv = l_oEigenSolver.eigenvectors() * l_oEigenValues.asDiagonal() * l_oEigenSolver.eigenvectors().inverse();	

	const MatrixXd l_oMeanSum  = l_aMean[0] + l_aMean[1];
	const MatrixXd l_oMeanDiff = l_aMean[0] - l_aMean[1];

	const MatrixXd l_oBias = -0.5 * l_oMeanSum * l_oGlobalCovInv * l_oMeanDiff.transpose();
	const MatrixXd l_oWeights = l_oGlobalCovInv * l_oMeanDiff.transpose();

	// Catenate the bias term and the weights
	m_oCoefficients.resize(1, l_ui32nCols+1 );
	m_oCoefficients(0,0) = l_oBias(0,0);
	m_oCoefficients.block(0,1,1,l_ui32nCols) = l_oWeights.transpose();

	// Write the classifier to an .xml
	std::stringstream l_sClasses;
	std::stringstream l_sCoefficients;

	l_sClasses << m_f64Class1 << " " << m_f64Class2;
	l_sCoefficients << std::scientific;
	for(uint32 i=0; i<l_ui32nCols+1; i++)
	{
		l_sCoefficients << " " << m_oCoefficients(0,i);
	}

	m_oConfiguration.setSize(0, true);
	XML::IWriter* l_pWriter=XML::createWriter(*this);
	l_pWriter->openChild("OpenViBE-Classifier");
	 l_pWriter->openChild("LDA");
	  l_pWriter->openChild("Creator");
	   l_pWriter->setChildData("ShrinkageLDA");
	  l_pWriter->closeChild();
	  l_pWriter->openChild("Classes");
	   l_pWriter->setChildData(l_sClasses.str().c_str());
	  l_pWriter->closeChild();
	  l_pWriter->openChild("Coefficients");
	   l_pWriter->setChildData(l_sCoefficients.str().c_str());
	  l_pWriter->closeChild();
	 l_pWriter->closeChild();
	l_pWriter->closeChild();
	l_pWriter->release();
	l_pWriter=NULL;
	
	// Debug output
	dumpMatrix(this->getLogManager(), l_oGlobalCov, "Global cov");
	dumpMatrix(this->getLogManager(), l_oEigenValues, "Eigenvalues");
	dumpMatrix(this->getLogManager(), l_oEigenSolver.eigenvectors(), "Eigenvectors");
	dumpMatrix(this->getLogManager(), l_oGlobalCovInv, "Global cov inverse");
	dumpMatrix(this->getLogManager(), m_oCoefficients, "Hyperplane weights");

	return true;
}

boolean CAlgorithmClassifierShrinkageLDA::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
{
	const uint32 l_ui32nColsWithBiasTerm = m_oCoefficients.size();

	if(rFeatureVector.getSize()+1!=l_ui32nColsWithBiasTerm)
	{
		this->getLogManager() << LogLevel_Warning << "Feature vector size " << rFeatureVector.getSize() << " + 1 and hyperplane parameter size " << l_ui32nColsWithBiasTerm << " do not match\n";
		return false;
	}

	const Map<MatrixXdRowMajor> l_oFeatureVec(const_cast<float64*>(rFeatureVector.getBuffer()), 1, rFeatureVector.getSize());

	// Catenate 1.0 to match the bias term
	MatrixXd l_oWeights(1, l_ui32nColsWithBiasTerm);
	l_oWeights(0,0) = 1.0;
	l_oWeights.block(0,1,1,l_ui32nColsWithBiasTerm-1) = l_oFeatureVec;

	const float64 l_f64Result = (l_oWeights*m_oCoefficients.transpose()).col(0)(0);

	rClassificationValues.setSize(1);
	rClassificationValues[0]= -l_f64Result;

	if(l_f64Result >= 0)
	{
		rf64Class=m_f64Class1;
	}
	else
	{
		rf64Class=m_f64Class2;
	}

	return true;
}

uint32 CAlgorithmClassifierShrinkageLDA::getBestClassification(IMatrix& rFirstClassificationValue, IMatrix& rSecondClassificationValue)
{
    return 0;
}

boolean CAlgorithmClassifierShrinkageLDA::saveConfiguration(IMemoryBuffer& rMemoryBuffer)
{
	rMemoryBuffer.setSize(0, true);
	rMemoryBuffer.append(m_oConfiguration);
	return true;
}

boolean CAlgorithmClassifierShrinkageLDA::loadConfiguration(const IMemoryBuffer& rMemoryBuffer)
{
	m_f64Class1=0;
	m_f64Class2=0;

	XML::IReader* l_pReader=XML::createReader(*this);
	l_pReader->processData(rMemoryBuffer.getDirectPointer(), rMemoryBuffer.getSize());
	l_pReader->release();
	l_pReader=NULL;

	return true;
}

void CAlgorithmClassifierShrinkageLDA::write(const char* sString)
{
	m_oConfiguration.append((const uint8*)sString, ::strlen(sString));
}

void CAlgorithmClassifierShrinkageLDA::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{
	m_vNode.push(sName);
}

void CAlgorithmClassifierShrinkageLDA::processChildData(const char* sData)
{
	std::stringstream l_sData(sData);

	if(m_vNode.top()==CString("Classes"))
	{
		l_sData >> m_f64Class1;
		l_sData >> m_f64Class2;
	}

	if(m_vNode.top()==CString("Coefficients"))
	{
		std::vector < float64 > l_vCoefficients;
		while(!l_sData.eof())
		{
			float64 l_f64Value;
			l_sData >> l_f64Value;
			l_vCoefficients.push_back(l_f64Value);
		}

		m_oCoefficients.resize(1,l_vCoefficients.size());
		for(size_t i=0; i<l_vCoefficients.size(); i++)
		{
			m_oCoefficients(0,i)=l_vCoefficients[i];
		}
	}
}

void CAlgorithmClassifierShrinkageLDA::closeChild(void)
{
	m_vNode.pop();
}

#endif // TARGET_HAS_ThirdPartyEIGEN
