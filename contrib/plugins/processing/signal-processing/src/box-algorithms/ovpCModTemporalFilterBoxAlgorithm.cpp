#include "ovpCModTemporalFilterBoxAlgorithm.h"
#include <cstdlib>
#include <cerrno>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;
using namespace std;

boolean CModTemporalFilterBoxAlgorithm::initialize(void)
{
	m_bHasBeenInit = false;

	m_pStreamDecoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamDecoder));
	m_pStreamEncoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamEncoder));

	m_pStreamDecoder->initialize();
	m_pStreamEncoder->initialize();

	ip_pMemoryBufferToDecode.initialize(m_pStreamDecoder->getInputParameter(OVP_GD_Algorithm_SignalStreamDecoder_InputParameterId_MemoryBufferToDecode));
	op_pEncodedMemoryBuffer.initialize(m_pStreamEncoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamEncoder_OutputParameterId_EncodedMemoryBuffer));

	// Compute filter coeff algorithm
	m_pComputeModTemporalFilterCoefficients=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ComputeTemporalFilterCoefficients));
	m_pComputeModTemporalFilterCoefficients->initialize();

	// Apply filter to signal input buffer
	m_pApplyModTemporalFilter=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ApplyTemporalFilter));
	m_pApplyModTemporalFilter->initialize();

	m_ui64LastEndTime = 0;

	m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_SignalStreamEncoder_InputParameterId_SamplingRate)->setReferenceTarget(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_SamplingRate));

	m_sFilterMethod=CString("");
	m_sFilterType=CString("");
	m_sFilterOrder=CString("");
	m_sLowBand=CString("");
	m_sHighBand=CString("");
	m_sPassBandRiple=CString("");
	if(!updateSettings())
	{
		this->getLogManager() << LogLevel_Error << "The box cannot be initialized.\n";
		return false;
	}

	m_pComputeModTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_SamplingFrequency)->setReferenceTarget(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_SamplingRate));


	// apply filter settings
	m_pApplyModTemporalFilter->getInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_SignalMatrix)->setReferenceTarget(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_Matrix));
	m_pApplyModTemporalFilter->getInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_FilterCoefficientsMatrix)->setReferenceTarget(m_pComputeModTemporalFilterCoefficients->getOutputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_OutputParameterId_Matrix));

	m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_SignalStreamEncoder_InputParameterId_Matrix)->setReferenceTarget(m_pApplyModTemporalFilter->getOutputParameter(OVP_Algorithm_ApplyTemporalFilter_OutputParameterId_FilteredSignalMatrix));
	return true;
}

