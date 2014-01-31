#include "ovpCInputChannel.h"

#include <iostream>
#include <system/Memory.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

namespace
{
	class _AutoCast_
	{
	public:
		_AutoCast_(IBox& rBox, IConfigurationManager& rConfigurationManager, const uint32 ui32Index) : m_rConfigurationManager(rConfigurationManager) { rBox.getSettingValue(ui32Index, m_sSettingValue); }
		operator uint64 (void) { return m_rConfigurationManager.expandAsUInteger(m_sSettingValue); }
		operator int64 (void) { return m_rConfigurationManager.expandAsInteger(m_sSettingValue); }
		operator float64 (void) { return m_rConfigurationManager.expandAsFloat(m_sSettingValue); }
		operator boolean (void) { return m_rConfigurationManager.expandAsBoolean(m_sSettingValue); }
		operator const CString (void) { return m_sSettingValue; }
	protected:
		IConfigurationManager& m_rConfigurationManager;
		CString m_sSettingValue;
	};
};

CInputChannel::CInputChannel(const OpenViBE::uint16 ui16InputIndex /*= 0*/)
	: m_ui32SignalChannel(ui16InputIndex*NB_CHANNELS + SIGNAL_CHANNEL)
	, m_ui32StimulationChannel(ui16InputIndex*NB_CHANNELS + STIMULATION_CHANNEL)

{
}

CInputChannel::~CInputChannel()
{ 
}

boolean CInputChannel::initialize(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm>* pTBoxAlgorithm)
{
	m_bIsWorking                    = false;

	m_ui64StartTimestamp            = 0;
	m_ui64EndTimestamp              = 0;
	
	m_oIStimulationSet              = 0;
	m_pTBoxAlgorithm                = pTBoxAlgorithm;

	m_pStreamDecoderSignal          = &m_pTBoxAlgorithm->getAlgorithmManager().getAlgorithm(m_pTBoxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamDecoder));
	m_pStreamDecoderSignal->initialize();
	ip_pMemoryBufferSignal.initialize(m_pStreamDecoderSignal->getInputParameter(OVP_GD_Algorithm_SignalStreamDecoder_InputParameterId_MemoryBufferToDecode));
	op_pMatrixSignal.initialize(m_pStreamDecoderSignal->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_Matrix));
	op_ui64SamplingRateSignal.initialize(m_pStreamDecoderSignal->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_SamplingRate));

	m_pStreamDecoderStimulation     = &m_pTBoxAlgorithm->getAlgorithmManager().getAlgorithm(m_pTBoxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamDecoder));
	m_pStreamDecoderStimulation->initialize();
	ip_pMemoryBufferStimulation.initialize(m_pStreamDecoderStimulation->getInputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_InputParameterId_MemoryBufferToDecode));
	op_pStimulationSetStimulation.initialize(m_pStreamDecoderStimulation->getOutputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_OutputParameterId_StimulationSet));

	return true;
}

boolean CInputChannel::uninitialize()
{
	op_pStimulationSetStimulation.uninitialize();
	ip_pMemoryBufferStimulation.uninitialize();
	m_pStreamDecoderStimulation->uninitialize();
	m_pTBoxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_pStreamDecoderStimulation);

	
	op_ui64SamplingRateSignal.uninitialize();
	op_pMatrixSignal.uninitialize();
	ip_pMemoryBufferSignal.uninitialize();
	m_pStreamDecoderSignal->uninitialize();
	m_pTBoxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_pStreamDecoderSignal);


	return true;
}

boolean CInputChannel::waitForSignalHeader()
{
	IBoxIO& l_rDynamicBoxContext=m_pTBoxAlgorithm->getDynamicBoxContext();

	if(l_rDynamicBoxContext.getInputChunkCount(m_ui32SignalChannel))
	{
		ip_pMemoryBufferSignal    = l_rDynamicBoxContext.getInputChunk(m_ui32SignalChannel, 0);
		m_pStreamDecoderSignal->process();

		if(m_pStreamDecoderSignal->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			m_bIsWorking          = true;

			m_ui64StartTimestamp  = l_rDynamicBoxContext.getInputChunkStartTime(m_ui32SignalChannel, 0);
			m_ui64EndTimestamp    = l_rDynamicBoxContext.getInputChunkEndTime(m_ui32SignalChannel, 0);

			l_rDynamicBoxContext.markInputAsDeprecated(m_ui32SignalChannel, 0);

			return true;
		}
	}

	return false;
}

OpenViBE::uint32 CInputChannel::getNbOfStimulationBuffers()
{
	IBoxIO& l_rDynamicBoxContext  = m_pTBoxAlgorithm->getDynamicBoxContext();

	return l_rDynamicBoxContext.getInputChunkCount(m_ui32StimulationChannel);
}

