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

	for(uint32 i=0;i<2;i++)
	{
		m_oSignalDecoders[i].initialize(*this,i+1);

		const CIdentifier l_oCovAlgId = this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_OnlineCovariance);
		if(l_oCovAlgId == OV_UndefinedIdentifier)
		{
			this->getLogManager() << LogLevel_Error << "Unable to create the online cov algorithm\n";
			return false;
		}

		m_pIncrementalCov[i] = &this->getAlgorithmManager().getAlgorithm(l_oCovAlgId);
		m_pIncrementalCov[i]->initialize();

		// Set the params of the cov algorithm
		OpenViBE::Kernel::TParameterHandler < uint64 > ip_ui64UpdateMethod(m_pIncrementalCov[i]->getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_UpdateMethod));
		ip_ui64UpdateMethod = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
		OpenViBE::Kernel::TParameterHandler < boolean > ip_bTraceNormalization(m_pIncrementalCov[i]->getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_TraceNormalization));
		ip_bTraceNormalization = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
		OpenViBE::Kernel::TParameterHandler < float64 > ip_f64Shrinkage(m_pIncrementalCov[i]->getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_Shrinkage));
		ip_f64Shrinkage = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
		m_f64Tikhonov = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);

		m_ui64nBuffers[i] = 0;
		m_ui64nSamples[i] = 0;
	}

	m_ui64StimulationIdentifier=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_sSpatialFilterConfigurationFilename=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_ui64FilterDimension=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

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

	for(uint32 i=0;i<2;i++)
	{
		m_oSignalDecoders[i].uninitialize();

		m_pIncrementalCov[i]->uninitialize();
		
		getAlgorithmManager().releaseAlgorithm(*m_pIncrementalCov[i]);
	}

	return true;
}

