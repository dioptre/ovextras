#include "ovpCBoxAlgorithmMessageSpy.h"
#include <sstream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Tools;

boolean CBoxAlgorithmMessageSpy::initialize(void)
{

	uint64  l_ui64LogLevel=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_eLogLevel=static_cast<ELogLevel>(l_ui64LogLevel);
	
	// If you need to, you can manually set the reference targets to link the codecs input and output. To do so, you can use :
	//m_oEncoder.getInputX().setReferenceTarget(m_oDecoder.getOutputX())
	// Where 'X' depends on the codec type. Please refer to the Codec Toolkit Reference Page
	// (http://openvibe.inria.fr/documentation/unstable/Doc_Tutorial_Developer_SignalProcessing_CodecToolkit_Ref.html) for a complete list.
	
	// If you need to retrieve setting values, use the FSettingValueAutoCast function.
	// For example :
	// - CString setting at index 0 in the setting list :
	// CString l_sSettingValue = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	// - unsigned int64 setting at index 1 in the setting list :
	// uint64 l_ui64SettingValue = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	// - float64 setting at index 2 in the setting list :
	// float64 l_f64SettingValue = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	// ...

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmMessageSpy::uninitialize(void)
{

	return true;
}
/*******************************************************************************/


boolean CBoxAlgorithmMessageSpy::processClock(IMessageClock& rMessageClock)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/


uint64 CBoxAlgorithmMessageSpy::getClockFrequency(void)
{
	// Note that the time is coded on a 64 bits unsigned integer, fixed decimal point (32:32)
	return 1LL<<32; // the box clock frequency
}
/*******************************************************************************/

/*
boolean CBoxAlgorithmMessageSpy::processInput(uint32 ui32InputIndex)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
//*/
OpenViBE::boolean CBoxAlgorithmMessageSpy::processMessage(const IMyMessage& msg, uint32 inputIndex)
{
	CString l_sMessageContent("content\n");
	bool success;
	const CString* l_sKey;
	//get the content
	//get first key

	l_sKey = msg.getFirstCStringToken();
	if(l_sKey==NULL)
	{
		l_sMessageContent = l_sMessageContent+CString("No string value\n");
	}
	else
	{
		while(l_sKey!=NULL)
		{
			const CString* l_sValue = msg.getValueCString(*l_sKey, success);
			l_sMessageContent = l_sMessageContent+(*l_sKey)+CString(" ")+(*l_sValue)+CString("\n");
			l_sKey = msg.getNextCStringToken(*l_sKey);
		}
	}

	l_sKey = msg.getFirstUInt64Token();
	if(l_sKey==NULL)
	{
		l_sMessageContent = l_sMessageContent+CString("No UInt64 value\n");
	}
	else
	{
		while(l_sKey!=NULL)
		{
			const uint64 l_ui64Value = msg.getValueUint64(*l_sKey, success);
			//easier way to convert uint -> char* -> CString ?
			std::stringstream l_oStream;
			l_oStream << l_ui64Value;
			l_sMessageContent = l_sMessageContent+(*l_sKey)+CString(" ")+CString(l_oStream.str().c_str())+CString("\n");
			l_sKey = msg.getNextUInt64Token(*l_sKey);
		}
	}

	l_sKey = msg.getFirstFloat64Token();
	if(l_sKey==NULL)
	{
		l_sMessageContent = l_sMessageContent+CString("No Float64 value\n");
	}
	else
	{
		while(l_sKey!=NULL)
		{
			const float64 l_f64Value = msg.getValueFloat64(*l_sKey, success);
			//easier way to convert Float -> char* -> CString ?
			std::stringstream l_oStream;
			l_oStream << l_f64Value;
			l_sMessageContent = l_sMessageContent+(*l_sKey)+CString(" ")+CString(l_oStream.str().c_str())+CString("\n");
			l_sKey = msg.getNextFloat64Token(*l_sKey);
		}
	}
//*
	l_sKey = msg.getFirstIMatrixToken();
	if(l_sKey==NULL)
	{
		l_sMessageContent = l_sMessageContent+CString("No IMatrix value\n");
	}
	else
	{
		while(l_sKey!=NULL)
		{
			const IMatrix* l_oMatrix = msg.getValueCMatrix(*l_sKey, success);
			//easier way to convert Float -> char* -> CString ?
			std::stringstream l_oStream;
			const float64* l_f64Buffer = l_oMatrix->getBuffer();
			float64 l_f64BufferSize = l_oMatrix->getBufferElementCount();
			for (uint64 i=0; i<l_f64BufferSize; i++)
			{
				l_oStream << l_f64Buffer[i] << " ";
			}
			l_sMessageContent = l_sMessageContent+(*l_sKey)+CString(" ")+CString(l_oStream.str().c_str())+CString("\n");
			l_sKey = msg.getNextIMatrixToken(*l_sKey);
		}
	}
	getLogManager() << m_eLogLevel << l_sMessageContent;
	return true;
}




/*******************************************************************************/

boolean CBoxAlgorithmMessageSpy::process(void)
{
	
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

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
	/*
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//iterate over all chunk on input 0
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		// decode the chunk i on input 0
		m_oSignalDecoder.decode(0,i);
		// the decoder may have decoded 3 different parts : the header, a buffer or the end of stream.
		if(m_oSignalDecoder.isHeaderReceived())
		{
			// Header received. This happens only once when pressing "play". For example with a StreamedMatrix input, you now know the dimension count, sizes, and labels of the matrix
			// ... maybe do some process ...
			
			// Pass the header to the next boxes, by encoding a header on the output 0:
			m_oSignalEncoder.encodeHeader(0);
			// send the output chunk containing the header. The dates are the same as the input chunk:
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
		if(m_oSignalDecoder.isBufferReceived())
		{
			// Buffer received. For example the signal values
			// Access to the buffer can be done thanks to :
			IMatrix* l_pMatrix = m_oSignalDecoder.getOutputMatrix(); // the StreamedMatrix of samples.
			uint64 l_uiSamplingFrequency = m_oSignalDecoder.getOutputSamplingRate(); // the sampling rate of the signal
			
			// ... do some process on the matrix ...

			// Encode the output buffer :
			m_oSignalEncoder.encodeBuffer(0);
			// and send it to the next boxes :
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			
		}
		if(m_oSignalDecoder.isEndReceived())
		{
			// End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
			m_oSignalEncoder.encodeEnd(0);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

		// The current input chunk has been processed, and automaticcaly discarded.
		// you don't need to call "l_rDynamicBoxContext.markInputAsDeprecated(0, i);"
	}
	*/

	// check the official developer documentation webpage for more example and information :
	
	// Tutorials:
	// http://openvibe.inria.fr/documentation/#Developer+Documentation
	// Codec Toolkit page :
	// http://openvibe.inria.fr/codec-toolkit-references/
	
	// Feel free to ask experienced developers on the forum (http://openvibe.inria.fr/forum) and IRC (#openvibe on irc.freenode.net).

	return true;
}
