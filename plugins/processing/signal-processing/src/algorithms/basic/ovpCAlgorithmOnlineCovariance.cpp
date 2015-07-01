#if defined TARGET_HAS_ThirdPartyEIGEN

#include "ovpCAlgorithmOnlineCovariance.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

using namespace OpenViBEToolkit;

using namespace Eigen;

#define COV_DEBUG 0
#if COV_DEBUG
void CAlgorithmOnlineCovariance::dumpMatrix(OpenViBE::Kernel::ILogManager &rMgr, const MatrixXdRowMajor &mat, const CString &desc)
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
void CAlgorithmOnlineCovariance::dumpMatrix(OpenViBE::Kernel::ILogManager& /* rMgr */, const MatrixXdRowMajor& /*mat*/, const CString& /*desc*/) { }
#endif

OpenViBE::boolean CAlgorithmOnlineCovariance::initialize(void) 
{ 
	m_ui32Count = 0;

	return true;
} 

OpenViBE::boolean CAlgorithmOnlineCovariance::uninitialize(void) 
{ 
	return true;
} 

OpenViBE::boolean CAlgorithmOnlineCovariance::process(void) 
{
	// Note: The input parameters must have been set by the caller by now
	const TParameterHandler< float64 > ip_f64Shrinkage(getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_Shrinkage));
	const TParameterHandler< uint64 > ip_ui64UpdateMethod(getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_UpdateMethod));
	const TParameterHandler< IMatrix* > ip_pFeatureVectorSet(getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_InputVectors));
	TParameterHandler< IMatrix* > op_pMean(getOutputParameter(OVP_Algorithm_OnlineCovariance_OutputParameterId_Mean));
	TParameterHandler< IMatrix* > op_pCovarianceMatrix(getOutputParameter(OVP_Algorithm_OnlineCovariance_OutputParameterId_CovarianceMatrix));

	if(isInputTriggerActive(OVP_Algorithm_OnlineCovariance_Process_Reset))
	{
		if( ip_f64Shrinkage < 0.0) {
			this->getLogManager() << LogLevel_Error << "Min shrinkage parameter value is 0.0\n";
			return false;
		}

		if(ip_pFeatureVectorSet->getDimensionCount() !=2 ) 
		{
			this->getLogManager() << LogLevel_Error << "Feature vector set should have dim=2\n";
			return false;
		}

		const uint32 l_ui32nRows = ip_pFeatureVectorSet->getDimensionSize(0);
		const uint32 l_ui32nCols = ip_pFeatureVectorSet->getDimensionSize(1);
		if(l_ui32nRows<1 || l_ui32nCols<1) 
		{
			this->getLogManager() << LogLevel_Error << "Input matrix is too small, [" << l_ui32nRows << "x" << l_ui32nCols << "]\n";
			return false;
		}

		this->getLogManager() << LogLevel_Debug << "Using shrinkage coeff " << ip_f64Shrinkage << " ...\n";
		this->getLogManager() << LogLevel_Debug << "Using update method " << getTypeManager().getEnumerationEntryNameFromValue(OVP_TypeId_OnlineCovariance_UpdateMethod, ip_ui64UpdateMethod) << "\n";

		// Set the output buffers
		op_pMean->setDimensionCount(2);
		op_pMean->setDimensionSize(0, 1);
		op_pMean->setDimensionSize(1, l_ui32nCols);
		op_pCovarianceMatrix->setDimensionCount(2);
		op_pCovarianceMatrix->setDimensionSize(0, l_ui32nCols);
		op_pCovarianceMatrix->setDimensionSize(1, l_ui32nCols);

		// These keep the incremental estimates
		m_oIncrementalMean.resize(1, l_ui32nCols);
		m_oIncrementalMean.setZero();

		m_oIncrementalCov.resize(l_ui32nCols, l_ui32nCols);
		m_oIncrementalCov.setZero();

		m_ui32Count = 0;

	} 
	
	if(isInputTriggerActive(OVP_Algorithm_OnlineCovariance_Process_Update))
	{
		const uint32 l_ui32nRows = ip_pFeatureVectorSet->getDimensionSize(0);
		const uint32 l_ui32nCols = ip_pFeatureVectorSet->getDimensionSize(1);

		const float64 *l_pBuffer = ip_pFeatureVectorSet->getBuffer();
		if(!l_pBuffer)
		{
			this->getLogManager() << LogLevel_Error << "Feature set buffer ptr is NULL\n";
			return false;
		}

		// Increment sample counts
		const uint32 l_ui32CountBefore = m_ui32Count;
		const uint32 l_ui32CountChunk = l_ui32nRows;
		const uint32 l_ui32CountAfter = l_ui32CountBefore + l_ui32CountChunk;

		// Cast our data into an Eigen matrix. As Eigen doesn't have const float64* constructor, we cast away the const.
		const Map<MatrixXdRowMajor> l_oSampleChunk(const_cast<float64*>(l_pBuffer),l_ui32nRows,l_ui32nCols); 

		// Update the mean & cov estimates

		if(ip_ui64UpdateMethod==OVP_TypeId_OnlineCovariance_UpdateMethod_ChunkAverage.toUInteger())
		{
			// 'Average of per-chunk covariance matrices'; this might not be proper cov over the dataset, but seems 
			// occasionally produce nicely smoothed results when used for CSP

			const MatrixXd l_oSampleMean = l_oSampleChunk.colwise().mean();
			const MatrixXd l_oSampleCentered = l_oSampleChunk.rowwise() - l_oSampleMean.row(0);

			const MatrixXd l_oSampleCenteredMean = l_oSampleCentered.colwise().mean();
			
			// dumpMatrix(this->getLogManager(), l_oSampleChunk, "SampleChunk");
			// dumpMatrix(this->getLogManager(), l_oSampleCenteredMean, "SampleCenteredMean");

			const MatrixXd l_oSampleCov = (1.0/float64(l_ui32nRows)) * l_oSampleCentered.transpose() * l_oSampleCentered;

			m_oIncrementalMean += l_oSampleMean;
			m_oIncrementalCov += l_oSampleCov;

			m_ui32Count++;
		}
		else if(ip_ui64UpdateMethod == OVP_TypeId_OnlineCovariance_UpdateMethod_Incremental.toUInteger())
		{
			// Incremental sample-per-sample cov updating.
			// It should be implementing the Youngs & Cramer algorithm as described in 
			// Chan, Golub, Leveq, "Updating formulae and a pairwise algorithm...", 1979

			uint32 l_ui32Start=0;
			if(m_ui32Count==0)
			{
				// dumpMatrix(this->getLogManager(), l_oSampleChunk, "Sample");

				m_oIncrementalMean = l_oSampleChunk.row(0);
				l_ui32Start = 1;
				m_ui32Count = 1;
			}


			for(uint32 i=l_ui32Start;i<l_ui32nRows;i++)
			{
				m_oIncrementalMean += l_oSampleChunk.row(i);

				const MatrixXd l_oDiff = (m_ui32Count+1)*l_oSampleChunk.row(i) - m_oIncrementalMean;
				const MatrixXd l_oOuterProd = l_oDiff.transpose()*l_oDiff;

				m_oIncrementalCov += 1.0 / (m_ui32Count*(m_ui32Count+1)) * l_oOuterProd;

				m_ui32Count++;
			}
		}
#if 0
		else if(l_ui32Method == 2)
		{
			const MatrixXd l_oSampleSum = l_oSampleChunk.colwise().sum();

			// Center the chunk
			const MatrixXd l_oSampleCentered = l_oSampleChunk.rowwise() - l_oSampleSum.row(0)*(1.0/(float64)l_ui32CountChunk);

			const MatrixXd l_oSampleCoMoment = (l_oSampleCentered.transpose() * l_oSampleCentered);

			m_oIncrementalCov = m_oIncrementalCov + l_oSampleCoMoment;
		
			if(l_ui32CountBefore>0)
			{
				const MatrixXd l_oMeanDifference = (l_ui32CountChunk/(float64)l_ui32CountBefore) * m_oIncrementalMean - l_oSampleSum;
				const MatrixXd l_oMeanDiffOuterProduct =  l_oMeanDifference.transpose()*l_oMeanDifference;

				m_oIncrementalCov += l_oMeanDiffOuterProduct*l_ui32CountBefore/(l_ui32CountChunk*l_ui32CountAfter);
			}

			m_oIncrementalMean = m_oIncrementalMean + l_oSampleSum;

			m_ui32Count = l_ui32CountAfter;
		}
		else
		{
			
			// Insert our data into an Eigen matrix. As Eigen doesn't have const float64* constructor, we cast away the const.
			const Map<MatrixXdRowMajor> l_oDataMatrix(const_cast<float64*>(l_pBuffer),l_ui32nRows,l_ui32nCols); 

			// Estimate the current sample means
			const MatrixXdRowMajor l_oSampleMean = l_oDataMatrix.colwise().mean();	
		
			// Center the current data with the previous(!) mean
			const MatrixXdRowMajor l_oSampleCentered = l_oDataMatrix.rowwise() - m_oIncrementalMean.row(0);

			// Estimate the current covariance
			const MatrixXd l_oSampleCov = (l_oSampleCentered.transpose() * l_oSampleCentered) * (1.0/(double)l_ui32nRows);

			// fixme: recheck the weights ...
			
			// Update the global mean and cov
			if(l_ui32CountBefore>0)
			{
				m_oIncrementalMean = ( m_oIncrementalMean*l_ui32CountBefore + l_oSampleMean*l_ui32nRows) / (float64)l_ui32CountAfter;
				m_oIncrementalCov = ( m_oIncrementalCov*l_ui32CountBefore + l_oSampleCov*(l_ui32CountBefore/(float64)l_ui32CountAfter) ) / (float64)l_ui32CountAfter;
			} 
			else
			{
				m_oIncrementalMean = l_oSampleMean;
				m_oIncrementalCov = l_oSampleCov;
			}


			m_ui32Count = l_ui32CountAfter;
		}
#endif
		else
		{
			this->getLogManager() << LogLevel_Error << "Unknown update method " << CIdentifier(ip_ui64UpdateMethod) << "\n";
			return false;
		}
	} 
		
	// Give output with regularization (mix prior + cov)
	if(isInputTriggerActive(OVP_Algorithm_OnlineCovariance_Process_GetCov))
	{
		const uint32 l_ui32nRows = ip_pFeatureVectorSet->getDimensionSize(0);
		const uint32 l_ui32nCols = ip_pFeatureVectorSet->getDimensionSize(1);

		if(!m_ui32Count) {
			this->getLogManager() << LogLevel_Error << "Not enough samples to compute covariance\n";
			return false;
		}

		// Converters to CMatrix
		Map<MatrixXdRowMajor> l_oOutputMean(op_pMean->getBuffer(),1,l_ui32nCols); 
		Map<MatrixXdRowMajor> l_oOutputCov(op_pCovarianceMatrix->getBuffer(),l_ui32nCols,l_ui32nCols); 

		// Compute the prior cov matrix
		MatrixXd l_oPriorCov;
		l_oPriorCov.resizeLike(m_oIncrementalCov);
		l_oPriorCov.setIdentity();
		l_oPriorCov *= ip_f64Shrinkage;

		// Mix the prior and the sample estimates according to the shrinkage parameter. We scale by 1/n to normalize
		l_oOutputMean = m_oIncrementalMean / (float64)m_ui32Count;
		l_oOutputCov = 	l_oPriorCov + m_oIncrementalCov / (float64)m_ui32Count;

		// Debug block
		dumpMatrix(this->getLogManager(), l_oOutputMean, "Data mean");
		dumpMatrix(this->getLogManager(), m_oIncrementalCov/(float64)m_ui32Count, "Data cov");
		dumpMatrix(this->getLogManager(), l_oPriorCov, "Prior cov");
		dumpMatrix(this->getLogManager(), l_oOutputCov, "Output cov");

	}

	// Give just the output with no shrinkage?
	if(isInputTriggerActive(OVP_Algorithm_OnlineCovariance_Process_GetCovRaw))
	{
		const uint32 l_ui32nRows = ip_pFeatureVectorSet->getDimensionSize(0);
		const uint32 l_ui32nCols = ip_pFeatureVectorSet->getDimensionSize(1);

		if(!m_ui32Count) {
			this->getLogManager() << LogLevel_Error << "Not enough samples to compute covariance\n";
			return false;
		}

		// Converters to CMatrix
		Map<MatrixXdRowMajor> l_oOutputMean(op_pMean->getBuffer(),1,l_ui32nCols); 
		Map<MatrixXdRowMajor> l_oOutputCov(op_pCovarianceMatrix->getBuffer(),l_ui32nCols,l_ui32nCols); 

		// We scale by 1/n to normalize
		l_oOutputMean = m_oIncrementalMean / (float64)m_ui32Count;
		l_oOutputCov = 	m_oIncrementalCov / (float64)m_ui32Count;

		// Debug block
		dumpMatrix(this->getLogManager(), l_oOutputMean, "Data mean");
		dumpMatrix(this->getLogManager(), l_oOutputCov, "Data Cov");

	}

	return true;
}

#endif // TARGET_HAS_ThirdPartyEIGEN
