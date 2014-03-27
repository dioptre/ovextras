#include "ovpCBoxAlgorithmMouseTracking.h"

#include <openvibe/ovITimeArithmetics.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Tools;
using namespace OpenViBEToolkit;


void motion_event_handler(GtkWidget* widget, GdkEventMotion* event, gpointer data);

boolean CBoxAlgorithmMouseTracking::initialize(void)
{
	// Feature vector stream encoder
	m_oAlgo0_SignalEncoder.initialize(*this);
	// Feature vector stream encoder
	m_oAlgo1_SignalEncoder.initialize(*this);

	m_oAlgo0_SignalEncoder.getInputMatrix().setReferenceTarget(m_pAbsoluteCoordinateBuffer);
	m_oAlgo1_SignalEncoder.getInputMatrix().setReferenceTarget(m_pRelativeCoordinateBuffer);

	// Allocates coordinates Matrix
	m_pAbsoluteCoordinateBuffer=new CMatrix();
	m_pRelativeCoordinateBuffer=new CMatrix();

	// Retrieves settings
	m_f64Frequency = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_ui32GeneratedEpochSampleCount = FSettingValueAutoCast(*this->getBoxAlgorithmContext(),1);
//	m_ui32GeneratedEpochSampleCount = 1;
	m_ui32SentSampleCount = 0;

	m_f64Previous_x = 0;
	m_f64Previous_y = 0;

	// Creates empty window to get mouse position
	m_myWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_add_events(m_myWindow, GDK_POINTER_MOTION_MASK);
	gtk_window_fullscreen(GTK_WINDOW(m_myWindow));
	gtk_widget_show_all(m_myWindow);

	g_signal_connect(m_myWindow, "motion-notify-event", G_CALLBACK(motion_event_handler), this);

	m_bHeaderSent=false;
	
	m_oAlgo0_SignalEncoder.getInputSamplingRate()=m_f64Frequency;
	m_oAlgo1_SignalEncoder.getInputSamplingRate()=m_f64Frequency;

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmMouseTracking::uninitialize(void)
{
	m_oAlgo0_SignalEncoder.uninitialize();
	m_oAlgo1_SignalEncoder.uninitialize();

	delete m_pAbsoluteCoordinateBuffer;
	m_pAbsoluteCoordinateBuffer=NULL;

	delete m_pRelativeCoordinateBuffer;
	m_pRelativeCoordinateBuffer=NULL;

	gtk_widget_destroy(m_myWindow);

	return true;
}
/*******************************************************************************/


boolean CBoxAlgorithmMouseTracking::processClock(IMessageClock& rMessageClock)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}
/*******************************************************************************/


uint64 CBoxAlgorithmMouseTracking::getClockFrequency(void)
{
	// Note that the time is coded on a 64 bits unsigned integer, fixed decimal point (32:32)
//	return (uint64)((double)(1LL<<32) * m_f64Frequency); // the box clock frequency chosen by user

	// Intentional parameter swap to get the frequency
	m_f64ClockFrequency = ITimeArithmetics::sampleCountToTime(m_ui32GeneratedEpochSampleCount, m_f64Frequency);
	
	return m_f64ClockFrequency;
}

/*******************************************************************************/

boolean CBoxAlgorithmMouseTracking::process(void)
{

	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
	
	uint32 l_ui32NbChannel = 2;

	//Send header and initialize Matrix
	if(!m_bHeaderSent)
	{
		m_pAbsoluteCoordinateBuffer->setDimensionCount(2);
		m_pAbsoluteCoordinateBuffer->setDimensionSize(0,2);
		m_pAbsoluteCoordinateBuffer->setDimensionSize(1, m_ui32GeneratedEpochSampleCount);
		m_pAbsoluteCoordinateBuffer->setDimensionLabel(0,0, "x");
		m_pAbsoluteCoordinateBuffer->setDimensionLabel(0,1, "y");

		m_pRelativeCoordinateBuffer->setDimensionCount(2);
		m_pRelativeCoordinateBuffer->setDimensionSize(0,2);
		m_pRelativeCoordinateBuffer->setDimensionSize(1, m_ui32GeneratedEpochSampleCount);
		m_pRelativeCoordinateBuffer->setDimensionLabel(0,0, "x");
		m_pRelativeCoordinateBuffer->setDimensionLabel(0,1, "y");


		m_oAlgo0_SignalEncoder.encodeHeader(0);
		m_oAlgo1_SignalEncoder.encodeHeader(1);

		m_bHeaderSent=true;

		l_rDynamicBoxContext.markOutputAsReadyToSend(0, 0, 0);
		l_rDynamicBoxContext.markOutputAsReadyToSend(1, 0, 0);
	}
	else //Do the process and send coordinates to output
	{
		uint32 l_ui32SentSampleCount=m_ui32SentSampleCount;


		for(uint32 i=0; i<m_ui32GeneratedEpochSampleCount; i=i++)
		{
			m_pAbsoluteCoordinateBuffer->getBuffer()[0*m_ui32GeneratedEpochSampleCount+i] = m_Mouse_x;
			m_pAbsoluteCoordinateBuffer->getBuffer()[1*m_ui32GeneratedEpochSampleCount+i] = m_Mouse_y;

			m_pRelativeCoordinateBuffer->getBuffer()[0*m_ui32GeneratedEpochSampleCount+i] = m_Mouse_x - m_f64Previous_x;
			m_pRelativeCoordinateBuffer->getBuffer()[1*m_ui32GeneratedEpochSampleCount+i] = m_Mouse_y - m_f64Previous_y;
		}

		m_f64Previous_x = m_Mouse_x;
		m_f64Previous_y = m_Mouse_y;

		m_ui32SentSampleCount+=m_ui32GeneratedEpochSampleCount;

		uint64 l_ui64StartTime = ITimeArithmetics::sampleCountToTime(m_f64Frequency, l_ui32SentSampleCount);
		uint64 l_ui64EndTime = ITimeArithmetics::sampleCountToTime(m_f64Frequency, m_ui32SentSampleCount);

		m_oAlgo0_SignalEncoder.encodeBuffer(0);
		m_oAlgo1_SignalEncoder.encodeBuffer(1);

		l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);
		l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_ui64StartTime, l_ui64EndTime);
	}
	return true;
}

// CALLBACK
// Get mouse position
void motion_event_handler(GtkWidget* widget, GdkEventMotion* event, gpointer data)
{
	CBoxAlgorithmMouseTracking* l_pBox = static_cast < CBoxAlgorithmMouseTracking* >(data);
	l_pBox->m_Mouse_x = event->x;
	l_pBox->m_Mouse_y = event->y;
}


