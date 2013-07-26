#include "ovpCAlgorithmCovariance.h"

#if defined TARGET_HAS_ThirdPartyEIGEN

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

using namespace Eigen;

#define COV_DEBUG 0
#if COV_DEBUG
void CAlgorithmCovariance::dumpMatrix(OpenViBE::Kernel::ILogManager &pMgr, const MatrixXdRowMajor &mat, const CString &desc)
{
	pMgr << LogLevel_Info << desc << "\n";
	for(int i=0;i<mat.rows();i++) {
		pMgr << LogLevel_Info << "Row " << i << ": ";
		for(int j=0;j<mat.cols();j++) {
			pMgr << mat(i,j) << " ";
		}
		pMgr << "\n";
	}
}
#else 
void CAlgorithmCovariance::dumpMatrix(OpenViBE::Kernel::ILogManager& /* pMgr */, const MatrixXdRowMajor& /*mat*/, const CString& /*desc*/) { };
#endif

OpenViBE::boolean CAlgorithmCovariance::initialize(void) 
{ 
	// Default value setting
	OpenViBE::Kernel::TParameterHandler < float64 > ip_f64Shrinkage(getInputParameter(OVP_Algorithm_Covariance_InputParameterId_Shrinkage));
	ip_f64Shrinkage = -1.0;

	return true;
} 

OpenViBE::boolean CAlgorithmCovariance::uninitialize(void) 
{ 
	return true;
} 

OpenViBE::boolean CAlgorithmCovariance::process(void) 
{
	// Set up the IO
	const TParameterHandler< float64 >  ip_f64Shrinkage(getInputParameter(OVP_Algorithm_Covariance_InputParameterId_Shrinkage));
	TParameterHandler< IMatrix* > ip_pFeatureVectorSet(getInputParameter(OVP_Algorithm_Covariance_InputParameterId_FeatureVectorSet));
	TParameterHandler< IMatrix* > op_pCovarianceMatrix(getOutputParameter(OVP_Algorithm_Covariance_OutputParameterId_CovarianceMatrix));
	float64 l_f64Shrinkage = ip_f64Shrinkage;
	if(l_f64Shrinkage>1.0) {
		this->getLogManager() << LogLevel_Error << "Max shrinkage parameter value is 1.0\n";
		return false;
	}
	
	if(ip_pFeatureVectorSet->getDimensionCount() !=2 ) 
	{
		this->getLogManager() << LogLevel_Error << "Feature vector set should have dim=2\n";
		return false;
	}

	const uint32 l_ui32nRows = ip_pFeatureVectorSet->getDimensionSize(0);
	const uint32 l_ui32nCols = ip_pFeatureVectorSet->getDimensionSize(1);
	float64 *l_pBuffer = ip_pFeatureVectorSet->getBuffer();
	if(l_ui32nRows<1 || l_ui32nCols<1 || !l_pBuffer) 
	{
		this->getLogManager() << LogLevel_Error << "Input matrix is too small, [" << l_ui32nRows << "x" << l_ui32nCols << "]\n";
		return false;
	}

	// Insert our data into an Eigen matrix
	const Map<MatrixXdRowMajor> l_oDataMatrix(l_pBuffer,l_ui32nRows,l_ui32nCols); 

	dumpMatrix(this->getLogManager(), l_oDataMatrix, "Data");

	// Estimate the data center and center the data
	const VectorXd l_oDataMean = l_oDataMatrix.colwise().mean();	
	const MatrixXdRowMajor l_oDataCentered = l_oDataMatrix.rowwise() - l_oDataMean.transpose();

	dumpMatrix(this->getLogManager(), l_oDataCentered, "Centered data");

	// Compute the sample cov matrix
	const MatrixXd l_oSampleCov = (l_oDataCentered.transpose() * l_oDataCentered) * (1/(double)l_ui32nRows);

	dumpMatrix(this->getLogManager(), l_oSampleCov, "Sample cov");

	// Compute the prior cov matrix
	MatrixXd l_oPriorCov = MatrixXd::Zero(l_ui32nCols,l_ui32nCols);
	l_oPriorCov.diagonal().setConstant( l_oSampleCov.diagonal().mean() );

	dumpMatrix(this->getLogManager(), l_oPriorCov, "Prior cov");

	// Compute shrinkage coefficient if its not given
	if(l_f64Shrinkage<0) {
		const MatrixXd l_oDataSquared = l_oDataCentered.cwiseProduct(l_oDataCentered);

		MatrixXd l_oPhiMat = (l_oDataSquared.transpose()*l_oDataSquared) / (double)l_ui32nRows;
		l_oPhiMat -= 2*(1.0/(double)l_ui32nRows)*(l_oDataCentered.transpose()*l_oDataCentered).cwiseProduct(l_oSampleCov);
		l_oPhiMat += (l_oSampleCov.cwiseProduct(l_oSampleCov));

		const float64 l_f64phi = l_oPhiMat.sum();
		const float64 l_f64gamma = (l_oSampleCov - l_oPriorCov).squaredNorm();
		const float64 l_f64kappa = l_f64phi / l_f64gamma;

		l_f64Shrinkage = std::max<float64>(0,std::min<float64>(1,l_f64kappa/(double)l_ui32nRows));

		this->getLogManager() << LogLevel_Debug << "Estimated shrinkage weight as " << l_f64Shrinkage << "\n";

		dumpMatrix(this->getLogManager(), l_oPhiMat, "PhiMat");

		// y=x.^2;
		// phiMat=y'*y/t-2*(x'*x).*sample/t+sample.^2;
		// phi=sum(sum(phiMat)); 
		// gamma=norm(sample-prior,'fro')^2;
		// % compute shrinkage constant
		// kappa=phi/gamma;
		// shrinkage=max(0,min(1,kappa/t));
	}
	else 
	{
		this->getLogManager() << LogLevel_Info << "Using user-provided shrinkage weight " << l_f64Shrinkage << "\n";
	}

	// Mix the prior and the sample estimates according to the shrinkage parameter
	const MatrixXd l_oOutputCov = l_f64Shrinkage*l_oPriorCov + (1.0-l_f64Shrinkage)*l_oSampleCov;

	dumpMatrix(this->getLogManager(), l_oOutputCov, "Output cov");

	// Load the estimate to the output
	op_pCovarianceMatrix->setDimensionCount(2);
	op_pCovarianceMatrix->setDimensionSize(0, l_ui32nCols);
	op_pCovarianceMatrix->setDimensionSize(1, l_ui32nCols);

	for(uint32 cnt=0;cnt<l_ui32nCols*l_ui32nCols;cnt++) {
		uint32 i = (cnt / l_ui32nCols);
		uint32 j = (cnt % l_ui32nCols);
		op_pCovarianceMatrix->getBuffer()[cnt] = l_oOutputCov.coeff(i,j);
	}

	return true;
}

#endif // TARGET_HAS_ThirdPartyEIGEN
