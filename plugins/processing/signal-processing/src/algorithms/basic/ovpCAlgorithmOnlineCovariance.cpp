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
	m_ui64Count = 0;

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
	const TParameterHandler< boolean > ip_bTraceNormalization(getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_TraceNormalization));
	const TParameterHandler< uint64 > ip_ui64UpdateMethod(getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_UpdateMethod));
	const TParameterHandler< IMatrix* > ip_pFeatureVectorSet(getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_InputVectors));
	TParameterHandler< IMatrix* > op_pMean(getOutputParameter(OVP_Algorithm_OnlineCovariance_OutputParameterId_Mean));
	TParameterHandler< IMatrix* > op_pCovarianceMatrix(getOutputParameter(OVP_Algorithm_OnlineCovariance_OutputParameterId_CovarianceMatrix));

	if(isInputTriggerActive(OVP_Algorithm_OnlineCovariance_Process_Reset))
	{
		if( ip_f64Shrinkage < 0.0 || ip_f64Shrinkage > 1.0) {
			this->getLogManager() << LogLevel_Error << "Shrinkage parameter must be in range [0,1]\n";
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
		this->getLogManager() << LogLevel_Debug << "Trace normalization is " << (ip_bTraceNormalization ? "[on]" : "[off]") << "\n";
		this->getLogManager() << LogLevel_Debug << "Using update method " << getTypeManager().getEnumerationEntryNameFromValue(OVP_TypeId_OnlineCovariance_UpdateMethod, ip_ui64UpdateMethod) << "\n";
		
		// Set the output buffers
		op_pMean->setDimensionCount(2);
		op_pMean->setDimensionSize(0, 1);
		op_pMean->setDimensionSize(1, l_ui32nCols);
		op_pCovarianceMatrix->setDimensionCount(2);
		op_pCovarianceMatrix->setDimensionSize(0, l_ui32nCols);
		op_pCovarianceMatrix->setDimensionSize(1, l_ui32nCols);

		// These keep track of the non-normalized incremental estimates
		m_oIncrementalMean.resize(1, l_ui32nCols);
		m_oIncrementalMean.setZero();
		m_oIncrementalCov.resize(l_ui32nCols, l_ui32nCols);
		m_oIncrementalCov.setZero();

		m_ui64Count = 0;

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


		// Cast our data into an Eigen matrix. As Eigen doesn't have const float64* constructor, we cast away the const.
		const Map<MatrixXdRowMajor> l_oSampleChunk(const_cast<float64*>(l_pBuffer),l_ui32nRows,l_ui32nCols); 

		// Update the mean & cov estimates

		if(ip_ui64UpdateMethod==OVP_TypeId_OnlineCovariance_UpdateMethod_ChunkAverage.toUInteger())
		{
			// 'Average of per-chunk covariance matrices'. This might not be a proper cov over 
			// the dataset, but seems occasionally produce nicely smoothed results when used for CSP.

			const MatrixXd l_oChunkMean = l_oSampleChunk.colwise().mean();
			const MatrixXd l_oChunkCentered = l_oSampleChunk.rowwise() - l_oChunkMean.row(0);
			
			MatrixXd l_oChunkCov = (1.0/float64(l_ui32nRows)) * l_oChunkCentered.transpose() * l_oChunkCentered;

			if(ip_bTraceNormalization) 
			{
				// The origin of this trick has been lost. Presumably the idea is to normalize the scale of 
				// each chunk in order to compensate for possible signal power drift over time during the EEG recording, 
				// making each chunks' covariance contribute similarly to the average regardless of
				// the current average power. Such a normalization could also be implemented in its own 
				// box and not done here.

				l_oChunkCov = l_oChunkCov / l_oChunkCov.trace();
			}

			m_oIncrementalMean += l_oChunkMean;
			m_oIncrementalCov += l_oChunkCov;

			m_ui64Count++;

			// dumpMatrix(this->getLogManager(), l_oSampleChunk, "SampleChunk");
			// dumpMatrix(this->getLogManager(), l_oSampleCenteredMean, "SampleCenteredMean");
		}
		else if(ip_ui64UpdateMethod == OVP_TypeId_OnlineCovariance_UpdateMethod_Incremental.toUInteger())
		{
			// Incremental sample-per-sample cov updating.
			// It should be implementing the Youngs & Cramer algorithm as described in 
			// Chan, Golub, Leveq, "Updating formulae and a pairwise algorithm...", 1979

			uint32 l_ui32Start=0;
			if(m_ui64Count==0)
			{
				m_oIncrementalMean = l_oSampleChunk.row(0);
				l_ui32Start = 1;
				m_ui64Count = 1;
			}

			MatrixXd l_oChunkContribution;
			l_oChunkContribution.resizeLike(m_oIncrementalCov);
			l_oChunkContribution.setZero();

			for(uint32 i=l_ui32Start;i<l_ui32nRows;i++)
			{
				m_oIncrementalMean += l_oSampleChunk.row(i);

				const MatrixXd l_oDiff = (m_ui64Count+1.0)*l_oSampleChunk.row(i) - m_oIncrementalMean;
				const MatrixXd l_oOuterProd = l_oDiff.transpose()*l_oDiff;

				l_oChunkContribution += 1.0 / (m_ui64Count*(m_ui64Count+1.0)) * l_oOuterProd;

				m_ui64Count++;
			}

			if(ip_bTraceNormalization) 
			{
				l_oChunkContribution = l_oChunkContribution / l_oChunkContribution.trace();
			}

			m_oIncrementalCov += l_oChunkContribution;

			// dumpMatrix(this->getLogManager(), l_oSampleChunk, "Sample");
		}
#if 0
		else if(l_ui32Method == 2)
		{
			// Increment sample counts
			const uint64 l_ui64CountBefore = m_ui64Count;
			const uint64 l_ui64CountChunk = l_ui32nRows;
			const uint64 l_ui64CountAfter = l_ui64CountBefore + l_ui64CountChunk;
			const MatrixXd l_oSampleSum = l_oSampleChunk.colwise().sum();

			// Center the chunk
			const MatrixXd l_oSampleCentered = l_oSampleChunk.rowwise() - l_oSampleSum.row(0)*(1.0/(float64)l_ui64CountChunk);

			const MatrixXd l_oSampleCoMoment = (l_oSampleCentered.transpose() * l_oSampleCentered);

			m_oIncrementalCov = m_oIncrementalCov + l_oSampleCoMoment;
		
			if(l_ui64CountBefore>0)
			{
				const MatrixXd l_oMeanDifference = (l_ui64CountChunk/(float64)l_ui64CountBefore) * m_oIncrementalMean - l_oSampleSum;
				const MatrixXd l_oMeanDiffOuterProduct =  l_oMeanDifference.transpose()*l_oMeanDifference;

				m_oIncrementalCov += l_oMeanDiffOuterProduct*l_ui64CountBefore/(l_ui64CountChunk*l_ui64CountAfter);
			}

			m_oIncrementalMean = m_oIncrementalMean + l_oSampleSum;

			m_ui64Count = l_ui64CountAfter;
		}
		else
		{
			// Increment sample counts
			const uint64 l_ui64CountBefore = m_ui64Count;
			const uint64 l_ui64CountChunk = l_ui32nRows;
			const uint64 l_ui64CountAfter = l_ui64CountBefore + l_ui64CountChunk;
			
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
			if(l_ui64CountBefore>0)
			{
				m_oIncrementalMean = ( m_oIncrementalMean*l_ui64CountBefore + l_oSampleMean*l_ui32nRows) / (float64)l_ui64CountAfter;
				m_oIncrementalCov = ( m_oIncrementalCov*l_ui64CountBefore + l_oSampleCov*(l_ui64CountBefore/(float64)l_ui64CountAfter) ) / (float64)l_ui64CountAfter;
			} 
			else
			{
				m_oIncrementalMean = l_oSampleMean;
				m_oIncrementalCov = l_oSampleCov;
			}


			m_ui64Count = l_ui64CountAfter;
		}
#endif
		else
		{
			this->getLogManager() << LogLevel_Error << "Unknown update method " << CIdentifier(ip_ui64UpdateMethod) << "\n";
			return false;
		}
	} 
		
	// Give output with regularization (mix prior + cov)?
	if(isInputTriggerActive(OVP_Algorithm_OnlineCovariance_Process_GetCov))
	{
		const uint32 l_ui32nCols = ip_pFeatureVectorSet->getDimensionSize(1);

		if(!m_ui64Count) {
			this->getLogManager() << LogLevel_Error << "Not enough samples to compute covariance\n";
			return false;
		}

		// Converters to CMatrix
		Map<MatrixXdRowMajor> l_oOutputMean(op_pMean->getBuffer(),1,l_ui32nCols); 
		Map<MatrixXdRowMajor> l_oOutputCov(op_pCovarianceMatrix->getBuffer(),l_ui32nCols,l_ui32nCols); 

		// The shrinkage parameter pulls the covariance matrix towards diagonal covariance
		MatrixXd l_oPriorCov;
		l_oPriorCov.resizeLike(m_oIncrementalCov);
		l_oPriorCov.setIdentity();

		// Mix the prior and the sample estimates according to the shrinkage parameter. We scale by 1/n to normalize
		l_oOutputMean = m_oIncrementalMean / (float64)m_ui64Count;
		l_oOutputCov = 	ip_f64Shrinkage*l_oPriorCov + (1.0-ip_f64Shrinkage)*(m_oIncrementalCov / (float64)m_ui64Count);

		// Debug block
		dumpMatrix(this->getLogManager(), l_oOutputMean, "Data mean");
		dumpMatrix(this->getLogManager(), m_oIncrementalCov/(float64)m_ui64Count, "Data cov");
		dumpMatrix(this->getLogManager(), ip_f64Shrinkage*l_oPriorCov, "Prior cov");
		dumpMatrix(this->getLogManager(), l_oOutputCov, "Output cov");

	}

	// Give just the output with no shrinkage?
	if(isInputTriggerActive(OVP_Algorithm_OnlineCovariance_Process_GetCovRaw))
	{
		const uint32 l_ui32nCols = ip_pFeatureVectorSet->getDimensionSize(1);

		if(!m_ui64Count) {
			this->getLogManager() << LogLevel_Error << "Not enough samples to compute covariance\n";
			return false;
		}

		// Converters to CMatrix
		Map<MatrixXdRowMajor> l_oOutputMean(op_pMean->getBuffer(),1,l_ui32nCols); 
		Map<MatrixXdRowMajor> l_oOutputCov(op_pCovarianceMatrix->getBuffer(),l_ui32nCols,l_ui32nCols); 

		// We scale by 1/n to normalize
		l_oOutputMean = m_oIncrementalMean / (float64)m_ui64Count;
		l_oOutputCov = 	m_oIncrementalCov / (float64)m_ui64Count;

		// Debug block
		dumpMatrix(this->getLogManager(), l_oOutputMean, "Data mean");
		dumpMatrix(this->getLogManager(), l_oOutputCov, "Data Cov");

	}

	return true;
}

#endif // TARGET_HAS_ThirdPartyEIGEN
