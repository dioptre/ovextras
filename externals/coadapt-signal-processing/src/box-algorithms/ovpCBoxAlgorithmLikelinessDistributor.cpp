#include "ovpCBoxAlgorithmLikelinessDistributor.h"
#include <algorithm>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessingStatistics;

boolean CBoxAlgorithmLikelinessDistributor::initialize(void)
{
	// Streamed matrix stream encoder
	m_oAlgo0_StreamedMatrixEncoder.initialize(*this);
	// Streamed matrix stream decoder
	m_oAlgo1_StreamedMatrixDecoder.initialize(*this);
	
	// If you need to, you can manually set the reference targets to link the codecs input and output. To do so, you can use :
	//m_oEncoder.getInputX().setReferenceTarget(m_oDecoder.getOutputX())
	// Where 'X' depends on the codec type. Please refer to the Codec Toolkit Reference Page
	// (http://openvibe.inria.fr/documentation/unstable/Doc_Tutorial_Developer_SignalProcessing_CodecToolkit_Ref.html) for a complete list.
	
	// If you need to retrieve setting values, use the FSettingValueAutoCast function.
	// For example :
	m_ui32Index = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_ui32Number = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_f64Scale = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	// ...

	return true;
}

boolean CBoxAlgorithmLikelinessDistributor::uninitialize(void)
{
	m_oAlgo0_StreamedMatrixEncoder.uninitialize();
	m_oAlgo1_StreamedMatrixDecoder.uninitialize();

	return true;
}

boolean CBoxAlgorithmLikelinessDistributor::processInput(uint32 ui32InputIndex)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmLikelinessDistributor::process(void)
{
	
	// the static box context describes the box inputs, outputs, settings structures
	//IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	//IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	// here is some useful functions:
	// - To get input/output/setting count:
	// l_rStaticBoxContext.getInputCount();
	// l_rStaticBoxContext.getOutputCount();
	
	// - To get the number of chunks currently available on a particular input :
	// l_rDynamicBoxContext.getInputChunkCount(input_index)
	
	// - To send an output chunk :
	// l_rDynamicBoxContext.markOutputAsReadyToSend(output_index, chunk_start_time, chunk_end_time);
	
	
	// A typical process iteration may look like this.
	// This example only iterate over the first input of type Signal, and output a modified Signal.
	// thus, the box uses 1 decoder (m_oSignalDecoder) and 1 encoder (m_oSignalEncoder)
	
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//iterate over all chunk on input 0
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		// decode the chunk i on input 0
		m_oAlgo1_StreamedMatrixDecoder.decode(0,i);
		// the decoder may have decoded 3 different parts : the header, a buffer or the end of stream.
		if(m_oAlgo1_StreamedMatrixDecoder.isHeaderReceived())
		{
			//IMatrix* l_pHeaderMatrix = new CMatrix();
			m_oAlgo0_StreamedMatrixEncoder.getInputMatrix()->setDimensionCount(1);
			m_oAlgo0_StreamedMatrixEncoder.getInputMatrix()->setDimensionSize(0,(uint32)m_ui32Number);
			for (int m=0; i<m_ui32Number; i++)
				*(m_oAlgo0_StreamedMatrixEncoder.getInputMatrix()->getBuffer()+i) = 0;
			
			// Pass the header to the next boxes, by encoding a header on the output 0:
			m_oAlgo0_StreamedMatrixEncoder.encodeHeader(0);
			// send the output chunk containing the header. The dates are the same as the input chunk:
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
		if(m_oAlgo1_StreamedMatrixDecoder.isBufferReceived())
		{
			// Buffer received. For example the signal values
			// Access to the buffer can be done thanks to :
			IMatrix* l_pDecodedMatrix = m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix(); // the StreamedMatrix of samples.
			//IMatrix* l_pEncodedMatrix = new CMatrix();
			//m_oAlgo0_StreamedMatrixEncoder.getInputMatrix()->setDimensionCount(1);
			//m_oAlgo0_StreamedMatrixEncoder.getInputMatrix()->setDimensionSize(0,(uint32)m_ui32Number);
			if (l_pDecodedMatrix->getDimensionSize(0)==1) 
				*(m_oAlgo0_StreamedMatrixEncoder.getInputMatrix()->getBuffer()+m_ui32Index) = std::max(0.0,1.0-(float)m_f64Scale*(*l_pDecodedMatrix->getBuffer()));
			else
				this->getLogManager() << LogLevel_Warning << "Input matrix should be of size one by one" << "\n";

			// Encode the output buffer :
			//m_oAlgo0_StreamedMatrixEncoder.getInputMatrix() = l_pEncodedMatrix;
			m_oAlgo0_StreamedMatrixEncoder.encodeBuffer(0);
			// and send it to the next boxes :
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			
		}
		if(m_oAlgo1_StreamedMatrixDecoder.isEndReceived())
		{
			// End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
			m_oAlgo0_StreamedMatrixEncoder.encodeEnd(0);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

		// The current input chunk has been processed, and automaticcaly discarded.
		// you don't need to call "l_rDynamicBoxContext.markInputAsDeprecated(0, i);"
	}
	

	// check the official developer documentation webpage for more example and information :
	
	// Tutorials:
	// http://openvibe.inria.fr/documentation/#Developer+Documentation
	// Codec Toolkit page :
	// http://openvibe.inria.fr/codec-toolkit-references/
	
	// Feel free to ask experienced developers on the forum (http://openvibe.inria.fr/forum) and IRC (#openvibe on irc.freenode.net).

	return true;
}
