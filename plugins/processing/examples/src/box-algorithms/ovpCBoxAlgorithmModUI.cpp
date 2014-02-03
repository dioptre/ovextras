#include "ovpCBoxAlgorithmModUI.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Examples;

boolean CBoxAlgorithmModUI::initialize(void)
{
	m_ui64StimulationCode = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_ui64Factor = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	// Signal stream decoder
	m_oSignalDecoder.initialize(*this);
	// Signal stream encoder
	m_oSignalEncoder.initialize(*this);
	// Stimulation stream encoder
	m_oAlgo2_StimulationEncoder.initialize(*this);
	
	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmModUI::uninitialize(void)
{
	m_oSignalDecoder.uninitialize();
	m_oSignalEncoder.uninitialize();
	m_oAlgo2_StimulationEncoder.uninitialize();

	return true;
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
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//you have to get back the setting value from the box context at each cycle
	uint64 l_ui64OldValue = m_ui64Factor;
	m_ui64Factor= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	boolean l_bValueChanged = !(l_ui64OldValue==m_ui64Factor);

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
			IMatrix* l_ipMatrix = m_oSignalEncoder.getInputMatrix();

			OpenViBEToolkit::Tools::Matrix::copy(*l_ipMatrix, *l_pMatrix);
			
			// Pass the header to the next boxes, by encoding a header on the output 0:
			m_oSignalEncoder.encodeHeader(0);
			m_oAlgo2_StimulationEncoder.encodeHeader(1);
			// send the output chunk containing the header. The dates are the same as the input chunk:
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
		if(m_oSignalDecoder.isBufferReceived())
		{
			// Buffer received. For example the signal values
			// Access to the buffer can be done thanks to :
			IMatrix* l_pMatrix = m_oSignalDecoder.getOutputMatrix(); // the StreamedMatrix of samples.

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

			if(l_bValueChanged)
			{
				getLogManager() << LogLevel_Trace << "value changed\n";
				IStimulationSet* l_oStimulationSet = m_oAlgo2_StimulationEncoder.getInputStimulationSet();
				l_oStimulationSet->appendStimulation(m_ui64StimulationCode, getPlayerContext().getCurrentTime() ,0);
				m_oAlgo2_StimulationEncoder.encodeBuffer(1);
				l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));

			}
			
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
