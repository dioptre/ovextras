#include "ovpCGrazVisualization.h"

#include <cmath>
#include <iostream>

#if defined TARGET_OS_Linux
 #include <unistd.h>
#endif

using namespace OpenViBE;
using namespace Plugins;
using namespace Kernel;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SimpleVisualisation;

using namespace OpenViBEToolkit;

using namespace std;

////////////////////////

#include <iostream>
#include <fstream>
#include <cstdlib>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <sys/timeb.h>

using boost::asio::ip::tcp;

/*
 * \class StimulusSender
 * \brief Basic illustratation of how to read from TCPWriter using boost::asio
 */
class StimulusSender {
public:
	~StimulusSender()
	{
		if(m_oStimulusSocket.is_open())
		{
			std::cout << "Disconnecting\n";
			m_oStimulusSocket.close();
		}
	}

	StimulusSender(void)
		: m_oStimulusSocket(m_ioService), m_bConnectedOnce(false)
	{
	}
	
	boolean connect(const char* sAddress, const char* sStimulusPort)
	{
		//m_pStimulusSocket = new tcp::socket(m_ioService);
		boost::system::error_code error;
		tcp::resolver resolver(m_ioService);
			
		// Stimulus port
		std::cout << "Connecting to stimulus port [" << sAddress << " : " << sStimulusPort << "]\n";
		tcp::resolver::query query = tcp::resolver::query(tcp::v4(), sAddress, sStimulusPort);
		m_oStimulusSocket.connect(*resolver.resolve(query), error);
		if(error)
		{
			std::cout << "Connection error: " << error << "\n";
			return false;
		}
		
		m_bConnectedOnce = true;

		return true;
	}

	boolean sendStimuli(uint64 ui64Stimuli) 
	{
		if(!m_bConnectedOnce) {
			return false;
		}	

		timeb time_buffer;
		ftime(&time_buffer);
		const uint64 posixTime = time_buffer.time*1000ULL + time_buffer.millitm;

		if(!m_oStimulusSocket.is_open())
		{
			std::cout << "Cannot send stimulation, socket is not open\n";
			return false;
		}

		uint64 l_ui64tmp = 0;
		try
		{
			boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&l_ui64tmp, sizeof(uint64)));
			boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&ui64Stimuli, sizeof(uint64)));
			//boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&posixTime, sizeof(uint64)));
			boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&l_ui64tmp, sizeof(uint64)));
		} 
		catch (boost::system::system_error l_oError) 
		{
			std::cout << "Issue '" << l_oError.code().message().c_str() << "' with writing stimulus to server\n";
		}

		return true;
	}

private:

	boost::asio::io_service m_ioService;

	tcp::socket m_oStimulusSocket;

	OpenViBE::boolean m_bConnectedOnce;
};

