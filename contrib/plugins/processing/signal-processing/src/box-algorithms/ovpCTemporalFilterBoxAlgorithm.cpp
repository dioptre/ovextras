#include "ovpCTemporalFilterBoxAlgorithm.h"
#include <cstdlib>
#include <cerrno>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;
using namespace std;

boolean CTemporalFilterBoxAlgorithm::initialize(void)
{
	m_pStreamDecoder=new OpenViBEToolkit::TSignalDecoder < CTemporalFilterBoxAlgorithm >(*this,0);
	m_pStreamEncoder=new OpenViBEToolkit::TSignalEncoder < CTemporalFilterBoxAlgorithm >(*this,0);

	// Compute filter coeff algorithm
	m_pComputeTemporalFilterCoefficients=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ComputeTemporalFilterCoefficients));
	m_pComputeTemporalFilterCoefficients->initialize();

	// Apply filter to signal input buffer
	m_pApplyTemporalFilter=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ApplyTemporalFilter));
	m_pApplyTemporalFilter->initialize();

	m_ui64LastEndTime = 0;

	// compute filter coefs settings
	CString l_oNameFilter;
	CString l_oKindFilter;
	CString l_oFilterOrder;
	CString l_oLowPassBandEdge;
	CString l_oHighPassBandEdge;
	CString l_oPassBandRipple;

	getStaticBoxContext().getSettingValue(0, l_oNameFilter);
	getStaticBoxContext().getSettingValue(1, l_oKindFilter);
	getStaticBoxContext().getSettingValue(2, l_oFilterOrder);
	getStaticBoxContext().getSettingValue(3, l_oLowPassBandEdge);
	getStaticBoxContext().getSettingValue(4, l_oHighPassBandEdge);
	getStaticBoxContext().getSettingValue(5, l_oPassBandRipple);

	uint64 l_ui64UInteger64Parameter;
	int64 l_i64Integer64Parameter;
	float64 l_f64Float64Parameter;
	boolean l_bInitError=false;
	char* l_pEndPtr = NULL;


	l_ui64UInteger64Parameter = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FilterMethod, l_oNameFilter);
	if(l_ui64UInteger64Parameter == OV_UndefinedIdentifier)
	{
		this->getLogManager() << LogLevel_Error << "Unrecognized filter method " << l_oNameFilter << ".\n";
		l_bInitError = true;
	}
	TParameterHandler<uint64> ip_ui64NameFilter(m_pComputeTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_FilterMethod));
	ip_ui64NameFilter=l_ui64UInteger64Parameter;


	l_ui64UInteger64Parameter = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FilterType, l_oKindFilter);
	if(l_ui64UInteger64Parameter == OV_UndefinedIdentifier)
	{
		this->getLogManager() << LogLevel_Error << "Unrecognized filter type " << l_oKindFilter << ".\n";
		l_bInitError = true;
	}
	TParameterHandler<uint64> ip_ui64KindFilter(m_pComputeTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_FilterType));
	ip_ui64KindFilter=l_ui64UInteger64Parameter;

	errno = 0;
	l_i64Integer64Parameter = strtol(l_oFilterOrder, &l_pEndPtr, 10);
	if(l_i64Integer64Parameter <= 0 || (errno !=0 && l_i64Integer64Parameter == 0) || *l_pEndPtr != '\0' || errno == ERANGE)
	{
		this->getLogManager() << LogLevel_Error << "Wrong filter order (" << l_oFilterOrder << "). Should be one or more.\n";
		l_bInitError = true;
	}
	TParameterHandler<uint64> ip_ui64FilterOrder(m_pComputeTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_FilterOrder));
	ip_ui64FilterOrder=l_i64Integer64Parameter;

	errno = 0;
	l_f64Float64Parameter = strtod(l_oLowPassBandEdge, &l_pEndPtr);
	if(l_f64Float64Parameter < 0  || (errno !=0 && l_f64Float64Parameter == 0) || *l_pEndPtr != '\0' || errno == ERANGE)
	{
		this->getLogManager() << LogLevel_Error << "Wrong low cut frequency (" << l_oLowPassBandEdge << " Hz). Should be positive.\n";
		l_bInitError = true;
	}
	TParameterHandler<float64> ip_f64LowCutFrequency(m_pComputeTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_LowCutFrequency));
	ip_f64LowCutFrequency=l_f64Float64Parameter;

	errno = 0;
	l_f64Float64Parameter = strtod(l_oHighPassBandEdge, &l_pEndPtr);
	if(l_f64Float64Parameter < 0 || (errno !=0 && l_f64Float64Parameter == 0) || *l_pEndPtr != '\0' || errno == ERANGE)
	{
		this->getLogManager() << LogLevel_Error << "Wrong high cut frequency (" << l_oHighPassBandEdge << " Hz). Should be positive.\n";
		l_bInitError = true;
	}
	else if(l_f64Float64Parameter < static_cast<float64>(ip_f64LowCutFrequency))
	{
		this->getLogManager() << LogLevel_Error << "Wrong high cut frequency (" << l_oHighPassBandEdge << " Hz). Should be over the low cut frequency "
							  << l_oLowPassBandEdge << " Hz.\n";
		l_bInitError = true;
	}
	TParameterHandler<float64> ip_f64HighCutFrequency(m_pComputeTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_HighCutFrequency));
	ip_f64HighCutFrequency=l_f64Float64Parameter;

	errno = 0;
	l_f64Float64Parameter = strtod(l_oPassBandRipple, &l_pEndPtr);;
	if((errno !=0 && l_f64Float64Parameter == 0) || *l_pEndPtr != '\0' || errno == ERANGE)
	{
		this->getLogManager() << LogLevel_Error << "Wrong pass band ripple (" << l_oPassBandRipple << " dB).\n";
		l_bInitError = true;
	}
	TParameterHandler<float64> ip_f64PassBandRipple(m_pComputeTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_BandPassRipple));
	ip_f64PassBandRipple = l_f64Float64Parameter;


	TParameterHandler<uint64> ip_ui64SamplingFrequency(m_pComputeTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_SamplingFrequency));
	ip_ui64SamplingFrequency.setReferenceTarget(m_pStreamDecoder->getOutputSamplingRate());

	// apply filter settings
	m_pApplyTemporalFilter->getInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_FilterCoefficientsMatrix)->setReferenceTarget(m_pComputeTemporalFilterCoefficients->getOutputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_OutputParameterId_Matrix));

	m_pStreamEncoder->getInputMatrix().setReferenceTarget(m_pApplyTemporalFilter->getOutputParameter(OVP_Algorithm_ApplyTemporalFilter_OutputParameterId_FilteredSignalMatrix));
	m_pStreamEncoder->getInputSamplingRate().setReferenceTarget(m_pStreamDecoder->getOutputSamplingRate());

	if(l_bInitError)
	{
		this->getLogManager() << LogLevel_Error << "Something went wrong during the intialization. Desactivation of the box.\n";
		return false;
	}

	return true;
}

