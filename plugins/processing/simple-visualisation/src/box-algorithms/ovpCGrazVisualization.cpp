#include "ovpCGrazVisualization.h"

#include <cmath>
#include <iostream>

#include <glib.h>
#include <glib/gprintf.h>

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

		void CGrazVisualization::setStimulationCount(const uint32 ui32StimulationCount)
		{
			/* TODO nothing? */
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

			switch(ui64StimulationIdentifier)
			{
				case Stimulation_Idle:
					m_eCurrentState = EGrazVisualizationState_Idle;
					l_bStateUpdated = true;
					break;

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
					m_eCurrentState = EGrazVisualizationState_ContinousFeedback;
#if 1
					m_ui32WindowIndex = 0;
					m_vAmplitude.clear();
#endif
					l_bStateUpdated = true;
					break;

				case OVTK_StimulationId_ExperimentStop:
					m_eCurrentState = EGrazVisualizationState_End;
					l_bStateUpdated = true;
					break;

				case OVTK_StimulationId_Label_01:
					m_ui64RecognizedClassLabel = OVTK_StimulationId_Label_01;
					break;

				case OVTK_StimulationId_Label_02:
					m_ui64RecognizedClassLabel = OVTK_StimulationId_Label_02;
					break;

				case OVTK_StimulationId_Label_03:
					m_ui64RecognizedClassLabel = OVTK_StimulationId_Label_03;
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
					//m_f64MaxAmplitude = -DBL_MAX;
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
			m_pStimulationReaderCallBack(NULL),
			m_pClassReaderCallBack(NULL),
			m_pStreamedMatrixReaderCallBack(NULL),
			m_eCurrentState(EGrazVisualizationState_Idle),
			m_eCurrentDirection(EArrowDirection_None),
			m_f64MaxAmplitude(-DBL_MAX),
			m_f64BarScale(0.0),
			m_bError(false),
			m_pOriginalBar(NULL),
			m_pOriginalVerticalBar(NULL),
			m_pLeftBar(NULL),
			m_pRightBar(NULL),
			m_pDownBar(NULL),
			m_pUpBar(NULL),
			m_pOriginalLeftArrow(NULL),
			m_pOriginalRightArrow(NULL),
			m_pOriginalUpArrow(NULL),
			m_pOriginalDownArrow(NULL),
			m_pLeftArrow(NULL),
			m_pRightArrow(NULL),
			m_pUpArrow(NULL),
			m_pDownArrow(NULL),
			m_pOriginalImageLeft(NULL),
			m_pOriginalImageRight(NULL),
			m_pImageLeft(NULL),
			m_pImageRight(NULL),
			m_bShowInstruction(true),
			m_bShowFeedback(false),
			m_bOnlyPositiveFeedback(false),
			m_ui32FirstNb(0),
			m_ui32SecondNb(0)

		{
			m_pReader[0] = NULL;
			m_pReader[1] = NULL;
			m_pReader[2] = NULL;

			m_oBackgroundColor.pixel = 0;
			m_oBackgroundColor.red = 0;//0xFFFF;
			m_oBackgroundColor.green = 0;//0xFFFF;
			m_oBackgroundColor.blue = 0;//0xFFFF;

			m_oForegroundColor.pixel = 0;
			m_oForegroundColor.red = 0xFFFF; //0;
			m_oForegroundColor.green = 0xFFFF; //0x8000;
			m_oForegroundColor.blue = 0xFFFF; //0;
		}

		boolean CGrazVisualization::initialize()
		{
			CString l_sShowInstruction;
			if(getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(0, l_sShowInstruction))
			{
				m_bShowInstruction=(l_sShowInstruction==CString("true")?true:false);
			}
			CString l_sShowFeedback;
			if(getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(1, l_sShowFeedback))
			{
				m_bShowFeedback=(l_sShowFeedback==CString("true")?true:false);
			}

			CString l_sPositiveFeedback;
			if(getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(2, l_sPositiveFeedback))
			{
				m_bOnlyPositiveFeedback=(l_sPositiveFeedback==CString("true")?true:false);
			}

			uint32 l_ui32SettingCount = this->getBoxAlgorithmContext()->getStaticBoxContext()->getSettingCount();
			uint32 l_ui32ClassCount = l_ui32SettingCount - 3;

			for(uint32 i = 3; i<l_ui32ClassCount+3; i++)
			  {
			    CString l_sClassLabel = FSettingValueAutoCast(*this->getBoxAlgorithmContext(),i);
			    m_vClassLabel.push_back(this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation,l_sClassLabel));
			  }

			m_pStimulationReaderCallBack=createBoxAlgorithmStimulationInputReaderCallback(*this);
			m_pReader[0] = EBML::createReader(*m_pStimulationReaderCallBack);

			m_pClassReaderCallBack=createBoxAlgorithmStimulationInputReaderCallback(*this);
			m_pReader[1] = EBML::createReader(*m_pClassReaderCallBack);

			m_pStreamedMatrixReaderCallBack = createBoxAlgorithmStreamedMatrixInputReaderCallback(*this);
			m_pReader[2] =EBML::createReader(*m_pStreamedMatrixReaderCallBack);



			//load the gtk builder interface
			m_pBuilderInterface=gtk_builder_new(); // glade_xml_new(OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-GrazVisualization.ui", NULL, NULL);
			gtk_builder_add_from_file(m_pBuilderInterface, OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-GrazVisualization.ui", NULL);

			if(!m_pBuilderInterface)
			{
				g_warning("Couldn't load the interface!");
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

			//images

			m_pOriginalImageLeft = gdk_pixbuf_new_from_file_at_size(OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-hand.png",  -1, -1, NULL);
			m_pOriginalImageRight = gdk_pixbuf_new_from_file_at_size(OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-3D-geometric-figure.png",  -1, -1, NULL);

			if(!m_pOriginalLeftArrow || !m_pOriginalRightArrow || !m_pOriginalUpArrow || !m_pOriginalDownArrow || !m_pOriginalImageLeft || !m_pOriginalImageRight)
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning <<"Error couldn't load arrow ressource files!\n";
				m_bError = true;

				return false;
			}

			//bar
			m_pOriginalBar = gdk_pixbuf_new_from_file_at_size(OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-GrazVisualization-bar.png", -1, -1, NULL);
			m_pOriginalVerticalBar = gdk_pixbuf_new_from_file_at_size(OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-GrazVisualization-vertical-bar.png", -1, -1, NULL);
			if(!m_pOriginalBar || !m_pOriginalVerticalBar)
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning <<"Error couldn't load bar ressource file!\n";
				m_bError = true;

				return false;
			}

#if 0
			gtk_widget_show_all(m_pMainWindow);
#endif
			getBoxAlgorithmContext()->getVisualisationContext()->setWidget(m_pDrawingArea);

			return true;
		}

		boolean CGrazVisualization::uninitialize()
		{
			releaseBoxAlgorithmStimulationInputReaderCallback(m_pStimulationReaderCallBack);

			//release the ebml reader
			m_pReader[0]->release();
			m_pReader[0]=NULL;
			m_pReader[1]->release();
			m_pReader[1]=NULL;
			m_pReader[2]->release();
			m_pReader[2]=NULL;

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
			if(m_pOriginalVerticalBar){ g_object_unref(G_OBJECT(m_pOriginalVerticalBar)); }
			if(m_pLeftBar){ g_object_unref(G_OBJECT(m_pLeftBar)); }
			if(m_pRightBar){ g_object_unref(G_OBJECT(m_pRightBar)); }
			if(m_pDownBar){ g_object_unref(G_OBJECT(m_pDownBar)); }
			if(m_pUpBar){ g_object_unref(G_OBJECT(m_pUpBar)); }
			if(m_pLeftArrow){ g_object_unref(G_OBJECT(m_pLeftArrow)); }
			if(m_pRightArrow){ g_object_unref(G_OBJECT(m_pRightArrow)); }
			if(m_pUpArrow){	g_object_unref(G_OBJECT(m_pUpArrow)); }
			if(m_pDownArrow){ g_object_unref(G_OBJECT(m_pDownArrow)); }
			if(m_pOriginalLeftArrow){ g_object_unref(G_OBJECT(m_pOriginalLeftArrow)); }
			if(m_pOriginalRightArrow){ g_object_unref(G_OBJECT(m_pOriginalRightArrow)); }
			if(m_pOriginalUpArrow){	g_object_unref(G_OBJECT(m_pOriginalUpArrow)); }
			if(m_pOriginalDownArrow){ g_object_unref(G_OBJECT(m_pOriginalDownArrow)); }
			if(m_pOriginalImageLeft){ g_object_unref(G_OBJECT(m_pOriginalImageLeft)); }
			if(m_pOriginalImageRight){ g_object_unref(G_OBJECT(m_pOriginalImageRight)); }
			if(m_pImageLeft){ g_object_unref(G_OBJECT(m_pImageLeft)); }
			if(m_pImageRight){ g_object_unref(G_OBJECT(m_pImageRight)); }

			return true;
		}

		boolean CGrazVisualization::processInput(uint32 ui32InputIndex)
		{
			if(m_bError)
			{
				return false;
			}

			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
			return true;
		}

		boolean CGrazVisualization::process()
		{
			IBoxIO* l_pBoxIO=getBoxAlgorithmContext()->getDynamicBoxContext();

			for(uint32 input=0 ; input<getBoxAlgorithmContext()->getStaticBoxContext()->getInputCount() ; input++)
			{
				for(uint32 chunk=0; chunk<l_pBoxIO->getInputChunkCount(input); chunk++)
				{
					uint64 l_ui64ChunkSize;
					const uint8* l_pChunkBuffer=NULL;

					if(l_pBoxIO->getInputChunk(input, chunk, m_ui64StartTime, m_ui64EndTime, l_ui64ChunkSize, l_pChunkBuffer))
					{
						m_pReader[input]->processData(l_pChunkBuffer, l_ui64ChunkSize);
						l_pBoxIO->markInputAsDeprecated(input, chunk);
					}
				}
			}

			return true;
		}

		void CGrazVisualization::redraw()
		{
			switch(m_eCurrentState)
			{
				case EGrazVisualizationState_Reference:
					m_ui32FirstNb = rand()%99 + 101;
					m_ui32SecondNb = rand()%9 + 11;
					drawReferenceCross();
					drawClassImages();
					break;

				case EGrazVisualizationState_Cue:
					drawReferenceCross();
					drawClassImages();
					drawArrow(m_bShowInstruction?m_eCurrentDirection:EArrowDirection_None);
					drawRectangle(m_bShowInstruction?m_eCurrentDirection:EArrowDirection_None);

					break;

				case EGrazVisualizationState_ContinousFeedback:
					drawReferenceCross();
					drawClassImages();
					if(m_bShowFeedback) drawBar();
					drawRectangle(m_bShowInstruction?m_eCurrentDirection:EArrowDirection_None);
					break;

				default:
					break;
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

		void CGrazVisualization::drawClassImages()
		{

			gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			gint l_iWindowHeight = m_pDrawingArea->allocation.height;

			gint l_iLeftX = (l_iWindowWidth/2) - gdk_pixbuf_get_width(m_pLeftArrow)- gdk_pixbuf_get_width(m_pImageLeft);
			gint l_iLeftY = (l_iWindowHeight/2) - (gdk_pixbuf_get_height(m_pImageLeft)/2);

			gint l_iRightX = (l_iWindowWidth/2) + gdk_pixbuf_get_width(m_pImageRight);
			gint l_iRightY = (l_iWindowHeight/2) - (gdk_pixbuf_get_height(m_pImageLeft)/2);

			gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pImageLeft, 0, 0, l_iLeftX, l_iLeftY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
			gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pImageRight, 0, 0, l_iRightX, l_iRightY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);


			gchar l_pValueLabel[40];

			PangoContext * l_pPangoContext = gtk_widget_get_pango_context(m_pDrawingArea);
			PangoFontDescription * l_pFontDescription = pango_context_get_font_description(l_pPangoContext);
			pango_font_description_set_absolute_size(l_pFontDescription, 60 * PANGO_SCALE);

			g_sprintf(l_pValueLabel,"%i - %i", m_ui32FirstNb, m_ui32SecondNb);

			PangoLayout * l_pText = gtk_widget_create_pango_layout(m_pDrawingArea, l_pValueLabel);

			gint l_iTextX = (l_iWindowWidth/2) - (l_iWindowWidth/16) - 25;
			gint l_iTextY = (l_iWindowHeight/4) - gdk_pixbuf_get_height(m_pUpArrow)*3/4;

			gdk_draw_layout(m_pDrawingArea->window, m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE (m_pDrawingArea)], l_iTextX, l_iTextY, l_pText);

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
					// this->drawArrow(EArrowDirection_Left);
					// this->drawArrow(EArrowDirection_Right);
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

		}

		void CGrazVisualization::drawBar()
		{
			gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			gint l_iWindowHeight = m_pDrawingArea->allocation.height;

			gint l_iRectangleWidth = static_cast<gint>(fabs(l_iWindowWidth * fabs(m_f64BarScale) / 2));

			l_iRectangleWidth = (l_iRectangleWidth>(l_iWindowWidth/2)) ? (l_iWindowWidth/2) : l_iRectangleWidth;

			gint l_iRectangleHeight = l_iWindowHeight/6;

			gint l_iRectangleTopLeftX = l_iWindowWidth / 2;
			gint l_iRectangleTopLeftY = (l_iWindowHeight/2)-(l_iRectangleHeight/2);


			if(m_f64BarScale<0 && m_eCurrentDirection==EArrowDirection_Left && m_ui64RecognizedClassLabel == m_vClassLabel[0])
			{
				l_iRectangleTopLeftX -= l_iRectangleWidth;

				gdk_pixbuf_render_to_drawable(m_pLeftBar, m_pDrawingArea->window, NULL,
						gdk_pixbuf_get_width(m_pLeftBar)-l_iRectangleWidth, 0,
						l_iRectangleTopLeftX, l_iRectangleTopLeftY, l_iRectangleWidth, l_iRectangleHeight,
						GDK_RGB_DITHER_NONE, 0, 0);
			}
			else if(m_f64BarScale<0 && m_eCurrentDirection==EArrowDirection_Right && m_ui64RecognizedClassLabel == m_vClassLabel[2])
			{
				gdk_pixbuf_render_to_drawable(m_pRightBar, m_pDrawingArea->window, NULL, 0, 0, l_iRectangleTopLeftX, l_iRectangleTopLeftY, l_iRectangleWidth, l_iRectangleHeight, GDK_RGB_DITHER_NONE, 0, 0);

			}
			else if (m_f64BarScale<0 && m_eCurrentDirection==EArrowDirection_Up && m_ui64RecognizedClassLabel == m_vClassLabel[1])
			{

			    gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_pUpBar, 0, l_iRectangleWidth, l_iWindowWidth/2 - gdk_pixbuf_get_width(m_pUpBar)/2, l_iWindowHeight/2 - l_iRectangleWidth, -1, l_iRectangleWidth, GDK_RGB_DITHER_NONE, 0, 0);
			}

		}

		void CGrazVisualization::drawPositiveBar()
		{

		}

		void CGrazVisualization::drawRectangle(EArrowDirection eDirection)
		{


		  gint l_iWindowWidth = m_pDrawingArea->allocation.width;
		  gint l_iWindowHeight = m_pDrawingArea->allocation.height;

		  gint l_iX = 0;
		  gint l_iY = 0;

		  switch(eDirection)
		  {
			  case EArrowDirection_None:
				  break;

			  case EArrowDirection_Left:
				  l_iX = (l_iWindowWidth/2) - gdk_pixbuf_get_width(m_pLeftArrow) - 1;
				  l_iY = (l_iWindowHeight/2) - (gdk_pixbuf_get_height(m_pLeftArrow)/2);

				  gdk_draw_rectangle(m_pDrawingArea->window, m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE (m_pDrawingArea)], false,
						     l_iX - (gdk_pixbuf_get_width(m_pLeftArrow)/2) - gdk_pixbuf_get_width(m_pImageLeft)/2 + 10, l_iY,
						     gdk_pixbuf_get_width(m_pImageLeft)-10, gdk_pixbuf_get_height(m_pImageLeft));
				  break;

			  case EArrowDirection_Right:
				  l_iX = (l_iWindowWidth/2) + 2;
				  l_iY = (l_iWindowHeight/2) - (gdk_pixbuf_get_height(m_pRightArrow)/2);

				  gdk_draw_rectangle(m_pDrawingArea->window, m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE (m_pDrawingArea)], false,
						     l_iX + (gdk_pixbuf_get_width(m_pRightArrow)/2) + gdk_pixbuf_get_width(m_pImageRight)/2 , l_iY,
						     gdk_pixbuf_get_width(m_pImageRight) - 15, gdk_pixbuf_get_height(m_pImageLeft));
				  break;

			  case EArrowDirection_Up:

				  gdk_draw_rectangle(m_pDrawingArea->window, m_pDrawingArea->style->fg_gc[GTK_WIDGET_STATE (m_pDrawingArea)], false,
						     (l_iWindowWidth/2) - (l_iWindowWidth/16) - 25 - 20, (l_iWindowHeight/4) - gdk_pixbuf_get_height(m_pUpArrow)*3/4 - 15,
						     gdk_pixbuf_get_width(m_pImageRight)*3/4, gdk_pixbuf_get_height(m_pImageRight)/2);
				  break;

			  default:
				  break;
		    }

		}

		void CGrazVisualization::setMatrixDimensionCount(const uint32 ui32DimensionCount)
		{
			if(ui32DimensionCount != 1)
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning <<"Error, dimension count isn't 1 for Amplitude input !\n";
				m_bError = true;
			}

		}

		void CGrazVisualization::setMatrixDimensionSize(const uint32 ui32DimensionIndex, const uint32 ui32DimensionSize)
		{
			if(ui32DimensionSize != 1)
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning <<"Error, dimension size isn't 1 for Amplitude input !\n";
				m_bError = true;
			}
		}

		void CGrazVisualization::setMatrixDimensionLabel(const uint32 ui32DimensionIndex, const uint32 ui32DimensionEntryIndex, const char* sDimensionLabel)
		{
			/* nothing to do */
		}

		void CGrazVisualization::setMatrixBuffer(const float64* pBuffer)
		{
			if(m_bError)
			{
				return;
			}

			float64 l_f64CurrentAmplitude = *pBuffer;

#if 1
			l_f64CurrentAmplitude=0;
			m_vAmplitude.push_back(*pBuffer);
			if(m_vAmplitude.size()>5)
			{
				m_vAmplitude.pop_front();
			}

			for(std::deque<float64>::iterator a=m_vAmplitude.begin(); a!=m_vAmplitude.end(); a++)
			{
				l_f64CurrentAmplitude+=*a;
			}
			l_f64CurrentAmplitude/=m_vAmplitude.size();

			if(m_eCurrentState==EGrazVisualizationState_ContinousFeedback)
			{
			    if(!m_ui32WindowIndex)
				{
				uint32 l_ui32WindowCount=
						m_vWindowSuccessCount.size()>m_vWindowFailCount.size()?
						m_vWindowSuccessCount.size():m_vWindowFailCount.size();

					for(uint32 i=0; i<l_ui32WindowCount; i++)
					{
						getBoxAlgorithmContext()->getPlayerContext()->getLogManager()
							<< LogLevel_Info
							<< "Score estimation window " << i << " : [fail:success:ratio]=["
							<< m_vWindowFailCount[i] << ":"
							<< m_vWindowSuccessCount[i] << ":"
							<< (((m_vWindowSuccessCount[i]*10000)/(m_vWindowSuccessCount[i]+m_vWindowFailCount[i])))/100.0<<"%]\n";
					}
				}

			    if((m_eCurrentDirection==EArrowDirection_Left && m_ui64RecognizedClassLabel != m_vClassLabel[0] )
				|| (m_eCurrentDirection==EArrowDirection_Right && m_ui64RecognizedClassLabel != m_vClassLabel[2])
			       || (m_eCurrentDirection==EArrowDirection_Up && m_ui64RecognizedClassLabel != m_vClassLabel[1])
			       || (l_f64CurrentAmplitude > 0))
				{
					m_vWindowFailCount[m_ui32WindowIndex]++;
				}

				if((m_eCurrentDirection==EArrowDirection_Left && m_ui64RecognizedClassLabel == m_vClassLabel[0] && l_f64CurrentAmplitude<0)
				|| (m_eCurrentDirection==EArrowDirection_Right && m_ui64RecognizedClassLabel == m_vClassLabel[2] && l_f64CurrentAmplitude<0)
				|| (m_eCurrentDirection==EArrowDirection_Up && m_ui64RecognizedClassLabel == m_vClassLabel[1] && l_f64CurrentAmplitude<0))
				{
					m_vWindowSuccessCount[m_ui32WindowIndex]++;
				}

				m_ui32WindowIndex++;

				if(fabs(l_f64CurrentAmplitude) > m_f64MaxAmplitude)
				{
					m_f64MaxAmplitude = fabs(l_f64CurrentAmplitude);
				}

				m_f64BarScale = (l_f64CurrentAmplitude/m_f64MaxAmplitude);

				gdk_window_invalidate_rect(m_pDrawingArea->window,
						NULL,
						true);
			}

			// To get the score of last trial
			if(m_eCurrentState == EGrazVisualizationState_End)
			  {
			    if(m_ui32WindowIndex == 60)
				{
			    uint32 l_ui32WindowCount=
					    m_vWindowSuccessCount.size()>m_vWindowFailCount.size()?
					    m_vWindowSuccessCount.size():m_vWindowFailCount.size();

				    for(uint32 i=0; i<l_ui32WindowCount; i++)
				    {
					    getBoxAlgorithmContext()->getPlayerContext()->getLogManager()
						    << LogLevel_Info
						    << "Score estimation window " << i << " : [fail:success:ratio]=["
						    << m_vWindowFailCount[i] << ":"
						    << m_vWindowSuccessCount[i] << ":"
						    << (((m_vWindowSuccessCount[i]*10000)/(m_vWindowSuccessCount[i]+m_vWindowFailCount[i])))/100.0<<"%]\n";
				    }
			      }
			    m_ui32WindowIndex = 0;
			  }

#endif
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

			if(m_pImageLeft)
			  {
			    g_object_unref(G_OBJECT(m_pImageLeft));
			  }

			if(m_pImageRight)
			  {
			    g_object_unref(G_OBJECT(m_pImageRight));
			  }

			m_pLeftArrow = gdk_pixbuf_scale_simple(m_pOriginalLeftArrow, (2*ui32Width)/8, ui32Height/4, GDK_INTERP_BILINEAR);
			m_pRightArrow = gdk_pixbuf_scale_simple(m_pOriginalRightArrow, (2*ui32Width)/8, ui32Height/4, GDK_INTERP_BILINEAR);
			m_pUpArrow = gdk_pixbuf_scale_simple(m_pOriginalUpArrow, ui32Width/4, (2*ui32Height)/8, GDK_INTERP_BILINEAR);
			m_pDownArrow = gdk_pixbuf_scale_simple(m_pOriginalDownArrow, ui32Width/4, (2*ui32Height)/8, GDK_INTERP_BILINEAR);
			m_pImageLeft = gdk_pixbuf_scale_simple(m_pOriginalImageLeft, (2*ui32Width)/8, ui32Height/4, GDK_INTERP_BILINEAR);
			m_pImageRight = gdk_pixbuf_scale_simple(m_pOriginalImageRight, (2*ui32Width)/8, ui32Height/4, GDK_INTERP_BILINEAR);

			m_pRightBar = gdk_pixbuf_scale_simple(m_pOriginalBar, ui32Width, ui32Height/6, GDK_INTERP_BILINEAR);
			m_pLeftBar = gdk_pixbuf_flip(m_pRightBar, true);

			m_pDownBar = gdk_pixbuf_scale_simple(m_pOriginalVerticalBar, ui32Height/6, ui32Width, GDK_INTERP_BILINEAR);
			m_pUpBar = gdk_pixbuf_flip(m_pDownBar, true);
		}
	};
};


