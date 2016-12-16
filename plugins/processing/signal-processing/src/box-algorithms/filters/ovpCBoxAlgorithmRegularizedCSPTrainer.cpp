#if defined TARGET_HAS_ThirdPartyEIGEN

#include "ovpCBoxAlgorithmRegularizedCSPTrainer.h"

#include <sstream>
#include <cstdio>

#include <Eigen/Eigenvalues>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

using namespace Eigen;

// typedef Eigen::Matrix< double , Eigen::Dynamic , Eigen::Dynamic, Eigen::RowMajor > MatrixXdRowMajor;

#define CSP_DEBUG 0
#if CSP_DEBUG
void CBoxAlgorithmRegularizedCSPTrainer::dumpMatrix(OpenViBE::Kernel::ILogManager &rMgr, const MatrixXdRowMajor &mat, const CString &desc)
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
void CBoxAlgorithmRegularizedCSPTrainer::dumpMatrixFile(const MatrixXd& mat, const char *fn)
{
	FILE *fp = fopen(fn, "w");
	if(!fp) { this->getLogManager() << LogLevel_Error << "Cannot open " << fn << "\n"; return; }; 
	for(int i=0;i<mat.rows();i++) {
		for(int j=0;j<mat.cols();j++) {
			fprintf(fp, "%s%e", (j>0 ? "," : ""), mat(i,j));
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}

void CBoxAlgorithmRegularizedCSPTrainer::dumpVector(OpenViBE::Kernel::ILogManager &rMgr, const VectorXd &mat, const CString &desc)
{
	rMgr << LogLevel_Info << desc << " : ";
	for(int i=0;i<mat.size();i++) {
		rMgr << mat(i) << " ";
	}
	rMgr << "\n";
}
#else 
void CBoxAlgorithmRegularizedCSPTrainer::dumpMatrix(OpenViBE::Kernel::ILogManager& /* rMgr */, const MatrixXdRowMajor& /*mat*/, const CString& /*desc*/) { }
void CBoxAlgorithmRegularizedCSPTrainer::dumpVector(OpenViBE::Kernel::ILogManager &rMgr, const VectorXd &mat, const CString &desc) { }
void CBoxAlgorithmRegularizedCSPTrainer::dumpMatrixFile(const MatrixXd& mat, const char *fn) { }
#endif


boolean CBoxAlgorithmRegularizedCSPTrainer::initialize(void)
{
	m_oStimulationDecoder.initialize(*this,0);
	m_oStimulationEncoder.initialize(*this,0);

	const IBox& l_rStaticBoxContext = this->getStaticBoxContext();

	m_ui32NumClasses = l_rStaticBoxContext.getInputCount() - 1;

	m_vIncrementalCov.resize(m_ui32NumClasses);
	for(uint32 i=0;i<m_ui32NumClasses;i++)
	{
		m_vIncrementalCov[i] = NULL;
	}

	m_ui64StimulationIdentifier=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_sSpatialFilterConfigurationFilename=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_ui32FiltersPerClass = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	m_bSaveAsBoxConf = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

	if(m_ui32FiltersPerClass <= 0)
	{
		this->getLogManager() << LogLevel_Error << "CSP filter dimension cannot be <= 0.\n";
		return false;
	}
	m_bHasBeenInitialized = true;

	m_vNumBuffers.resize(m_ui32NumClasses);
	m_vNumSamples.resize(m_ui32NumClasses);

	m_vSignalDecoders.resize(m_ui32NumClasses);
	for(uint32 i=0;i<m_ui32NumClasses;i++)
	{
		m_vSignalDecoders[i].initialize(*this,i+1);

		const CIdentifier l_oCovAlgId = this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_OnlineCovariance);
		if(l_oCovAlgId == OV_UndefinedIdentifier)
		{
			this->getLogManager() << LogLevel_Error << "Unable to create the online cov algorithm\n";
			return false;
		}

		m_vIncrementalCov[i] = &this->getAlgorithmManager().getAlgorithm(l_oCovAlgId);
		if(!m_vIncrementalCov[i]->initialize())
		{
			this->getLogManager() << LogLevel_Error << "Unable to initiate the online cov algorithm\n";
			return false;
		}

		// Set the params of the cov algorithm
		OpenViBE::Kernel::TParameterHandler < uint64 > ip_ui64UpdateMethod(m_vIncrementalCov[i]->getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_UpdateMethod));
		OpenViBE::Kernel::TParameterHandler < boolean > ip_bTraceNormalization(m_vIncrementalCov[i]->getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_TraceNormalization));
		OpenViBE::Kernel::TParameterHandler < float64 > ip_f64Shrinkage(m_vIncrementalCov[i]->getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_Shrinkage));

		ip_ui64UpdateMethod = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
		ip_bTraceNormalization = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
		ip_f64Shrinkage = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);

		m_vNumBuffers[i] = 0;
		m_vNumSamples[i] = 0;
	}

	m_f64Tikhonov = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);

	if(m_sSpatialFilterConfigurationFilename == CString(""))
	{
		this->getLogManager() << LogLevel_Error << "Output filename is required in box config\n";
		return false;
	}

	return true;
}