boolean CBoxAlgorithmRegularizedCSPTrainer::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CBoxAlgorithmRegularizedCSPTrainer::updateCov(int index)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(index+1); i++)
	{
		OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmRegularizedCSPTrainer >* l_oDecoder = &m_oSignalDecoders[index];			
		const IMatrix* l_pInputSignal = l_oDecoder->getOutputMatrix();
		
		l_oDecoder->decode(i);
		if(l_oDecoder->isHeaderReceived())
		{
			TParameterHandler < OpenViBE::IMatrix* > ip_pFeatureVectorSet(m_pIncrementalCov[index]->getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_InputVectors));

			ip_pFeatureVectorSet->setDimensionCount(2);
			ip_pFeatureVectorSet->setDimensionSize(0, l_pInputSignal->getDimensionSize(1));
			ip_pFeatureVectorSet->setDimensionSize(1, l_pInputSignal->getDimensionSize(0));

			m_pIncrementalCov[index]->activateInputTrigger(OVP_Algorithm_OnlineCovariance_Process_Reset, true);
			m_pIncrementalCov[index]->process();
		}
		if(l_oDecoder->isBufferReceived())
		{
			TParameterHandler < OpenViBE::IMatrix* > ip_pFeatureVectorSet(m_pIncrementalCov[index]->getInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_InputVectors));

			// transpose data
			const uint32 l_ui32nChannels = l_pInputSignal->getDimensionSize(0);
			const uint32 l_ui32nSamples = l_pInputSignal->getDimensionSize(1);

			const Map<MatrixXdRowMajor> l_oInputMapper(const_cast<float64*>(l_pInputSignal->getBuffer()), l_ui32nChannels, l_ui32nSamples);
			Map<MatrixXdRowMajor> l_oOutputMapper(ip_pFeatureVectorSet->getBuffer(), l_ui32nSamples, l_ui32nChannels);
			l_oOutputMapper = l_oInputMapper.transpose();

			m_pIncrementalCov[index]->activateInputTrigger(OVP_Algorithm_OnlineCovariance_Process_Update, true);
			m_pIncrementalCov[index]->process();

			m_ui64nBuffers[index]++;
			m_ui64nSamples[index]+=l_ui32nSamples;
		}
		if(l_oDecoder->isEndReceived())
		{
			// nop
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
	for(uint32 i=0;i<2;i++)
	{
		if(!updateCov(i)) 
		{
			return false;
		}
	}
	
	if(l_bShouldTrain)
	{
		this->getLogManager() << LogLevel_Info << "Received train stimulation - be patient\n";	

		const IMatrix* l_pInput = m_oSignalDecoders[0].getOutputMatrix();
		const uint32 l_ui32nChannels = l_pInput->getDimensionSize(0);

		this->getLogManager() << LogLevel_Debug << "Computing eigen vector decomposition...\n";


		// Get out the covariances
		MatrixXd l_oCov[2],l_oCovRaw[2];	
		for(uint32 i=0;i<2;i++) {

			if(m_ui64nSamples[i] < 2)
			{
				this->getLogManager() << LogLevel_Error << "Condition " << i << " had only " << m_ui64nSamples[i] << "samples\n";
				return false;
			}

			TParameterHandler < OpenViBE::IMatrix* > op_pCovarianceMatrix(m_pIncrementalCov[i]->getOutputParameter(OVP_Algorithm_OnlineCovariance_OutputParameterId_CovarianceMatrix));
		
			// Get regularized cov
			m_pIncrementalCov[i]->activateInputTrigger(OVP_Algorithm_OnlineCovariance_Process_GetCov, true);
			if(!m_pIncrementalCov[i]->process()) {
				return false;
			}

			Map<MatrixXdRowMajor> l_oCovMapper(op_pCovarianceMatrix->getBuffer(), l_ui32nChannels, l_ui32nChannels);
			l_oCov[i] = l_oCovMapper;

			// std::stringstream ss; ss << "C:/jl/dump_cov" << i << ".csv";
			// CString tmp(ss.str().c_str());
			// dumpMatrixFile(l_oCov[i], tmp);
			
			// Get vanilla cov
			m_pIncrementalCov[i]->activateInputTrigger(OVP_Algorithm_OnlineCovariance_Process_GetCovRaw, true);
			if(!m_pIncrementalCov[i]->process()) {
				return false;
			}

			l_oCovRaw[i] = l_oCovMapper;

			// ss.str(""); ss << "C:/jl/dump_covraw" << i << ".csv";
			// tmp = CString(ss.str().c_str());
			// dumpMatrixFile(l_oCovRaw[i], tmp);
		}

		if(l_oCov[0].rows() != l_oCov[1].rows() || l_oCov[0].cols() != l_oCov[1].cols() )
		{
			this->getLogManager() << LogLevel_Info << "The input streams had different number of channels\n";
			return false;
		}

		this->getLogManager() << LogLevel_Info << "Data covariance dims are [" << l_oCov[0].rows() << "x" << l_oCov[0].cols() 
			<< "]. Number of samples per condition : \n";
		this->getLogManager() << LogLevel_Info << "  cond1 = " 
			<< m_ui64nBuffers[0] << " chunks, sized " << l_pInput->getDimensionSize(1) << " -> " << m_ui64nSamples[0] << " samples\n";
		this->getLogManager() << LogLevel_Info << "  cond2 = " 
			<< m_ui64nBuffers[1] << " chunks, sized " << l_pInput->getDimensionSize(1) << " -> " << m_ui64nSamples[1] << " samples\n";
		// this->getLogManager() << LogLevel_Info << "Using shrinkage coeff " << m_f64Shrinkage << " ...\n";

		// We wouldn't need to store all this -- they are kept for debugging purposes
		VectorXd l_oEigenValues[2];
		MatrixXd l_oEigenVectors[2];
		VectorXd l_oSortedEigenValues[2];
		MatrixXd l_oSortedEigenVectors[2];
		MatrixXd l_oCovInv[2];
		MatrixXd l_oCovProd[2];
		MatrixXd l_oTikhonov;
		l_oTikhonov.resizeLike(l_oCov[0]);
		l_oTikhonov.setIdentity();
		l_oTikhonov *= m_f64Tikhonov;

		// To get the CSP filters, we compute two sets of eigenvectors,
		// eig(inv(sigma2+tikhonov)*sigma1) and eig(inv(sigma1+tikhonov)*sigma2
		// and pick the ones corresponding to the largest eigenvalues as
		// spatial filters [following Lotte & Guan 2011]. Assumes the shrink
		// of the sigmas (if its used) has been performed inside the cov 
		// computation algorithm.
	
		EigenSolver<MatrixXd> l_oEigenSolverGeneral;

		for(uint32 c=0;c<2;c++) 
		{
			try {
				l_oCovInv[c] = (l_oCov[c]+l_oTikhonov).inverse();
			} catch(...) {
				this->getLogManager() << LogLevel_Error << "Inverse failed for condition " << c+1 << "\n";
				return false;
			}

			l_oCovProd[c] = l_oCovInv[c] * l_oCov[1-c];

			// std::stringstream fn; fn << "C:/jl/dump_covprod" << c << ".csv";
			// dumpMatrixFile(l_oCovProd[c], fn.str().c_str());

			try {
				l_oEigenSolverGeneral.compute(l_oCovProd[c]);
			} catch(...) {
				this->getLogManager() << LogLevel_Error << "EigenSolver failed for condition " << c+1 << "\n";
				return false;
			}

			l_oEigenValues[c] = l_oEigenSolverGeneral.eigenvalues().real();
			l_oEigenVectors[c] = l_oEigenSolverGeneral.eigenvectors().real();

			// Sort the vectors -_- 
			std::vector<std::pair<float64, int>> l_oIndexes;
			for(int i=0; i<l_oEigenValues[c].size(); i++)
			{
				l_oIndexes.push_back( std::make_pair( (l_oEigenValues[c])[i], i));
			}
			std::sort( std::begin(l_oIndexes), std::end(l_oIndexes), std::greater< std::pair<float64,int> >() );

			l_oSortedEigenValues[c].resizeLike(l_oEigenValues[c]);
			l_oSortedEigenVectors[c].resizeLike(l_oEigenVectors[c]);
			for(int i=0; i<l_oEigenValues[c].size(); i++)
			{
				(l_oSortedEigenValues[c])[i] = (l_oEigenValues[c])[l_oIndexes[i].second];
				l_oSortedEigenVectors[c].col(i) = l_oEigenVectors[c].col(l_oIndexes[i].second);
				// this->getLogManager() << LogLevel_Info << "E " << i << " " << (l_oSortedEigenValues[c])[i] << "\n";
			}
		}

		FILE* l_pFile=::fopen(m_sSpatialFilterConfigurationFilename.toASCIIString(), "wb");
		if(!l_pFile)
		{
			this->getLogManager() << LogLevel_Error << "The file [" << m_sSpatialFilterConfigurationFilename << "] could not be opened for writing...\n";
			return false;
		}

		::fprintf(l_pFile, "<OpenViBE-SettingsOverride>\n");
		::fprintf(l_pFile, "\t<SettingValue>");
			
		const uint32 l_ui32HowMany = static_cast<uint32>(m_ui64FilterDimension/2);
		
		for(uint32 c=0;c<2;c++)
		{
			float64 l_f64EigenSelected = 0;
			const float64 l_f64EigenMass = l_oSortedEigenValues[c].sum();
			for(uint32 i=0;i<l_ui32HowMany;i++) 
			{	
				for(uint32 j=0; j<l_ui32nChannels; j++)
				{
					::fprintf(l_pFile, "%e ", l_oSortedEigenVectors[c](j,i));
				}						
				l_f64EigenSelected += (l_oSortedEigenValues[c])[i];
			}
			this->getLogManager() << LogLevel_Info << "The " << l_ui32HowMany << " filter(s) for cond " << c+1 << " cover " << 100.0*l_f64EigenSelected/l_f64EigenMass << "% of corresp. eigenvalues\n";
		}
		
		::fprintf(l_pFile, "</SettingValue>\n");
		::fprintf(l_pFile, "\t<SettingValue>%d</SettingValue>\n", uint32(m_ui64FilterDimension));
		::fprintf(l_pFile, "\t<SettingValue>%d</SettingValue>\n", l_ui32nChannels);
		::fprintf(l_pFile, "</OpenViBE-SettingsOverride>\n");
		::fclose(l_pFile);

		this->getLogManager() << LogLevel_Info << "Shrinked CSP Spatial filter trained successfully.\n";

		m_oStimulationEncoder.getInputStimulationSet()->clear();
		m_oStimulationEncoder.getInputStimulationSet()->appendStimulation(OVTK_StimulationId_TrainCompleted, l_ui64TrainDate, 0);
		m_oStimulationEncoder.encodeBuffer();

		l_rDynamicBoxContext.markOutputAsReadyToSend(0,l_ui64TrainChunkStartTime,l_ui64TrainChunkEndTime);
	}

	return true;
}

#endif // TARGET_HAS_ThirdPartyEIGEN
