#include "ovpCBoxAlgorithmModUI.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Examples;

boolean CBoxAlgorithmModUI::initialize(void)
{

	m_ui64Factor = 1;
	// Signal stream decoder
	m_oSignalDecoder.initialize(*this);
	// Signal stream encoder
	m_oSignalEncoder.initialize(*this);
	// Stimulation stream encoder
	m_oAlgo2_StimulationEncoder.initialize(*this);
	
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

boolean CBoxAlgorithmModUI::uninitialize(void)
{
	getLogManager() << OpenViBE::Kernel::LogLevel_Info << "uninitialize " << m_ui64Factor <<"\n";
	m_oSignalDecoder.uninitialize();
	m_oSignalEncoder.uninitialize();
	m_oAlgo2_StimulationEncoder.uninitialize();

	return true;
}
/*******************************************************************************/

/*
boolean CBoxAlgorithmModUI::processClock(IMessageClock& rMessageClock)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

/*
uint64 CBoxAlgorithmModUI::getClockFrequency(void)
{
	// Note that the time is coded on a 64 bits unsigned integer, fixed decimal point (32:32)
	return 1LL<<32; // the box clock frequency
}
/*******************************************************************************/


boolean CBoxAlgorithmModUI::processInput(uint32 ui32InputIndex)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmModUI::process(void)
{
	
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//you have to get back the setting value from the box context at each cycle
	CString l_sFactor;
	getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(1, l_sFactor);
	m_ui64Factor= atoi(l_sFactor.toASCIIString());
	//getLogManager() << OpenViBE::Kernel::LogLevel_Info << "modUI box, factor is " << m_ui64Factor <<"\n";

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
			IMatrix* l_pMatrix = m_oSignalDecoder.getOutputMatrix(); // the StreamedMatrix of samples.
			uint64 l_uiSamplingFrequency = m_oSignalDecoder.getOutputSamplingRate(); // the sampling rate of the signal

			IMatrix* l_ipMatrix = m_oSignalEncoder.getInputMatrix();

			OpenViBEToolkit::Tools::Matrix::copy(*l_ipMatrix, *l_pMatrix);
			
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

			IMatrix* l_ipMatrix = m_oSignalEncoder.getInputMatrix();

			OpenViBEToolkit::Tools::Matrix::copy(*l_ipMatrix, *l_pMatrix);
			
			// ... do some process on the matrix ...
			float64 *l_f64Buffer = l_ipMatrix->getBuffer();
			uint64 l_ui64BufferSize = l_ipMatrix->getBufferElementCount();

			for (uint64 i=0; i<l_ui64BufferSize; i++)
			{
				l_f64Buffer[i]*=m_ui64Factor;
			}


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
	}

	return true;
}