boolean CBoxAlgorithmRegularizedCSPTrainer::uninitialize(void)
{
	m_oStimulationDecoder.uninitialize();
	m_oStimulationEncoder.uninitialize();

	if(m_bHasBeenInitialized)
	{
		for(uint32 i=0;i<m_ui32NumClasses;i++)
		{
			m_vSignalDecoders[i].uninitialize();
			if(m_vIncrementalCov[i])
			{
				m_vIncrementalCov[i]->uninitialize();
				getAlgorithmManager().releaseAlgorithm(*m_vIncrementalCov[i]);
			}
		}
	}

	return true;
}

boolean CBoxAlgorithmRegularizedCSPTrainer::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CBoxAlgorithmRegularizedCSPTrainer::updateCov(uint32 ui32Index)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(ui32Index+1); i++)
	{
		OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmRegularizedCSPTrainer >* l_oDecoder = &m_vSignalDecoders[ui32Index];			
		const IMatrix* l_pInputSignal = l_oDecoder->getOutputMatrix();
		
		l_oDecoder->decode(i);
		if(l_oDecoder->isHeaderReceived())
		{
			TParameterHandler < OpenViBE::IMatrix* > ip_pFeatureVectorSet(m_vIncrementalCov[ui32Index]->getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_InputVectors));

			ip_pFeatureVectorSet->setDimensionCount(2);
			ip_pFeatureVectorSet->setDimensionSize(0, l_pInputSignal->getDimensionSize(1));
			ip_pFeatureVectorSet->setDimensionSize(1, l_pInputSignal->getDimensionSize(0));
			
			if(m_ui32FiltersPerClass>l_pInputSignal->getDimensionSize(0)) {
				this->getLogManager() << LogLevel_Error << "CSP filter count cannot exceed the number of input channels (" << l_pInputSignal->getDimensionSize(1) << ") in the stream " << i+1 << "\n";
				return false;
			}

			m_vIncrementalCov[ui32Index]->activateInputTrigger(OVP_Algorithm_OnlineCovariance_Process_Reset, true);
			if(!m_vIncrementalCov[ui32Index]->process())
			{
				this->getLogManager() << LogLevel_Error << "Something went wrong during the parametrization of the covariance algorithm.\n";
				return false;
			}
		}
		if(l_oDecoder->isBufferReceived())
		{
			TParameterHandler < OpenViBE::IMatrix* > ip_pFeatureVectorSet(m_vIncrementalCov[ui32Index]->getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_InputVectors));

			// transpose data
			const uint32 l_ui32nChannels = l_pInputSignal->getDimensionSize(0);
			const uint32 l_ui32nSamples = l_pInputSignal->getDimensionSize(1);

			const Map<MatrixXdRowMajor> l_oInputMapper(const_cast<float64*>(l_pInputSignal->getBuffer()), l_ui32nChannels, l_ui32nSamples);
			Map<MatrixXdRowMajor> l_oOutputMapper(ip_pFeatureVectorSet->getBuffer(), l_ui32nSamples, l_ui32nChannels);
			l_oOutputMapper = l_oInputMapper.transpose();

			m_vIncrementalCov[ui32Index]->activateInputTrigger(OVP_Algorithm_OnlineCovariance_Process_Update, true);
			m_vIncrementalCov[ui32Index]->process();

			m_vNumBuffers[ui32Index]++;
			m_vNumSamples[ui32Index]+=l_ui32nSamples;
		}
		if(l_oDecoder->isEndReceived())
		{
			// nop
		}
	}

	return true;
}		

