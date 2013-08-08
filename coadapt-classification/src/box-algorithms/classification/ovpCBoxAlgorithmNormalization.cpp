#include "ovpCBoxAlgorithmNormalization.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;


boolean CBoxAlgorithmNormalization::initialize(void)
{
	// Stimulation stream encoder
	m_oAlgo0_StimulationEncoder.initialize(*this);
	// Streamed matrix stream decoder
	m_oAlgo1_StreamedMatrixDecoder.initialize(*this);
	m_oAlgo1_StreamedMatrixEncoder.initialize(*this);
	// We connect the Signal Input with the Signal Output so every chunk on the input will be copied to the output automatically.
    m_oAlgo1_StreamedMatrixEncoder.getInputMatrix().setReferenceTarget(m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix());
    //m_oAlgo1_StreamedMatrixEncoder.getInputSamplingRate().setReferenceTarget(m_oAlgo1_StreamedMatrixDecoder.getOutputSamplingRate());
 
	m_bFirstTime = false;

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmNormalization::uninitialize(void)
{
	m_oAlgo0_StimulationEncoder.uninitialize();

	m_oAlgo1_StreamedMatrixDecoder.uninitialize();
	m_oAlgo1_StreamedMatrixEncoder.uninitialize();

	return true;
}
/*******************************************************************************/

/*
boolean CBoxAlgorithmNormalization::processClock(IMessageClock& rMessageClock)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

/*
uint64 CBoxAlgorithmNormalization::getClockFrequency(void)
{
	// Note that the time is coded on a 64 bits unsigned integer, fixed decimal point (32:32)
	return 1LL<<32LL; // the box clock frequency
}
/*******************************************************************************/


boolean CBoxAlgorithmNormalization::processInput(uint32 ui32InputIndex)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmNormalization::process(void)
{
	
	IBox* l_pStaticBoxContext=getBoxAlgorithmContext()->getStaticBoxContext();
	IBoxIO* l_pDynamicBoxContext=getBoxAlgorithmContext()->getDynamicBoxContext();

	uint64 l_ui64StartTime=0;
	uint64 l_ui64EndTime=0;
	uint64 l_ui64ChunkSize=0;
	const uint8* l_pChunkBuffer=NULL;

	for(uint32 i=0; i<l_pStaticBoxContext->getInputCount(); i++)
	{
		for(uint32 j=0; j<l_pDynamicBoxContext->getInputChunkCount(i); j++)
		{
			m_oAlgo1_StreamedMatrixDecoder.decode(i,j);
			if(m_oAlgo1_StreamedMatrixDecoder.isHeaderReceived())
			{
				m_oAlgo1_StreamedMatrixEncoder.encodeHeader(i);
				l_pDynamicBoxContext->markOutputAsReadyToSend(i, l_pDynamicBoxContext->getInputChunkStartTime(i, j), l_pDynamicBoxContext->getInputChunkEndTime(i, j));
			}
			//----buffer---------------------------------------------------
			if(m_oAlgo1_StreamedMatrixDecoder.isBufferReceived())
			{
				//this->getLogManager() << LogLevel_Trace << "input vector received, look at the time ! \n";
				
				OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* >& l_mMatrix = m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix();

				float64* l_mMatrixRawBuffer = l_mMatrix->getBuffer();  

				//computing mean and var -------------
				float64 l_f64Mean = 0;
				float64 l_f64Var = 0;

				if (m_bFirstTime) 
					std::cout << l_pStaticBoxContext->getName() << " input " << i << ", chunk " << j << "\n";
				for (uint32 k=0;k<l_mMatrix->getBufferElementCount();k++)
				{
					l_f64Mean += l_mMatrixRawBuffer[k];
					l_f64Var += l_mMatrixRawBuffer[k]*l_mMatrixRawBuffer[k];
				}
				l_f64Mean /= l_mMatrix->getBufferElementCount();
				l_f64Var /= l_mMatrix->getBufferElementCount();

				
				l_f64Var -= l_f64Mean*l_f64Mean;

				
				//----normalization----------
				for (uint32 k=0;k<l_mMatrix->getBufferElementCount();k++)
				{
					l_mMatrixRawBuffer[k] -= l_f64Mean;
					l_mMatrixRawBuffer[k] /= sqrt(l_f64Var); 
				}

				if (m_bFirstTime)
					for (uint32 k=0;k<l_mMatrix->getBufferElementCount();k++)
						std::cout << l_mMatrixRawBuffer[k] << " ";				
				if (m_bFirstTime)
				{
					std::cout << "\n";
					m_bFirstTime = false;				
				}

				//sending
				m_oAlgo1_StreamedMatrixEncoder.encodeBuffer(i);
				l_pDynamicBoxContext->markOutputAsReadyToSend(i, l_pDynamicBoxContext->getInputChunkStartTime(i, j), l_pDynamicBoxContext->getInputChunkEndTime(i, j));
			}
			//end -----------------------------------------------------------------------------------------
			if(m_oAlgo1_StreamedMatrixDecoder.isEndReceived())
			{
				// End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
				m_oAlgo1_StreamedMatrixEncoder.encodeEnd(i);
				l_pDynamicBoxContext->markOutputAsReadyToSend(i, l_pDynamicBoxContext->getInputChunkStartTime(i, j), l_pDynamicBoxContext->getInputChunkEndTime(i, j));
			}

			l_pDynamicBoxContext->markInputAsDeprecated(i, j);
		}
	}

	return true;
}
