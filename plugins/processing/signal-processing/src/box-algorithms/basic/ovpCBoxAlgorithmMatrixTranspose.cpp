#include "ovpCBoxAlgorithmMatrixTranspose.h"
#include <cstdlib>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

boolean CBoxAlgorithmMatrixTranspose::initialize(void)
{
	m_oDecoder.initialize(*this,0);
	m_oEncoder.initialize(*this,0);

	return true;
}

boolean CBoxAlgorithmMatrixTranspose::uninitialize(void)
{
	m_oEncoder.uninitialize();
	m_oDecoder.uninitialize();

	return true;
}

boolean CBoxAlgorithmMatrixTranspose::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CBoxAlgorithmMatrixTranspose::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	for(uint32 chunk=0; chunk<l_rDynamicBoxContext.getInputChunkCount(0); chunk++)
	{
		m_oDecoder.decode(chunk);
		
		if(m_oDecoder.isHeaderReceived())
		{
			if(m_oDecoder.getOutputMatrix()->getDimensionCount()!=2)
			{
				this->getLogManager() << LogLevel_Error << "Only 2-dimensional matrices supported\n";
				return false;
			}
			const IMatrix* l_pInput = m_oDecoder.getOutputMatrix();
			IMatrix* l_pOutput = m_oEncoder.getInputMatrix();

			l_pOutput->setDimensionCount(2);
			l_pOutput->setDimensionSize(0, l_pInput->getDimensionSize(1));
			l_pOutput->setDimensionSize(1, l_pInput->getDimensionSize(0));

			this->getLogManager() << LogLevel_Trace << "Output matrix will be [" 
				<< l_pOutput->getDimensionSize(0) << "x"
				<< l_pOutput->getDimensionSize(1) << "]\n";

			for(uint32 j=0;j<l_pOutput->getDimensionSize(0);j++) 
			{
				l_pOutput->setDimensionLabel(0, j, l_pInput->getDimensionLabel(1, j));
			}
			for(uint32 j=0;j<l_pOutput->getDimensionSize(1);j++) 
			{
				l_pOutput->setDimensionLabel(1, j, l_pInput->getDimensionLabel(0, j));
			}

			m_oEncoder.encodeHeader();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, chunk), l_rDynamicBoxContext.getInputChunkEndTime(0, chunk));
		}

		if(m_oDecoder.isBufferReceived())
		{
			const IMatrix* l_pInput = m_oDecoder.getOutputMatrix();
			IMatrix* l_pOutput = m_oEncoder.getInputMatrix();

			const uint32 l_ui32nCols = l_pInput->getDimensionSize(0);
			const uint32 l_ui32nRows = l_pInput->getDimensionSize(1);

			const float64* l_pInputBuffer = l_pInput->getBuffer(); 
			float64* l_pOutputBuffer = l_pOutput->getBuffer(); 

			for(uint32 i=0;i<l_ui32nRows;i++)
			{
				for(uint32 j=0;j<l_ui32nCols;j++)
				{
					l_pOutputBuffer[j*l_ui32nRows+i] = l_pInputBuffer[i*l_ui32nCols+j];
				}
			}

			m_oEncoder.encodeBuffer();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, chunk), l_rDynamicBoxContext.getInputChunkEndTime(0, chunk));
		}


		if(m_oDecoder.isEndReceived())
		{
			m_oEncoder.encodeEnd();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, chunk), l_rDynamicBoxContext.getInputChunkEndTime(0, chunk));
		}
	}

	return true;
}