//
// Returns a sample-weighted average of given covariance matrices that does not include the cov of ui32SkipIndex
//
// @todo error handling is a bit scarce
//
// @note This will recompute the weights on every call, but given how small amount of 
// computations we're speaking of, there's not much point in optimizing.
//
boolean CBoxAlgorithmRegularizedCSPTrainer::outclassCovAverage(uint32 ui32SkipIndex, const std::vector<MatrixXd>& vCov, MatrixXd &oCovAvg)
{
	if (vCov.size() == 0 || ui32SkipIndex >= vCov.size()) 
	{
		return false;
	}

	std::vector<float64> l_vClassWeights;
	uint64 l_ui64TotalOutclassSamples = 0;

	// Compute the total number of samples
	for (uint32 i = 0; i < m_ui32NumClasses; i++)
	{
		if (i == ui32SkipIndex)
		{
			continue;
		}
		l_ui64TotalOutclassSamples += m_vNumSamples[i];

	}

	// Compute weigths for averaging
	l_vClassWeights.resize(m_ui32NumClasses);
	for (uint32 i = 0; i < m_ui32NumClasses; i++)
	{
		if (i == ui32SkipIndex)
		{
			// Ignore the in-class in the average
			l_vClassWeights[i] = 0;
		}
		else
		{
			l_vClassWeights[i] = m_vNumSamples[i] / static_cast<float64>(l_ui64TotalOutclassSamples);
		}
		this->getLogManager() << LogLevel_Debug << "Condition " << i + 1 << " averaging weight = " << l_vClassWeights[i] << "\n";
	}

	// Average the covs
	oCovAvg.resizeLike(vCov[0]);
	oCovAvg.setZero();
	for (uint32 i = 0; i < m_ui32NumClasses; i++)
	{
		oCovAvg += (l_vClassWeights[i] * vCov[i]);

	}

	return true;
}

