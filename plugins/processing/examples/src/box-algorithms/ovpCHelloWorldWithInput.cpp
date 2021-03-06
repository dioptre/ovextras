/*
 * Prints user-specified greeting to the log every time process() is called. Passes the signal through. 
 */ 
#include "ovpCHelloWorldWithInput.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEPlugins::Examples;

void CHelloWorldWithInput::release(void)
{
	delete this;
}

bool CHelloWorldWithInput::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

bool CHelloWorldWithInput::process(void)
{
	const IBox* l_pStaticBoxContext=getBoxAlgorithmContext()->getStaticBoxContext();
	IBoxIO* l_pDynamicBoxContext=getBoxAlgorithmContext()->getDynamicBoxContext();

	uint64 l_ui64StartTime=0;
	uint64 l_ui64EndTime=0;
	uint64 l_ui64ChunkSize=0;
	const uint8* l_pChunkBuffer=NULL;

	for(uint32 i=0; i<l_pStaticBoxContext->getInputCount(); i++)
	{
		for(uint32 j=0; j<l_pDynamicBoxContext->getInputChunkCount(i); j++)
		{
			l_pDynamicBoxContext->getInputChunk(i, j, l_ui64StartTime, l_ui64EndTime, l_ui64ChunkSize, l_pChunkBuffer);
			l_pDynamicBoxContext->appendOutputChunkData(i, l_pChunkBuffer, l_ui64ChunkSize);
			l_pDynamicBoxContext->markOutputAsReadyToSend(i, l_ui64StartTime, l_ui64EndTime);
			l_pDynamicBoxContext->markInputAsDeprecated(i, j);
		}
	}

	const CString l_sMyGreeting = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	getLogManager() << OpenViBE::Kernel::LogLevel_Info << ": " << l_sMyGreeting << "\n";

	return true;
}