boolean CTemporalFilterBoxAlgorithm::uninitialize(void)
{
	m_pApplyTemporalFilter->uninitialize();
	getAlgorithmManager().releaseAlgorithm(*m_pApplyTemporalFilter);
	m_pComputeTemporalFilterCoefficients->uninitialize();
	getAlgorithmManager().releaseAlgorithm(*m_pComputeTemporalFilterCoefficients);

	//codecs
	m_pStreamEncoder->uninitialize();
	delete m_pStreamEncoder;
	m_pStreamDecoder->uninitialize();
	delete m_pStreamDecoder;
	return true;
}

boolean CTemporalFilterBoxAlgorithm::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CTemporalFilterBoxAlgorithm::process(void)
{
	IBoxIO& l_rDynamicBoxContext=getDynamicBoxContext();
	IBox& l_rStaticBoxContext=getStaticBoxContext();

	for(uint32 i=0; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		for(uint32 j=0; j<l_rDynamicBoxContext.getInputChunkCount(i); j++)
		{
			uint64 l_ui64StartTime=l_rDynamicBoxContext.getInputChunkStartTime(i, j);
			uint64 l_ui64EndTime=l_rDynamicBoxContext.getInputChunkEndTime(i, j);

			if(!m_pStreamDecoder->decode(j)) return false;

			//this has to be done here as it does not work if done once in initialize()
			IMatrix* l_pInputMatrix = m_pStreamDecoder->getOutputMatrix();
			TParameterHandler<IMatrix*> l_oMatrixToFilter = m_pApplyTemporalFilter->getInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_SignalMatrix);
			l_oMatrixToFilter.setReferenceTarget(l_pInputMatrix);

			if(m_pStreamDecoder->isHeaderReceived())
			{
				if(!m_pComputeTemporalFilterCoefficients->process(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputTriggerId_Initialize)) return false;
				if(!m_pComputeTemporalFilterCoefficients->process(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputTriggerId_ComputeCoefficients)) return false;
				if(!m_pApplyTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_Initialize)) return false;
				if(!m_pStreamEncoder->encodeHeader()) return false;

				l_rDynamicBoxContext.markOutputAsReadyToSend(i, l_ui64StartTime, l_ui64EndTime);
			}
			if(m_pStreamDecoder->isBufferReceived())
			{
				if (m_ui64LastEndTime==l_ui64StartTime)
				{
					if(!m_pApplyTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilterWithHistoric)) return false;
				}
				else
				{
					if(!m_pApplyTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilter)) return false;
				}
				if(!m_pStreamEncoder->encodeBuffer()) return false;
				l_rDynamicBoxContext.markOutputAsReadyToSend(i, l_ui64StartTime, l_ui64EndTime);
			}
			if(m_pStreamDecoder->isEndReceived())
			{
				if(!m_pStreamEncoder->encodeEnd()) return false;
				l_rDynamicBoxContext.markOutputAsReadyToSend(i, l_ui64StartTime, l_ui64EndTime);
			}

//			m_ui64LastStartTime=l_ui64StartTime;
			m_ui64LastEndTime=l_ui64EndTime;
			l_rDynamicBoxContext.markInputAsDeprecated(i, j);
		}
	}

	return true;
}