boolean CBoxAlgorithmRegularizedCSPTrainer::computeCSP(const std::vector<Eigen::MatrixXd>& vCov, std::vector<Eigen::MatrixXd>& vSortedEigenVectors,
	std::vector<Eigen::VectorXd>& vSortedEigenValues)
{
	// We wouldn't need to store all this -- they are kept for debugging purposes
	std::vector<VectorXd> l_vEigenValues;
	std::vector<MatrixXd> l_vEigenVectors;
	std::vector<MatrixXd> l_vCovInv;
	std::vector<MatrixXd> l_vCovProd;
	MatrixXd l_oTikhonov;
	MatrixXd l_oOutclassCov;
	l_oTikhonov.resizeLike(vCov[0]);
	l_oTikhonov.setIdentity();
	l_oTikhonov *= m_f64Tikhonov;

	l_vEigenValues.resize(m_ui32NumClasses);
	l_vEigenVectors.resize(m_ui32NumClasses);
	l_vCovInv.resize(m_ui32NumClasses);
	l_vCovProd.resize(m_ui32NumClasses);

	vSortedEigenVectors.resize(m_ui32NumClasses);
	vSortedEigenValues.resize(m_ui32NumClasses);

	// To get the CSP filters, we compute two sets of eigenvectors,
	// eig(inv(sigma2+tikhonov)*sigma1) and eig(inv(sigma1+tikhonov)*sigma2
	// and pick the ones corresponding to the largest eigenvalues as
	// spatial filters [following Lotte & Guan 2011]. Assumes the shrink
	// of the sigmas (if its used) has been performed inside the cov 
	// computation algorithm.

	EigenSolver<MatrixXd> l_oEigenSolverGeneral;

	for (uint32 c = 0; c < m_ui32NumClasses; c++)
	{
		try {
			l_vCovInv[c] = (vCov[c] + l_oTikhonov).inverse();
		}
		catch (...) {
			this->getLogManager() << LogLevel_Error << "Inverse failed for condition " << c + 1 << "\n";
			return false;
		}

		// Compute covariance in all the classes except 'c'.
		if (!outclassCovAverage(c, vCov, l_oOutclassCov))
		{
			this->getLogManager() << LogLevel_Error << "Outclass cov computation failed for condition " << c + 1 << "\n";
			return false;
		}

		l_vCovProd[c] = l_vCovInv[c] * l_oOutclassCov;

		// std::stringstream fn; fn << "C:/jl/dump_covprod" << c << ".csv";
		// dumpMatrixFile(l_oCovProd[c], fn.str().c_str());

		try {
			l_oEigenSolverGeneral.compute(l_vCovProd[c]);
		}
		catch (...) {
			this->getLogManager() << LogLevel_Error << "EigenSolver failed for condition " << c + 1 << "\n";
			return false;
		}

		l_vEigenValues[c] = l_oEigenSolverGeneral.eigenvalues().real();
		l_vEigenVectors[c] = l_oEigenSolverGeneral.eigenvectors().real();

		// Sort the vectors -_- 
		std::vector< std::pair<float64, int> > l_oIndexes;
		for (int i = 0; i < l_vEigenValues[c].size(); i++)
		{
			l_oIndexes.push_back(std::make_pair((l_vEigenValues[c])[i], i));
		}
		std::sort(l_oIndexes.begin(), l_oIndexes.end(), std::greater< std::pair<float64, int> >());

		vSortedEigenValues[c].resizeLike(l_vEigenValues[c]);
		vSortedEigenVectors[c].resizeLike(l_vEigenVectors[c]);
		for (int i = 0; i < l_vEigenValues[c].size(); i++)
		{
			(vSortedEigenValues[c])[i]     = (l_vEigenValues[c])[l_oIndexes[i].second];
			vSortedEigenVectors[c].col(i) = l_vEigenVectors[c].col(l_oIndexes[i].second);
			// this->getLogManager() << LogLevel_Info << "E " << i << " " << (l_oSortedEigenValues[c])[i] << "\n";
		}
	}
	return true;

}