bool CModTemporalFilterBoxAlgorithm::updateSettings()
{
	boolean retVal = false;
	boolean l_bError = false;
	char* l_pEndPtr = NULL;
	//get the settings
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

	if(m_sFilterMethod!=l_oNameFilter)
	{
		uint64 l_ui64UInteger64Parameter = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FilterMethod, l_oNameFilter);
		if(l_ui64UInteger64Parameter == OV_UndefinedIdentifier)
		{
			this->getLogManager() << LogLevel_Error << "Unrecognized filter method " << l_oNameFilter << ".\n";
			l_bError = true;
		}
		else
		{
			TParameterHandler<uint64> ip_ui64NameFilter(m_pComputeModTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_FilterMethod));
			ip_ui64NameFilter=l_ui64UInteger64Parameter;
			retVal=true;
		}
		m_sFilterMethod=l_oNameFilter; //We set up the new value to avoid to repeat the error log over and over again
	}

	if(m_sFilterType!=l_oKindFilter)
	{
		uint64 l_ui64UInteger64Parameter = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_FilterType, l_oKindFilter);
		if(l_ui64UInteger64Parameter == OV_UndefinedIdentifier)
		{
			this->getLogManager() << LogLevel_Error << "Unrecognized filter type " << l_oKindFilter << ".\n";
			l_bError = true;
		}
		else
		{
			TParameterHandler<uint64> ip_ui64KindFilter(m_pComputeModTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_FilterType));
			ip_ui64KindFilter=l_ui64UInteger64Parameter;
			retVal=true;
		}
		m_sFilterType=l_oKindFilter; //We set up the new value to avoid to repeat the error log over and over again
	}

	if(m_sFilterOrder!=l_oFilterOrder)
	{
		errno = 0;
		int64 l_i64Integer64Parameter = strtol(l_oFilterOrder, &l_pEndPtr, 10);
		if(l_i64Integer64Parameter <= 0 || (errno !=0 && l_i64Integer64Parameter == 0) || *l_pEndPtr != '\0' || errno == ERANGE)
		{
			this->getLogManager() << LogLevel_Error << "Wrong filter order (" << l_oFilterOrder << "). Should be one or more.\n";
			l_bError = true;
		}
		else
		{
			TParameterHandler<uint64> ip_ui64FilterOrder(m_pComputeModTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_FilterOrder));
			ip_ui64FilterOrder=l_i64Integer64Parameter;
			retVal=true;
		}
		m_sFilterOrder=l_oFilterOrder; //We set up the new value to avoid to repeat the error log over and over again
	}

	if(m_sLowBand!=l_oLowPassBandEdge)
	{
		TParameterHandler<float64> ip_f64HighCutFrequency(m_pComputeModTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_HighCutFrequency));

		errno = 0;
		float64 l_f64Float64Parameter = strtod(l_oLowPassBandEdge, &l_pEndPtr);
		if(l_f64Float64Parameter < 0 || (errno !=0 && l_f64Float64Parameter == 0) || *l_pEndPtr != '\0' || errno == ERANGE)
		{
			this->getLogManager() << LogLevel_Error << "Wrong low cut frequency (" << l_oLowPassBandEdge << " Hz). Should be positive.\n";
			l_bError = true;
		}
		else if(m_bHasBeenInit && l_f64Float64Parameter > static_cast<float64>(ip_f64HighCutFrequency))//If it's not the first init we need to check that we do not set a wrong frequency according to the high one
		{
			this->getLogManager() << LogLevel_Error << "Wrong low cut frequency (" << l_oLowPassBandEdge << " Hz). Should be under the high cut frequency "
								  << static_cast<float64>(ip_f64HighCutFrequency) << " Hz.\n";
			l_bError = true;
		}
		else
		{
			TParameterHandler<float64> ip_f64LowCutFrequency(m_pComputeModTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_LowCutFrequency));
			ip_f64LowCutFrequency=l_f64Float64Parameter;
			retVal=true;
		}
		m_sLowBand=l_oLowPassBandEdge;
	}

	if(m_sHighBand!=l_oHighPassBandEdge)
	{
		TParameterHandler<float64> ip_f64LowCutFrequency(m_pComputeModTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_LowCutFrequency));
		errno = 0;
		float64 l_f64Float64Parameter = strtod(l_oHighPassBandEdge, &l_pEndPtr);
		if(l_f64Float64Parameter < 0 || (errno !=0 && l_f64Float64Parameter == 0) || *l_pEndPtr != '\0' || errno == ERANGE)
		{
			this->getLogManager() << LogLevel_Error << "Wrong high cut frequency (" << l_oHighPassBandEdge << " Hz). Should be positive.\n";
			l_bError = true;
		}
		else if(l_f64Float64Parameter < static_cast<float64>(ip_f64LowCutFrequency))
		{
			this->getLogManager() << LogLevel_Error << "Wrong high cut frequency (" << l_oHighPassBandEdge << " Hz). Should be over the low cut frequency "
								  << static_cast<float64>(ip_f64LowCutFrequency) << " Hz.\n";
			l_bError = true;
		}
		else
		{
			TParameterHandler<float64> ip_f64HighCutFrequency(m_pComputeModTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_HighCutFrequency));
			ip_f64HighCutFrequency=l_f64Float64Parameter;
			retVal=true;
		}
		m_sHighBand=l_oHighPassBandEdge;
	}

	if(m_sPassBandRiple!=l_oPassBandRipple)
	{
		errno = 0;
		float64 l_f64Float64Parameter = strtod(l_oPassBandRipple, &l_pEndPtr);;
		if((errno !=0 && l_f64Float64Parameter == 0) || *l_pEndPtr != '\0' || errno == ERANGE)
		{
			this->getLogManager() << LogLevel_Error << "Wrong pass band ripple (" << l_oPassBandRipple << " dB).\n";
			l_bError = true;
		}
		else
		{
			TParameterHandler<float64> ip_f64PassBandRipple(m_pComputeModTemporalFilterCoefficients->getInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputParameterId_BandPassRipple));
			ip_f64PassBandRipple = l_f64Float64Parameter;
			retVal=true;
		}
		m_sPassBandRiple=l_oPassBandRipple;
	}

	//If it was the original init we return false to stop the init process
	if(!m_bHasBeenInit && l_bError)
	{
		return false;
	}
	m_bHasBeenInit = true;
	return retVal;
}