//////////////////////////

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{
		gboolean GrazVisualization_SizeAllocateCallback(GtkWidget *widget, GtkAllocation *allocation, gpointer data)
		{
			reinterpret_cast<CGrazVisualization*>(data)->resize((uint32)allocation->width, (uint32)allocation->height);
			return FALSE;
		}

		gboolean GrazVisualization_RedrawCallback(GtkWidget *widget, GdkEventExpose *event, gpointer data)
		{
			reinterpret_cast<CGrazVisualization*>(data)->redraw();
			return TRUE;
		}

		void CGrazVisualization::setStimulation(const uint32 ui32StimulationIndex, const uint64 ui64StimulationIdentifier, const uint64 ui64StimulationDate)
		{
			/*
			OVTK_GDF_Start_Of_Trial
			OVTK_GDF_Cross_On_Screen
			OVTK_GDF_Left
			OVTK_GDF_Right
			*/
			boolean l_bStateUpdated = false;

			m_ui64LastStimulation = ui64StimulationIdentifier;
			switch(ui64StimulationIdentifier)
			{
				case OVTK_GDF_End_Of_Trial:
					m_eCurrentState = EGrazVisualizationState_Idle;
					l_bStateUpdated = true;
					if(m_bShowAccuracy || m_bDelayFeedback)
					{
						const float64 l_f64Prediction = aggregatePredictions(true);
						updateConfusionMatrix(l_f64Prediction);
						m_f64BarScale = l_f64Prediction;
					}
					break;
					m_pStimulusSender->sendStimuli(m_ui64LastStimulation);

				case OVTK_GDF_End_Of_Session:
					m_eCurrentState = EGrazVisualizationState_Idle;
					l_bStateUpdated = true;
					if(m_bShowFeedback)
					{
						m_f64BarScale = 0;
						drawBar();
					}
					break;
					m_pStimulusSender->sendStimuli(m_ui64LastStimulation);

				case OVTK_GDF_Cross_On_Screen:
					m_eCurrentState = EGrazVisualizationState_Reference;
					l_bStateUpdated = true;
					break;

				case OVTK_GDF_Beep:
					// gdk_beep();
					getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Trace << "Beep is no more considered in 'Graz Visu', use the 'Sound player' for this!\n";
#if 0
#if defined TARGET_OS_Linux
					system("cat /local/ov_beep.wav > /dev/dsp &");
#endif
#endif
					break;

				case OVTK_GDF_Left:
					m_eCurrentState = EGrazVisualizationState_Cue;
					m_eCurrentDirection = EArrowDirection_Left;
					l_bStateUpdated = true;
					break;

				case OVTK_GDF_Right:
					m_eCurrentState = EGrazVisualizationState_Cue;
					m_eCurrentDirection = EArrowDirection_Right;
					l_bStateUpdated = true;
					break;

				case OVTK_GDF_Up:
					m_eCurrentState = EGrazVisualizationState_Cue;
					m_eCurrentDirection = EArrowDirection_Up;
					l_bStateUpdated = true;
					break;

				case OVTK_GDF_Down:
					m_eCurrentState = EGrazVisualizationState_Cue;
					m_eCurrentDirection = EArrowDirection_Down;
					l_bStateUpdated = true;
					break;

				case OVTK_GDF_Feedback_Continuous:
					// New trial starts

					m_eCurrentState = EGrazVisualizationState_ContinousFeedback;
					m_vAmplitude.clear();

					// as some trials may have artifacts and hence very high responses from e.g. LDA
					// its better to reset the max between trials
					m_f64MaxAmplitude = -DBL_MAX;
					m_f64BarScale = 0;

					l_bStateUpdated = true;
					break;
			}

			if(l_bStateUpdated)
			{
				processState();
			}
		}

		void CGrazVisualization::processState()
		{
			switch(m_eCurrentState)
			{
				case EGrazVisualizationState_Reference:
					if(GTK_WIDGET(m_pDrawingArea)->window)
						gdk_window_invalidate_rect(GTK_WIDGET(m_pDrawingArea)->window,
								NULL,
								true);
					break;

				case EGrazVisualizationState_Cue:
					if(GTK_WIDGET(m_pDrawingArea)->window)
						gdk_window_invalidate_rect(GTK_WIDGET(m_pDrawingArea)->window,
								NULL,
								true);
					break;

				case EGrazVisualizationState_Idle:
					if(GTK_WIDGET(m_pDrawingArea)->window)
						gdk_window_invalidate_rect(GTK_WIDGET(m_pDrawingArea)->window,
								NULL,
								true);
					break;

				case EGrazVisualizationState_ContinousFeedback:
					if(GTK_WIDGET(m_pDrawingArea)->window)
						gdk_window_invalidate_rect(GTK_WIDGET(m_pDrawingArea)->window,
								NULL,
								true);
					break;

				default:
					break;
			}
		}

		/**
		* Constructor
		*/
		CGrazVisualization::CGrazVisualization(void) :
			m_pBuilderInterface(NULL),
			m_pMainWindow(NULL),
			m_pDrawingArea(NULL),
			m_eCurrentState(EGrazVisualizationState_Idle),
			m_eCurrentDirection(EArrowDirection_None),
			m_f64MaxAmplitude(-DBL_MAX),
			m_f64BarScale(0.0),
			m_bTwoValueInput(false),
			m_pOriginalBar(NULL),
			m_pLeftBar(NULL),
			m_pRightBar(NULL),
			m_pOriginalLeftArrow(NULL),
			m_pOriginalRightArrow(NULL),
			m_pOriginalUpArrow(NULL),
			m_pOriginalDownArrow(NULL),
			m_pLeftArrow(NULL),
			m_pRightArrow(NULL),
			m_pUpArrow(NULL),
			m_pDownArrow(NULL),
			m_bShowInstruction(true),
			m_bShowFeedback(false),
			m_bDelayFeedback(false),
			m_bShowAccuracy(false),
			m_bPositiveFeedbackOnly(false),
			m_i64PredictionsToIntegrate(5),
			m_pStimulusSender(NULL),
			m_ui64LastStimulation(0)
		{
			m_oBackgroundColor.pixel = 0;
			m_oBackgroundColor.red = 0;//0xFFFF;
			m_oBackgroundColor.green = 0;//0xFFFF;
			m_oBackgroundColor.blue = 0;//0xFFFF;

			m_oForegroundColor.pixel = 0;
			m_oForegroundColor.red = 0;
			m_oForegroundColor.green = 0x8000;
			m_oForegroundColor.blue = 0;

			m_oConfusion.setDimensionCount(2);
			m_oConfusion.setDimensionSize(0,2);
			m_oConfusion.setDimensionSize(1,2);
		}

		boolean CGrazVisualization::initialize()
		{
			m_bShowInstruction            = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			m_bShowFeedback               = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
			m_bDelayFeedback              = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
			m_bShowAccuracy               = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
			m_i64PredictionsToIntegrate   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
			m_bPositiveFeedbackOnly       = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

			if(m_i64PredictionsToIntegrate<1) 
			{
				this->getLogManager() << LogLevel_Error << "Number of predictions to integrate must be at least 1!";
				return false;
			}

			m_oStimulationDecoder.initialize(*this,0);
			m_oMatrixDecoder.initialize(*this,1);

			OpenViBEToolkit::Tools::Matrix::clearContent(m_oConfusion);

			//load the gtk builder interface
			m_pBuilderInterface=gtk_builder_new(); // glade_xml_new(OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-GrazVisualization.ui", NULL, NULL);
			gtk_builder_add_from_file(m_pBuilderInterface, OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-GrazVisualization.ui", NULL);

			if(!m_pBuilderInterface)
			{
				this->getLogManager() << LogLevel_Error << "Error: couldn't load the interface!";
				return false;
			}

			gtk_builder_connect_signals(m_pBuilderInterface, NULL);

			m_pDrawingArea = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "GrazVisualizationDrawingArea"));
			g_signal_connect(G_OBJECT(m_pDrawingArea), "expose_event", G_CALLBACK(GrazVisualization_RedrawCallback), this);
			g_signal_connect(G_OBJECT(m_pDrawingArea), "size-allocate", G_CALLBACK(GrazVisualization_SizeAllocateCallback), this);

#if 0
			//does nothing on the main window if the user tries to close it
			g_signal_connect (G_OBJECT(gtk_builder_get_object(m_pBuilderInterface, "GrazVisualizationWindow")),
					"delete_event",
					G_CALLBACK(gtk_widget_do_nothing), NULL);

			//creates the window
			m_pMainWindow = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "GrazVisualizationWindow"));
