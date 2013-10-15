#include "ovpCBoxAlgorithmModUI.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Examples;


static void enter_callback( GtkWidget *widget,
							GtkWidget *entry )
{
  const gchar *entry_text;
  entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
  //printf ("Entry contents: %s\n", entry_text);
}

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





	::GtkBuilder* l_pBuilder=gtk_builder_new(); // glade_xml_new(OpenViBE::Directories::getDataDir() + "/plugins/stimulation/keyboard-stimulator.ui", NULL, NULL);
	gtk_builder_add_from_file(l_pBuilder, OpenViBE::Directories::getDataDir() + "/plugins/stimulation/modui.ui", NULL);
	if(!l_pBuilder)
	{
		g_warning("Couldn't load the interface!");
		return false;
	}
	gtk_builder_connect_signals(l_pBuilder, NULL);

	m_pWidget=GTK_WIDGET(gtk_builder_get_object(l_pBuilder, "settings_collection-hbox_setting_integer"));

	m_pEntry = GTK_WIDGET(gtk_builder_get_object(l_pBuilder, "settings_collection-entry_setting_integer_string"));

			g_signal_connect (m_pEntry, "activate",
						  G_CALLBACK (enter_callback),
						  m_pEntry);

	g_object_unref(l_pBuilder);

	this->getVisualisationContext().setWidget(m_pWidget);

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmModUI::uninitialize(void)
{
	m_oSignalDecoder.uninitialize();
	m_oSignalEncoder.uninitialize();
	m_oAlgo2_StimulationEncoder.uninitialize();

	if(m_pWidget)
	{
		g_object_unref(m_pWidget);
		m_pWidget = NULL;
	}

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
	//*
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();


	const gchar * l_sText = gtk_entry_get_text( GTK_ENTRY(m_pEntry) );

	//uint64 l_ui64Factor
	m_ui64Factor= atoi(l_sText);

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

		// The current input chunk has been processed, and automaticcaly discarded.
		// you don't need to call "l_rDynamicBoxContext.markInputAsDeprecated(0, i);"
	}
	//*/

	// check the official developer documentation webpage for more example and information :
	
	// Tutorials:
	// http://openvibe.inria.fr/documentation/#Developer+Documentation
	// Codec Toolkit page :
	// http://openvibe.inria.fr/codec-toolkit-references/
	
	// Feel free to ask experienced developers on the forum (http://openvibe.inria.fr/forum) and IRC (#openvibe on irc.freenode.net).

	return true;
}