bool CModTemporalFilterBoxAlgorithm::compute()
{
	bool retVal = true;
	//compute filter coeff

	if(!m_pComputeModTemporalFilterCoefficients->process(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputTriggerId_Initialize)) return false;
	if(!m_pComputeModTemporalFilterCoefficients->process(OVP_Algorithm_ComputeTemporalFilterCoefficients_InputTriggerId_ComputeCoefficients)) return false;
	if(!m_pApplyModTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_Initialize)) return false;

	return retVal;
}

boolean CModTemporalFilterBoxAlgorithm::uninitialize(void)
{
	m_pApplyModTemporalFilter->uninitialize();
	m_pComputeModTemporalFilterCoefficients->uninitialize();
	m_pStreamEncoder->uninitialize();
	m_pStreamDecoder->uninitialize();

	getAlgorithmManager().releaseAlgorithm(*m_pApplyModTemporalFilter);
	getAlgorithmManager().releaseAlgorithm(*m_pComputeModTemporalFilterCoefficients);
	getAlgorithmManager().releaseAlgorithm(*m_pStreamEncoder);
	getAlgorithmManager().releaseAlgorithm(*m_pStreamDecoder);

	return true;
}

boolean CModTemporalFilterBoxAlgorithm::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CModTemporalFilterBoxAlgorithm::process(void)
{
	IBoxIO& l_rDynamicBoxContext=getDynamicBoxContext();
    const IBox& l_rStaticBoxContext=getStaticBoxContext();

	for(uint32 i=0; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		for(uint32 j=0; j<l_rDynamicBoxContext.getInputChunkCount(i); j++)
		{
//			TParameterHandler < const IMemoryBuffer* > l_oInputMemoryBufferHandle(m_pStreamDecoder->getInputParameter(OVP_GD_Algorithm_SignalStreamDecoder_InputParameterId_MemoryBufferToDecode));
//			TParameterHandler < IMemoryBuffer* > l_oOutputMemoryBufferHandle(m_pStreamEncoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamEncoder_OutputParameterId_EncodedMemoryBuffer));
//			l_oInputMemoryBufferHandle=l_rDynamicBoxContext.getInputChunk(i, j);
//			l_oOutputMemoryBufferHandle=l_rDynamicBoxContext.getOutputChunk(i);
			ip_pMemoryBufferToDecode=l_rDynamicBoxContext.getInputChunk(i, j);
			op_pEncodedMemoryBuffer=l_rDynamicBoxContext.getOutputChunk(i);
			uint64 l_ui64StartTime=l_rDynamicBoxContext.getInputChunkStartTime(i, j);
			uint64 l_ui64EndTime=l_rDynamicBoxContext.getInputChunkEndTime(i, j);

			if(!m_pStreamDecoder->process()) return false;
			if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedHeader))
			{
				compute();
				if(!m_pStreamEncoder->process(OVP_GD_Algorithm_SignalStreamEncoder_InputTriggerId_EncodeHeader)) return false;

				l_rDynamicBoxContext.markOutputAsReadyToSend(i, l_ui64StartTime, l_ui64EndTime);
			}
			if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedBuffer))
			{

				if(updateSettings())
				{
					//recompute if the settings have changed only
					bool rVar = compute();
					if(!rVar)
					{
						this->getLogManager() << LogLevel_Error << "error during computation\n";
					}
				}

				if (m_ui64LastEndTime==l_ui64StartTime)
				{
					if(!m_pApplyModTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilterWithHistoric)) return false;
				}
				else
				{
					if(!m_pApplyModTemporalFilter->process(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilter)) return false;
				}
				if(!m_pStreamEncoder->process(OVP_GD_Algorithm_SignalStreamEncoder_InputTriggerId_EncodeBuffer)) return false;
				l_rDynamicBoxContext.markOutputAsReadyToSend(i, l_ui64StartTime, l_ui64EndTime);
			}
			if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedEnd))
			{
				if(!m_pStreamEncoder->process(OVP_GD_Algorithm_SignalStreamEncoder_InputTriggerId_EncodeEnd)) return false;
				l_rDynamicBoxContext.markOutputAsReadyToSend(i, l_ui64StartTime, l_ui64EndTime);
			}

			m_ui64LastEndTime=l_ui64EndTime;
			l_rDynamicBoxContext.markInputAsDeprecated(i, j);
		}
	}

	return true;
}