#endif

			//set widget bg color
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oBackgroundColor);

			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oForegroundColor);

			//arrows
			m_pOriginalLeftArrow  = gdk_pixbuf_new_from_file_at_size(OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-GrazVisualization-leftArrow.png",  -1, -1, NULL);
			m_pOriginalRightArrow = gdk_pixbuf_new_from_file_at_size(OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-GrazVisualization-rightArrow.png", -1, -1, NULL);
			m_pOriginalUpArrow    = gdk_pixbuf_new_from_file_at_size(OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-GrazVisualization-upArrow.png",    -1, -1, NULL);
			m_pOriginalDownArrow  = gdk_pixbuf_new_from_file_at_size(OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-GrazVisualization-downArrow.png",  -1, -1, NULL);

			if(!m_pOriginalLeftArrow || !m_pOriginalRightArrow || !m_pOriginalUpArrow || !m_pOriginalDownArrow)
			{
				this->getLogManager() << LogLevel_Error << "Error couldn't load arrow resource files!\n";

				return false;
			}

			//bar
			m_pOriginalBar = gdk_pixbuf_new_from_file_at_size(OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-GrazVisualization-bar.png", -1, -1, NULL);
			if(!m_pOriginalBar)
			{
				this->getLogManager() << LogLevel_Error <<"Error couldn't load bar resource file!\n";

				return false;
			}

#if 0
			gtk_widget_show_all(m_pMainWindow);
#endif
			getBoxAlgorithmContext()->getVisualisationContext()->setWidget(m_pDrawingArea);
			
			m_pStimulusSender = new StimulusSender();

			if(!m_pStimulusSender->connect("localhost", "15361"))
			{
				this->getLogManager() << LogLevel_Warning << "Unable to connect to AS TCP Tagging, stimuli wont be forwarded.\n";
			}

			return true;
		}

		boolean CGrazVisualization::uninitialize()
		{
			if(m_pStimulusSender)
			{
				delete m_pStimulusSender;
			}

			m_oStimulationDecoder.uninitialize();
			m_oMatrixDecoder.uninitialize();
			
#if 0
			//destroy the window and its children
			if(m_pMainWindow)
			{
				gtk_widget_destroy(m_pMainWindow);
				m_pMainWindow = NULL;
			}
#endif
			//destroy drawing area
			if(m_pDrawingArea)
			{
				gtk_widget_destroy(m_pDrawingArea);
				m_pDrawingArea = NULL;
			}

			/* unref the xml file as it's not needed anymore */
			g_object_unref(G_OBJECT(m_pBuilderInterface));
			m_pBuilderInterface=NULL;

			if(m_pOriginalBar){ g_object_unref(G_OBJECT(m_pOriginalBar)); }
			if(m_pLeftBar){ g_object_unref(G_OBJECT(m_pLeftBar)); }
			if(m_pRightBar){ g_object_unref(G_OBJECT(m_pRightBar)); }
			if(m_pLeftArrow){ g_object_unref(G_OBJECT(m_pLeftArrow)); }
			if(m_pRightArrow){ g_object_unref(G_OBJECT(m_pRightArrow)); }
			if(m_pUpArrow){	g_object_unref(G_OBJECT(m_pUpArrow)); }
			if(m_pDownArrow){ g_object_unref(G_OBJECT(m_pDownArrow)); }
			if(m_pOriginalLeftArrow){ g_object_unref(G_OBJECT(m_pOriginalLeftArrow)); }
			if(m_pOriginalRightArrow){ g_object_unref(G_OBJECT(m_pOriginalRightArrow)); }
			if(m_pOriginalUpArrow){	g_object_unref(G_OBJECT(m_pOriginalUpArrow)); }
			if(m_pOriginalDownArrow){ g_object_unref(G_OBJECT(m_pOriginalDownArrow)); }

			return true;
		}

		boolean CGrazVisualization::processInput(uint32 ui32InputIndex)
		{
			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
			return true;
		}

		boolean CGrazVisualization::process()
		{
			const IBoxIO* l_pBoxIO=getBoxAlgorithmContext()->getDynamicBoxContext();

			for(uint32 chunk=0; chunk<l_pBoxIO->getInputChunkCount(0); chunk++)
			{
				m_oStimulationDecoder.decode(chunk);
				if(m_oStimulationDecoder.isBufferReceived())
				{
					const IStimulationSet* l_pStimulationSet = m_oStimulationDecoder.getOutputStimulationSet();
					for(uint32 s=0;s<l_pStimulationSet->getStimulationCount();s++)
					{
						setStimulation(s, 
							l_pStimulationSet->getStimulationIdentifier(s),
							l_pStimulationSet->getStimulationDate(s));
					}
				}
			}
			
			for(uint32 chunk=0; chunk<l_pBoxIO->getInputChunkCount(1); chunk++)
			{
				m_oMatrixDecoder.decode(chunk);
				if(m_oMatrixDecoder.isHeaderReceived())
				{
					const IMatrix* l_pMatrix = m_oMatrixDecoder.getOutputMatrix();
	
					if(l_pMatrix->getDimensionCount() == 0)
					{
						this->getLogManager() << LogLevel_Error << "Error, dimension count is 0 for Amplitude input !\n";
						return false;
					}

					if(l_pMatrix->getDimensionCount() > 1)
					{
						for(uint32 k=1;k<l_pMatrix->getDimensionSize(k);k++)
						{
							if(l_pMatrix->getDimensionSize(k) > 1)
							{
								this->getLogManager() << LogLevel_Error << "Error, only column vectors supported as Amplitude!\n";
								return false;
							}
						}
					}

					if(l_pMatrix->getDimensionSize(0) == 0)
					{
						this->getLogManager() << LogLevel_Error << "Error, need at least 1 dimension in Amplitude input !\n";
						return false;
					}
					else if(l_pMatrix->getDimensionSize(0) >= 2)
					{
						this->getLogManager() << LogLevel_Trace << "Got 2 or more dimensions for feedback, feedback will be the difference between the first two.\n";
						m_bTwoValueInput = true;
					}
				}

				if(m_oMatrixDecoder.isBufferReceived())
				{
					setMatrixBuffer(m_oMatrixDecoder.getOutputMatrix()->getBuffer());
				}

			}

			return true;
		}

		void CGrazVisualization::redraw()
		{
			switch(m_eCurrentState)
			{
				case EGrazVisualizationState_Reference:
					drawReferenceCross();
					m_pStimulusSender->sendStimuli(m_ui64LastStimulation);
					break;

				case EGrazVisualizationState_Cue:
					drawReferenceCross();
					drawArrow(m_bShowInstruction?m_eCurrentDirection:EArrowDirection_None);
					break;

				case EGrazVisualizationState_ContinousFeedback:
					drawReferenceCross();
					if(m_bShowFeedback && !m_bDelayFeedback) 
					{
						drawBar();
					}
					break;

				case EGrazVisualizationState_Idle:
					if(m_bShowFeedback && m_bDelayFeedback)
					{
						drawBar();
					}
					break;

				default:
					break;
			}
			if(m_bShowAccuracy)
			{
				drawAccuracy();
			}

		}

		void CGrazVisualization::drawReferenceCross()
		{
			gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			gint l_iWindowHeight = m_pDrawingArea->allocation.height;

			//increase line's width
			gdk_gc_set_line_attributes(m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE (m_pDrawingArea)], 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL);

			//horizontal line
			gdk_draw_line(m_pDrawingArea->window,
					m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE(m_pDrawingArea)],
					(l_iWindowWidth/4), (l_iWindowHeight/2),
					((3*l_iWindowWidth)/4), (l_iWindowHeight/2)
					);
			 //vertical line
			gdk_draw_line(m_pDrawingArea->window,
					m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE (m_pDrawingArea)],
					(l_iWindowWidth/2), (l_iWindowHeight/4),
					(l_iWindowWidth/2), ((3*l_iWindowHeight)/4)
					);

			//increase line's width
			gdk_gc_set_line_attributes(m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE (m_pDrawingArea)], 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL);

		}

		void CGrazVisualization::drawArrow(EArrowDirection eDirection)
		{
			gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			gint l_iWindowHeight = m_pDrawingArea->allocation.height;

			gint l_iX = 0;
			gint l_iY = 0;

			switch(eDirection)
			{
				case EArrowDirection_None:
					this->drawArrow(EArrowDirection_Left);
					this->drawArrow(EArrowDirection_Right);
					// this->drawArrow(EArrowDirection_Up);
					// this->drawArrow(EArrowDirection_Down);
					break;

				case EArrowDirection_Left:
					l_iX = (l_iWindowWidth/2) - gdk_pixbuf_get_width(m_pLeftArrow) - 1;
					l_iY = (l_iWindowHeight/2) - (gdk_pixbuf_get_height(m_pLeftArrow)/2);
					gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pLeftArrow, 0, 0, l_iX, l_iY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
					break;

				case EArrowDirection_Right:
					l_iX = (l_iWindowWidth/2) + 2;
					l_iY = (l_iWindowHeight/2) - (gdk_pixbuf_get_height(m_pRightArrow)/2);
					gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pRightArrow, 0, 0, l_iX, l_iY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
					break;

				case EArrowDirection_Up:
					l_iX = (l_iWindowWidth/2) - (gdk_pixbuf_get_width(m_pUpArrow)/2);
					l_iY = (l_iWindowHeight/2) - gdk_pixbuf_get_height(m_pUpArrow) - 1;
					gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pUpArrow, 0, 0, l_iX, l_iY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
					break;

				case EArrowDirection_Down:
					l_iX = (l_iWindowWidth/2) - (gdk_pixbuf_get_width(m_pDownArrow)/2);
					l_iY = (l_iWindowHeight/2) + 2;
					gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pDownArrow, 0, 0, l_iX, l_iY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
					break;

				default:
					break;
			}
			m_pStimulusSender->sendStimuli(m_ui64LastStimulation);
		}

		void CGrazVisualization::drawBar()
		{
			const gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			const gint l_iWindowHeight = m_pDrawingArea->allocation.height;

			float64 l_f64UsedScale = m_f64BarScale;
			if(m_bPositiveFeedbackOnly)
			{
				// @fixme for multiclass
				const uint32 l_ui32TrueDirection  = m_eCurrentDirection - 1;
				const uint32 l_ui32ThisVote = (m_f64BarScale < 0 ? 0 : 1);
				if(l_ui32TrueDirection != l_ui32ThisVote)
				{
					l_f64UsedScale = 0;
				}
			}

			gint l_iRectangleWidth = static_cast<gint>(fabs(l_iWindowWidth * fabs(l_f64UsedScale) / 2));

			l_iRectangleWidth = (l_iRectangleWidth>(l_iWindowWidth/2)) ? (l_iWindowWidth/2) : l_iRectangleWidth;

			const gint l_iRectangleHeight = l_iWindowHeight/6;

			gint l_iRectangleTopLeftX = l_iWindowWidth / 2;
			const gint l_iRectangleTopLeftY = (l_iWindowHeight/2)-(l_iRectangleHeight/2);

			if(m_f64BarScale<0)
			{
				l_iRectangleTopLeftX -= l_iRectangleWidth;

				gdk_pixbuf_render_to_drawable(m_pLeftBar, m_pDrawingArea->window, NULL,
						gdk_pixbuf_get_width(m_pLeftBar)-l_iRectangleWidth, 0,
						l_iRectangleTopLeftX, l_iRectangleTopLeftY, l_iRectangleWidth, l_iRectangleHeight,
						GDK_RGB_DITHER_NONE, 0, 0);
			}
			else
			{
				gdk_pixbuf_render_to_drawable(m_pRightBar, m_pDrawingArea->window, NULL, 0, 0, l_iRectangleTopLeftX, l_iRectangleTopLeftY, l_iRectangleWidth, l_iRectangleHeight, GDK_RGB_DITHER_NONE, 0, 0);

			}
		}

		void CGrazVisualization::drawAccuracy(void)
		{
			PangoLayout *layout;
			char tmp[512];
			layout = pango_layout_new(gdk_pango_context_get());

			const float64* l_pBuffer = m_oConfusion.getBuffer();

			sprintf(tmp, "L");
			pango_layout_set_text(layout, tmp, -1);
			gdk_draw_layout(m_pDrawingArea->window, m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE (m_pDrawingArea)], 8,     16, layout);

			sprintf(tmp, "%.3d", (int)l_pBuffer[0]);
			pango_layout_set_text(layout, tmp, -1);
			gdk_draw_layout(m_pDrawingArea->window, m_pDrawingArea->style->white_gc,                                 8+16,  16, layout);

			sprintf(tmp, "%.3d", (int)l_pBuffer[1]);
			pango_layout_set_text(layout, tmp, -1);
			gdk_draw_layout(m_pDrawingArea->window, m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE (m_pDrawingArea)], 8+56,  16, layout);

			sprintf(tmp, "R");
			pango_layout_set_text(layout, tmp, -1);
			gdk_draw_layout(m_pDrawingArea->window, m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE (m_pDrawingArea)], 8,     32, layout);

			sprintf(tmp, "%.3d", (int)l_pBuffer[2]);
			pango_layout_set_text(layout, tmp, -1);
			gdk_draw_layout(m_pDrawingArea->window, m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE (m_pDrawingArea)], 8+16,  32, layout);

			sprintf(tmp, "%.3d", (int)l_pBuffer[3]);
			pango_layout_set_text(layout, tmp, -1);
			gdk_draw_layout(m_pDrawingArea->window, m_pDrawingArea->style->white_gc,                                 8+56  ,32, layout);

			uint32 l_i32Predictions=0;
			for(uint32 i=0;i<4;i++) {
				l_i32Predictions += (int)l_pBuffer[i];
			}
	
			sprintf(tmp, "Acc = %3.1f%%", (l_i32Predictions == 0 ? 0 : 100.0*(l_pBuffer[0]+l_pBuffer[3])/(float64)l_i32Predictions));
			pango_layout_set_text(layout, tmp, -1);
			gdk_draw_layout(m_pDrawingArea->window, m_pDrawingArea->style->white_gc,                                 8+96, 32, layout);


			g_object_unref(layout);
		}

		float64 CGrazVisualization::aggregatePredictions(bool bIncludeAll)
		{
			float64 l_f64VoteAggregate = 0;

			// Do we have enough predictions to integrate a result?
			if(m_vAmplitude.size()>=m_i64PredictionsToIntegrate)
			{
				// step backwards with rev iter to take the latest samples
				uint64 count = 0;
				for(std::deque<float64>::reverse_iterator a=m_vAmplitude.rbegin(); 
					a!=m_vAmplitude.rend() && (bIncludeAll || count<m_i64PredictionsToIntegrate); a++,count++)
				{
					l_f64VoteAggregate += *a;
					m_f64MaxAmplitude = std::max<float64>(m_f64MaxAmplitude, abs(*a));
				}

				l_f64VoteAggregate /= m_f64MaxAmplitude;
				l_f64VoteAggregate /= count;

			}

			return l_f64VoteAggregate;
		}

		// @fixme for >2 classes
		void CGrazVisualization::updateConfusionMatrix(float64 f64VoteAggregate)
		{
			if(m_eCurrentDirection == EArrowDirection_Left || m_eCurrentDirection == EArrowDirection_Right)
			{
				const uint32 l_ui32TrueDirection  = m_eCurrentDirection - 1;

				const uint32 l_ui32ThisVote = (f64VoteAggregate < 0 ? 0 : 1);

				(m_oConfusion.getBuffer())[l_ui32TrueDirection*2 + l_ui32ThisVote]++;

				// std::cout << "Now " << l_ui32TrueDirection  << " vote " << l_ui32ThisVote << "\n";
			}

			// CString out;
			// OpenViBEToolkit::Tools::Matrix::toString(m_oConfusion,out);
			// this->getLogManager() << LogLevel_Info << "Trial conf " << out << "\n";
		}

		void CGrazVisualization::setMatrixBuffer(const float64* pBuffer)
		{
			if(m_eCurrentState != EGrazVisualizationState_ContinousFeedback)
			{
				// We're not inside a trial, discard the prediction
				return;
			}

			float64 l_f64PredictedAmplitude = 0;
			if(m_bTwoValueInput)
			{
				// Ad-hoc forcing to probability (range [0,1], sum to 1). This will make scaling easier 
				// if run forever in a continuous mode. If the input is already scaled this way, no effect.
				// 
				float64 l_f64Value0 = std::abs(pBuffer[0]);
				float64 l_f64Value1 = std::abs(pBuffer[1]);
				const float64 l_f64Sum = l_f64Value0 + l_f64Value1;
				if(l_f64Sum!=0) 
				{
					l_f64Value0 = l_f64Value0 / l_f64Sum;
					l_f64Value1 = l_f64Value1 / l_f64Sum;
				}
				else
				{
					l_f64Value0 = 0.5;
					l_f64Value1 = 0.5;
				}

//				printf("%f %f\n", l_f64Value0, l_f64Value1);

				l_f64PredictedAmplitude = l_f64Value1 - l_f64Value0;
			}
			else
			{
				l_f64PredictedAmplitude = pBuffer[0];
			}

			m_vAmplitude.push_back(l_f64PredictedAmplitude);

			if(m_bShowFeedback && !m_bDelayFeedback)
			{
				m_f64BarScale = aggregatePredictions(false);

				//printf("bs %f\n", m_f64BarScale);

				gdk_window_invalidate_rect(m_pDrawingArea->window,
					NULL,
					true);
			}
		}


		void CGrazVisualization::resize(uint32 ui32Width, uint32 ui32Height)
		{
			ui32Width =(ui32Width<8?8:ui32Width);
			ui32Height=(ui32Height<8?8:ui32Height);

			if(m_pLeftArrow)
			{
				g_object_unref(G_OBJECT(m_pLeftArrow));
			}

			if(m_pRightArrow)
			{
				g_object_unref(G_OBJECT(m_pRightArrow));
			}

			if(m_pUpArrow)
			{
				g_object_unref(G_OBJECT(m_pUpArrow));
			}

			if(m_pDownArrow)
			{
				g_object_unref(G_OBJECT(m_pDownArrow));
			}

			if(m_pRightBar)
			{
				g_object_unref(G_OBJECT(m_pRightBar));
			}

			if(m_pLeftBar)
			{
				g_object_unref(G_OBJECT(m_pLeftBar));
			}

			m_pLeftArrow = gdk_pixbuf_scale_simple(m_pOriginalLeftArrow, (2*ui32Width)/8, ui32Height/4, GDK_INTERP_BILINEAR);
			m_pRightArrow = gdk_pixbuf_scale_simple(m_pOriginalRightArrow, (2*ui32Width)/8, ui32Height/4, GDK_INTERP_BILINEAR);
			m_pUpArrow = gdk_pixbuf_scale_simple(m_pOriginalUpArrow, ui32Width/4, (2*ui32Height)/8, GDK_INTERP_BILINEAR);
			m_pDownArrow = gdk_pixbuf_scale_simple(m_pOriginalDownArrow, ui32Width/4, (2*ui32Height)/8, GDK_INTERP_BILINEAR);

			m_pRightBar = gdk_pixbuf_scale_simple(m_pOriginalBar, ui32Width, ui32Height/6, GDK_INTERP_BILINEAR);
			m_pLeftBar = gdk_pixbuf_flip(m_pRightBar, true);
		}
	};
};


/*
int main(int argc, char** argv)
{
	try {

		StimulusSender client("localhost", "15361");
		client.sendStimuli(666);

	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

    return 0;
}
*/