OpenViBE::uint32 CInputChannel::getNbOfSignalBuffers()
{
	IBoxIO& l_rDynamicBoxContext  = m_pTBoxAlgorithm->getDynamicBoxContext();

	return l_rDynamicBoxContext.getInputChunkCount(m_ui32SignalChannel);
}

OpenViBE::IStimulationSet* CInputChannel::getStimulation(OpenViBE::uint64& startTimestamp, OpenViBE::uint64& endTimestamp, const OpenViBE::uint32 stimulationIndex)
{
	IBoxIO& l_rDynamicBoxContext  = m_pTBoxAlgorithm->getDynamicBoxContext();

	ip_pMemoryBufferStimulation   = l_rDynamicBoxContext.getInputChunk(m_ui32StimulationChannel, stimulationIndex);
	m_pStreamDecoderStimulation->process();
	m_oIStimulationSet            = op_pStimulationSetStimulation;

	startTimestamp                = l_rDynamicBoxContext.getInputChunkStartTime(m_ui32StimulationChannel, stimulationIndex);
	endTimestamp                  = l_rDynamicBoxContext.getInputChunkEndTime(m_ui32StimulationChannel, stimulationIndex);

	l_rDynamicBoxContext.markInputAsDeprecated(m_ui32StimulationChannel, stimulationIndex);

	return m_oIStimulationSet;
}

OpenViBE::IStimulationSet* CInputChannel::discardStimulation(const OpenViBE::uint32 stimulationIndex)
{
	IBoxIO& l_rDynamicBoxContext  = m_pTBoxAlgorithm->getDynamicBoxContext();

	ip_pMemoryBufferStimulation   = l_rDynamicBoxContext.getInputChunk(m_ui32StimulationChannel, stimulationIndex);
	m_pStreamDecoderStimulation->process();
	m_oIStimulationSet            = op_pStimulationSetStimulation;

	l_rDynamicBoxContext.markInputAsDeprecated(m_ui32StimulationChannel, stimulationIndex);
	
	return m_oIStimulationSet;
}


OpenViBE::float64* CInputChannel::getSignal(OpenViBE::uint64& startTimestamp, OpenViBE::uint64& endTimestamp, const OpenViBE::uint32 signalIndex)
{
	IBoxIO& l_rDynamicBoxContext  = m_pTBoxAlgorithm->getDynamicBoxContext();
	ip_pMemoryBufferSignal        = l_rDynamicBoxContext.getInputChunk(m_ui32SignalChannel, signalIndex);
	m_pStreamDecoderSignal->process();
	if(!m_pStreamDecoderSignal->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedBuffer))
		return 0;

	startTimestamp                = l_rDynamicBoxContext.getInputChunkStartTime(m_ui32SignalChannel, signalIndex);
	endTimestamp                  = l_rDynamicBoxContext.getInputChunkEndTime(m_ui32SignalChannel, signalIndex);

	l_rDynamicBoxContext.markInputAsDeprecated(m_ui32SignalChannel, signalIndex);

	return op_pMatrixSignal->getBuffer();
}


OpenViBE::float64* CInputChannel::discardSignal(const OpenViBE::uint32 signalIndex)
{
	IBoxIO& l_rDynamicBoxContext  = m_pTBoxAlgorithm->getDynamicBoxContext();
	ip_pMemoryBufferSignal        = l_rDynamicBoxContext.getInputChunk(m_ui32SignalChannel, signalIndex);
	m_pStreamDecoderSignal->process();
	if(!m_pStreamDecoderSignal->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedBuffer))
		return 0;

	l_rDynamicBoxContext.markInputAsDeprecated(m_ui32SignalChannel, signalIndex);

	return op_pMatrixSignal->getBuffer();
}

#if 0
void CInputChannel::copyData(const OpenViBE::boolean copyFirstBlock, OpenViBE::uint64 matrixIndex)
{
	OpenViBE::CMatrix*&    l_pMatrixBuffer = m_oMatrixBuffer[matrixIndex & 1];

	OpenViBE::float64*     l_pSrcData = op_pMatrixSignal->getBuffer() + (copyFirstBlock ? 0 : m_ui64FirstBlock);
	OpenViBE::float64*     l_pDstData = l_pMatrixBuffer->getBuffer()  + (copyFirstBlock ? m_ui64SecondBlock : 0);
	OpenViBE::uint64       l_ui64Size = (copyFirstBlock ? m_ui64FirstBlock : m_ui64SecondBlock)*sizeof(OpenViBE::float64);

	for(OpenViBE::uint64 i=0; i < m_ui64NbChannels; i++, l_pSrcData+=m_ui64NbSamples, l_pDstData+=m_ui64NbSamples)
	{
		System::Memory::copy(l_pDstData, l_pSrcData, size_t (l_ui64Size));
	}
}
#endif
