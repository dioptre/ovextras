#include "ovpCBoxAlgorithmConditionalIdentity.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;
using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessingCoAdapt;
using namespace OpenViBEToolkit;
using namespace std;

boolean CConditionalIdentity::initialize(void)
{
	m_ui64StimulationIdentifier=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_bStopSending = false;
	m_pStimulationDecoder.initialize(*this);
	
	return true;
}

boolean CConditionalIdentity::uninitialize(void)
{
	m_pStimulationDecoder.uninitialize();

	return true;
}

void CConditionalIdentity::release(void)
{
	delete this;
}

boolean CConditionalIdentity::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CConditionalIdentity::process(void)
{
	IBox* l_pStaticBoxContext=getBoxAlgorithmContext()->getStaticBoxContext();
	IBoxIO* l_pDynamicBoxContext=getBoxAlgorithmContext()->getDynamicBoxContext();

	uint64 l_ui64StartTime=0;
	uint64 l_ui64EndTime=0;
	uint64 l_ui64ChunkSize=0;
	const uint8* l_pChunkBuffer=NULL;
	
	for(uint32 i=1; i<l_pStaticBoxContext->getInputCount(); i++)
	{
		for(uint32 j=0; j<l_pDynamicBoxContext->getInputChunkCount(i); j++)
		{
			if(!m_bStopSending)
			{
				l_pDynamicBoxContext->getInputChunk(i, j, l_ui64StartTime, l_ui64EndTime, l_ui64ChunkSize, l_pChunkBuffer);
				l_pDynamicBoxContext->appendOutputChunkData(i, l_pChunkBuffer, l_ui64ChunkSize);
				l_pDynamicBoxContext->markOutputAsReadyToSend(i, l_ui64StartTime, l_ui64EndTime);
			}
			l_pDynamicBoxContext->markInputAsDeprecated(i, j);
		}
	}	
	
	for(uint32 i=0; i<l_pDynamicBoxContext->getInputChunkCount(0); i++)
	{
		// decode the chunk i on input 0
		m_pStimulationDecoder.decode(0,i);
		// the decoder may have decoded 3 different parts : the header, a buffer or the end of stream.
		if(m_pStimulationDecoder.isHeaderReceived())
		{
		}
		else if (m_pStimulationDecoder.isBufferReceived())
		{
			IStimulationSet* l_pStimulationSet = m_pStimulationDecoder.getOutputStimulationSet();
			for (uint32 j=0; j<l_pStimulationSet->getStimulationCount(); j++)
			{
				if (l_pStimulationSet->getStimulationIdentifier(j)==m_ui64StimulationIdentifier && !m_bStopSending)
				{
					m_bStopSending = true;
					l_pDynamicBoxContext->getInputChunk(0, i, l_ui64StartTime, l_ui64EndTime, l_ui64ChunkSize, l_pChunkBuffer);
					l_pDynamicBoxContext->appendOutputChunkData(0, l_pChunkBuffer, l_ui64ChunkSize);
					l_pDynamicBoxContext->markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);	
				}
			}
		}
		else if(m_pStimulationDecoder.isEndReceived())
		{	
		}
		
		if(!m_bStopSending)
		{
			l_pDynamicBoxContext->getInputChunk(0, i, l_ui64StartTime, l_ui64EndTime, l_ui64ChunkSize, l_pChunkBuffer);
			l_pDynamicBoxContext->appendOutputChunkData(0, l_pChunkBuffer, l_ui64ChunkSize);
			l_pDynamicBoxContext->markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);
		}
		l_pDynamicBoxContext->markInputAsDeprecated(0, i);	
			
	}
	
	return true;
}