boolean CBoxAlgorithmRegularizedCSPTrainer::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	boolean l_bShouldTrain=false;
	uint64 l_ui64TrainDate=0, l_ui64TrainChunkStartTime=0, l_ui64TrainChunkEndTime=0;

	// Handle input stimulations
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		m_oStimulationDecoder.decode(i);
		if(m_oStimulationDecoder.isHeaderReceived())
		{
			m_oStimulationEncoder.encodeHeader();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0,l_rDynamicBoxContext.getInputChunkStartTime(0, i),l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
		if(m_oStimulationDecoder.isBufferReceived())
		{
			const TParameterHandler < IStimulationSet* > op_pStimulationSet(m_oStimulationDecoder.getOutputStimulationSet());
			for(uint32 j=0; j<op_pStimulationSet->getStimulationCount(); j++)
			{
				if(op_pStimulationSet->getStimulationIdentifier(j)==m_ui64StimulationIdentifier)
				{
					l_ui64TrainDate = op_pStimulationSet->getStimulationDate(op_pStimulationSet->getStimulationCount()-1);
					l_ui64TrainChunkStartTime = l_rDynamicBoxContext.getInputChunkStartTime(0, i);
					l_ui64TrainChunkEndTime = l_rDynamicBoxContext.getInputChunkEndTime(0, i);
					l_bShouldTrain = true;
					break;
				}
			}
		}
		if(m_oStimulationDecoder.isEndReceived())
		{
			m_oStimulationEncoder.encodeEnd();
		}
	}

	// Update all covs with the current data chunks (if any)
	for(uint32 i=0;i<m_ui32NumClasses;i++)
	{
		if(!updateCov(i)) 
		{
			return false;
		}
	}
	
	if (l_bShouldTrain)
	{
		this->getLogManager() << LogLevel_Info << "Received train stimulation - be patient\n";

		const IMatrix* l_pInput = m_vSignalDecoders[0].getOutputMatrix();
		const uint32 l_ui32nChannels = l_pInput->getDimensionSize(0);

		this->getLogManager() << LogLevel_Debug << "Computing eigen vector decomposition...\n";

		// Get out the covariances
		std::vector<MatrixXd> l_vCov, l_vCovRaw;

		l_vCov.resize(m_ui32NumClasses);
		l_vCovRaw.resize(m_ui32NumClasses);

		for (uint32 i = 0; i < m_ui32NumClasses; i++) {

			if (m_vNumSamples[i] < 2)
			{
				this->getLogManager() << LogLevel_Error << "Condition " << i << " had only " << m_vNumSamples[i] << " samples\n";
				return false;
			}

			TParameterHandler < OpenViBE::IMatrix* > op_pCovarianceMatrix(m_vIncrementalCov[i]->getOutputParameter(OVP_Algorithm_OnlineCovariance_OutputParameterId_CovarianceMatrix));

			// Get regularized cov
			m_vIncrementalCov[i]->activateInputTrigger(OVP_Algorithm_OnlineCovariance_Process_GetCov, true);
			if (!m_vIncrementalCov[i]->process()) {
				return false;
			}

			Map<MatrixXdRowMajor> l_oCovMapper(op_pCovarianceMatrix->getBuffer(), l_ui32nChannels, l_ui32nChannels);
			l_vCov[i] = l_oCovMapper;

			// std::stringstream ss; ss << "C:/jl/dump_cov" << i << ".csv";
			// CString tmp(ss.str().c_str());
			// dumpMatrixFile(l_oCov[i], tmp);

			// Get vanilla cov
			m_vIncrementalCov[i]->activateInputTrigger(OVP_Algorithm_OnlineCovariance_Process_GetCovRaw, true);
			if (!m_vIncrementalCov[i]->process()) {
				return false;
			}

			l_vCovRaw[i] = l_oCovMapper;

			// ss.str(""); ss << "C:/jl/dump_covraw" << i << ".csv";
			// tmp = CString(ss.str().c_str());
			// dumpMatrixFile(l_oCovRaw[i], tmp);
		}

		// Sanity check
		for (uint32 i = 1; i < m_ui32NumClasses; i++)
		{
			if (l_vCov[0].rows() != l_vCov[i].rows() || l_vCov[0].cols() != l_vCov[i].cols())
			{
				this->getLogManager() << LogLevel_Error << "The samples for conditions 1 and " << i + 1 << " had different numbers of channels\n";
				return false;
			}
		}

		this->getLogManager() << LogLevel_Info << "Data covariance dims are [" << static_cast<uint32>(l_vCov[0].rows()) << "x" << static_cast<uint32>(l_vCov[0].cols())
			<< "]. Number of samples per condition : \n";
		for (uint32 i = 0; i < m_ui32NumClasses; i++)
		{
			this->getLogManager() << LogLevel_Info << "  cond " << i + 1 << " = "
				<< m_vNumBuffers[i] << " chunks, sized " << l_pInput->getDimensionSize(1) << " -> " << m_vNumSamples[i] << " samples\n";
			// this->getLogManager() << LogLevel_Info << "Using shrinkage coeff " << m_f64Shrinkage << " ...\n";
		}

		// Compute the actual CSP using the obtained covariance matrices
		std::vector<Eigen::MatrixXd> l_vSortedEigenVectors;
		std::vector<Eigen::VectorXd> l_vSortedEigenValues;
		if (!computeCSP(l_vCov, l_vSortedEigenVectors, l_vSortedEigenValues)) 
		{
			return false;
		}
	
		// Create a CMatrix mapper that can spool the filters to a file
		CMatrix l_oSelectedVectors;
		l_oSelectedVectors.setDimensionCount(2);
		l_oSelectedVectors.setDimensionSize(0, m_ui32FiltersPerClass * m_ui32NumClasses);
		l_oSelectedVectors.setDimensionSize(1, l_ui32nChannels);

		Map<MatrixXdRowMajor> l_oSelectedVectorsMapper(l_oSelectedVectors.getBuffer(), m_ui32FiltersPerClass * m_ui32NumClasses, l_ui32nChannels);

		for (uint32 c = 0; c < m_ui32NumClasses; c++)
		{
			l_oSelectedVectorsMapper.block(c*m_ui32FiltersPerClass, 0, m_ui32FiltersPerClass, l_ui32nChannels) =
				l_vSortedEigenVectors[c].block(0, 0, l_ui32nChannels, m_ui32FiltersPerClass).transpose();

			this->getLogManager() << LogLevel_Info << "The " << m_ui32FiltersPerClass << " filter(s) for cond " << c + 1 << " cover " << 100.0*l_vSortedEigenValues[c].head(m_ui32FiltersPerClass).sum() / l_vSortedEigenValues[c].sum() << "% of corresp. eigenvalues\n";
		}

		if(m_bSaveAsBoxConf)
		{
			FILE* l_pFile=::fopen(m_sSpatialFilterConfigurationFilename.toASCIIString(), "wb");
			if(!l_pFile)
			{
				this->getLogManager() << LogLevel_Error << "The file [" << m_sSpatialFilterConfigurationFilename << "] could not be opened for writing...\n";
				return false;
			}

			::fprintf(l_pFile, "<OpenViBE-SettingsOverride>\n");
			::fprintf(l_pFile, "\t<SettingValue>");

			const uint32 l_ui32numCoefficients = m_ui32FiltersPerClass*m_ui32NumClasses*l_ui32nChannels;
			for(uint32 i=0;i<l_ui32numCoefficients;i++)
			{
				::fprintf(l_pFile, "%e ", l_oSelectedVectors.getBuffer()[i]);
			}
		
			::fprintf(l_pFile, "</SettingValue>\n");
			::fprintf(l_pFile, "\t<SettingValue>%d</SettingValue>\n", m_ui32FiltersPerClass*m_ui32NumClasses);
			::fprintf(l_pFile, "\t<SettingValue>%d</SettingValue>\n", l_ui32nChannels);
			::fprintf(l_pFile, "\t<SettingValue></SettingValue>\n");
			::fprintf(l_pFile, "</OpenViBE-SettingsOverride>\n");

			::fclose(l_pFile);
		}
		else
		{
			for (uint32 i = 0; i < l_oSelectedVectors.getDimensionSize(0); i++) {
				std::stringstream l_oLabel; l_oLabel << "Cond " << static_cast<uint32>(i / m_ui32FiltersPerClass) + 1
					<< " filter " <<  i % m_ui32FiltersPerClass + 1;

				l_oSelectedVectors.setDimensionLabel(0, i, l_oLabel.str().c_str());
			}

			if(!OpenViBEToolkit::Tools::Matrix::saveToTextFile(l_oSelectedVectors, m_sSpatialFilterConfigurationFilename, 10)) 
			{
				this->getLogManager() << LogLevel_Error << "Save to file [" << m_sSpatialFilterConfigurationFilename << "] failed\n";
				return false;
			}
		}

		this->getLogManager() << LogLevel_Info << "Regularized CSP Spatial filter trained successfully.\n";

		// Clean data, so if there's a new train stimulation, we'll start again.
		// @note possibly this should be a parameter in the future to allow incremental training
		for(uint32 i=0;i<m_ui32NumClasses;i++)
		{
			m_vIncrementalCov[i]->activateInputTrigger(OVP_Algorithm_OnlineCovariance_Process_Reset, true);
			m_vNumSamples[i] = 0;
			m_vNumBuffers[i] = 0;
		}

		m_oStimulationEncoder.getInputStimulationSet()->clear();
		m_oStimulationEncoder.getInputStimulationSet()->appendStimulation(OVTK_StimulationId_TrainCompleted, l_ui64TrainDate, 0);
		m_oStimulationEncoder.encodeBuffer();

		l_rDynamicBoxContext.markOutputAsReadyToSend(0,l_ui64TrainChunkStartTime,l_ui64TrainChunkEndTime);
	}

	return true;
}

#endif // TARGET_HAS_ThirdPartyEIGEN
