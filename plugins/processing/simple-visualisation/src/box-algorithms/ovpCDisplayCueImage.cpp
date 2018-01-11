#include "ovpCDisplayCueImage.h"

#include <cmath>
#include <iostream>
#include <cstdlib>

#if defined TARGET_OS_Linux
  #include <unistd.h>
#endif

#include <tcptagging/IStimulusSender.h>
#include <openvibe/ovITimeArithmetics.h>

#include <algorithm> // std::min

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
		// This callback flushes all accumulated stimulations to the TCP Tagging 
		// after the rendering has completed.
		gboolean DisplayCueImage_flush_callback(gpointer pUserData)
		{
			reinterpret_cast<CDisplayCueImage*>(pUserData)->flushQueue();

			return false;	// Only run once
		}

		gboolean DisplayCueImage_SizeAllocateCallback(GtkWidget *widget, GtkAllocation *allocation, gpointer data)
		{
			reinterpret_cast<CDisplayCueImage*>(data)->resize((uint32)allocation->width, (uint32)allocation->height);
			return FALSE;
		}

		gboolean DisplayCueImage_RedrawCallback(GtkWidget *widget, GdkEventExpose *event, gpointer data)
		{
			reinterpret_cast<CDisplayCueImage*>(data)->redraw();
			return TRUE;
		}

		CDisplayCueImage::CDisplayCueImage(void) :
			m_pBuilderInterface(NULL),
			m_pMainWindow(NULL),
			m_pDrawingArea(NULL),
			m_bImageRequested(false),
			m_int32RequestedImageID(-1),
			m_bImageDrawn(false),
			m_int32DrawnImageID(-1),
			m_bFullScreen(false),
			m_bScaleImages(false),
			m_ui64LastOutputChunkDate(0),
			m_pStimulusSender(nullptr),
			m_visualizationContext(nullptr)
		{
			m_oBackgroundColor.pixel = 0;
			m_oBackgroundColor.red = 0;
			m_oBackgroundColor.green = 0;
			m_oBackgroundColor.blue = 0;

			m_oForegroundColor.pixel = 0;
			m_oForegroundColor.red = 0xFFFF;
			m_oForegroundColor.green = 0xFFFF;
			m_oForegroundColor.blue = 0xFFFF;
		}

		bool CDisplayCueImage::initialize()
		{
			m_uiIdleFuncTag = 0;
			m_pStimulusSender = NULL;
			//>>>> Reading Settings:

			//Number of Cues:
			m_ui32NumberOfCues = (getStaticBoxContext().getSettingCount() - m_ui32NonCueSettingsCount) / 2;

			//Do we display the images in full screen?
			m_bFullScreen = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

			m_bScaleImages = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

			//Clear screen stimulation:
			m_ui64ClearScreenStimulation = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

			//Stimulation ID and images file names for each cue
			m_vImageNames.resize(m_ui32NumberOfCues);
			m_vStimulationsId.resize(m_ui32NumberOfCues);
			for(uint32 i=0; i<m_ui32NumberOfCues; i++)
			{
				m_vImageNames[i]     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), m_ui32NonCueSettingsCount + 2 * i);
				m_vStimulationsId[i] = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), m_ui32NonCueSettingsCount + 2 * i + 1);
			}

			//>>>> Initialisation
			m_oStimulationDecoder.initialize(*this,0);
			m_oStimulationEncoder.initialize(*this,0);

			//load the gtk builder interface
			m_pBuilderInterface=gtk_builder_new();
			if (!m_pBuilderInterface)
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Couldn't load the interface !";
				return false;
			}

			const CString l_sUIFile = OpenViBE::Directories::getDataDir() + "/plugins/simple-visualisation/openvibe-simple-visualisation-DisplayCueImage.ui";
			if (!gtk_builder_add_from_file(m_pBuilderInterface, l_sUIFile, NULL))
			{
				getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Could not load the .ui file " << l_sUIFile << "\n";
				return false;
			}

			gtk_builder_connect_signals(m_pBuilderInterface, NULL);

			m_pDrawingArea = GTK_WIDGET(gtk_builder_get_object(m_pBuilderInterface, "DisplayCueImageDrawingArea"));
			g_signal_connect(G_OBJECT(m_pDrawingArea), "expose_event", G_CALLBACK(DisplayCueImage_RedrawCallback), this);
			g_signal_connect(G_OBJECT(m_pDrawingArea), "size-allocate", G_CALLBACK(DisplayCueImage_SizeAllocateCallback), this);

			//set widget bg color
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oBackgroundColor);
			gtk_widget_modify_bg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oBackgroundColor);

			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_NORMAL, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_PRELIGHT, &m_oForegroundColor);
			gtk_widget_modify_fg(m_pDrawingArea, GTK_STATE_ACTIVE, &m_oForegroundColor);

			//Load the pictures:
			m_vOriginalPicture.resize(m_ui32NumberOfCues);
			m_vScaledPicture.resize(m_ui32NumberOfCues);

			for(uint32 i=0; i<m_ui32NumberOfCues; i++)
			{
				m_vOriginalPicture[i] = gdk_pixbuf_new_from_file_at_size(m_vImageNames[i], -1, -1, NULL);
				m_vScaledPicture[i]=NULL;
				if(!m_vOriginalPicture[i])
				{
					getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error << "Error couldn't load resource file : " << m_vImageNames[i] << "!\n";
					return false;
				}
			}
			m_vStimuliQueue.clear();

			m_pStimulusSender = TCPTagging::createStimulusSender();

			if (!m_pStimulusSender->connect("localhost", "15361"))
			{
				this->getLogManager() << LogLevel_Warning << "Unable to connect to AS's TCP Tagging plugin, stimuli wont be forwarded.\n";
			}

			if (m_bFullScreen)
			{
				GtkWidget *l_pWindow = gtk_widget_get_toplevel(m_pDrawingArea);
				gtk_window_fullscreen(GTK_WINDOW(l_pWindow));
				gtk_widget_show(l_pWindow);

				// @fixme small mem leak?
				GdkCursor* l_pCursor = gdk_cursor_new(GDK_BLANK_CURSOR);
				gdk_window_set_cursor(gtk_widget_get_window(l_pWindow), l_pCursor);
			}
			else
			{
				m_visualizationContext = dynamic_cast<OpenViBEVisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationContext));
				m_visualizationContext->setWidget(*this, m_pDrawingArea);
			}

			// Invalidate the drawing area in order to get the image resize already called at this point. The actual run will be smoother.
			if (GTK_WIDGET(m_pDrawingArea)->window)
			{
				gdk_window_invalidate_rect(GTK_WIDGET(m_pDrawingArea)->window, NULL, true);
			}

			return true;
		}

		bool CDisplayCueImage::uninitialize()
		{
			// Remove the possibly dangling idle loop. 
			if (m_uiIdleFuncTag)
			{
				g_source_remove(m_uiIdleFuncTag);
				m_uiIdleFuncTag = 0;
			}

			m_oStimulationDecoder.uninitialize();
			m_oStimulationEncoder.uninitialize();

			if (m_pStimulusSender)
			{
				delete m_pStimulusSender;
				m_pStimulusSender = NULL;
			}

			// Close the full screen
			if (m_bFullScreen)
			{
				GtkWidget *l_pWindow = gtk_widget_get_toplevel(m_pDrawingArea);
				gtk_window_unfullscreen(GTK_WINDOW(l_pWindow));
				gtk_widget_destroy(l_pWindow);
			}

			//destroy drawing area
			if(m_pDrawingArea)
			{
				gtk_widget_destroy(m_pDrawingArea);
				m_pDrawingArea = NULL;
			}

			// unref the xml file as it's not needed anymore
			if(m_pBuilderInterface) 
			{
				g_object_unref(G_OBJECT(m_pBuilderInterface));
				m_pBuilderInterface=NULL;
			}

			m_vStimulationsId.clear();
			m_vImageNames.clear();

			if(m_vOriginalPicture.size()>0) 
			{
				for(uint32 i=0; i<m_ui32NumberOfCues; i++)
				{
					if(m_vOriginalPicture[i]){ g_object_unref(G_OBJECT(m_vOriginalPicture[i])); }
				}
				m_vOriginalPicture.clear();
			}

			if (m_vScaledPicture.size()>0)
			{
				for(uint32 i=0; i<m_ui32NumberOfCues; i++)
				{
					if(m_vScaledPicture[i]){ g_object_unref(G_OBJECT(m_vScaledPicture[i])); }
				}
				m_vScaledPicture.clear();
			}

			this->releasePluginObject(m_visualizationContext);

			return true;
		}

		bool CDisplayCueImage::processClock(CMessageClock& rMessageClock)
		{
			IBoxIO* l_pBoxIO=getBoxAlgorithmContext()->getDynamicBoxContext();
			m_oStimulationEncoder.getInputStimulationSet()->clear();

			if (this->getPlayerContext().getCurrentTime() == 0)
			{
				// Always send header first
				m_oStimulationEncoder.encodeHeader();
				l_pBoxIO->markOutputAsReadyToSend(0, 0, 0);
			}

			if(m_bImageDrawn)
			{
				// this is first redraw() for that image or clear screen
				// we send a stimulation to signal it. 
				// @note this practice is deprecated, the TCP Tagging should be used. We pass the stimulus here for compatibility.

				if (m_int32DrawnImageID>=0)
				{
					// it was a image
					m_oStimulationEncoder.getInputStimulationSet()->appendStimulation(
								m_vStimulationsId[m_int32DrawnImageID],
								this->getPlayerContext().getCurrentTime(),
								0);
				}
				else
				{
					// it was a clear_screen
					m_oStimulationEncoder.getInputStimulationSet()->appendStimulation(
								m_ui64ClearScreenStimulation,
								this->getPlayerContext().getCurrentTime(),
								0);
				}

				m_bImageDrawn = false;

				if (m_int32DrawnImageID != m_int32RequestedImageID)
				{
					// We must be late...
					getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning << "One image may have been skipped => we must be late...\n";
				}

			}

			m_oStimulationEncoder.encodeBuffer();
			l_pBoxIO->markOutputAsReadyToSend(0, m_ui64LastOutputChunkDate, this->getPlayerContext().getCurrentTime());
			m_ui64LastOutputChunkDate = this->getPlayerContext().getCurrentTime();

			bool l_bStimulusMatchedBefore = false;

			// We check if some images must be displayed
			for(uint32 stim = 0; stim < m_oPendingStimulationSet.getStimulationCount() ; )
			{
				const uint64 l_ui64StimDate = m_oPendingStimulationSet.getStimulationDate(stim);
				const uint64 l_ui64Time = this->getPlayerContext().getCurrentTime();
				if (l_ui64StimDate < l_ui64Time)
				{
					const uint64 l_ui64StimID = m_oPendingStimulationSet.getStimulationIdentifier(stim);
					bool l_bStimulusMatchedNow = false;
					if(l_ui64StimID == m_ui64ClearScreenStimulation)
					{
						if (m_bImageRequested)
							getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "Clear screen was received before previous cue image in slot " << m_int32RequestedImageID+1  << " was displayed!!\n";
						m_bImageRequested = true;
						l_bStimulusMatchedBefore = true;
						l_bStimulusMatchedNow = true;
						m_int32RequestedImageID = -1;
					}
					else
					{
						for(uint32 i=0; i < m_ui32NumberOfCues; i++)
						{
							if(l_ui64StimID == m_vStimulationsId[i])
							{
								if (m_bImageRequested)
									getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning 
										<< "Previous request of slot " << m_int32RequestedImageID+1 << " image was replaced by request for slot " << i+1 << " => Not enough time between two images!!\n";
								m_bImageRequested = true;
								l_bStimulusMatchedBefore = true;
								l_bStimulusMatchedNow = true;
								m_int32RequestedImageID = i;
								break;
							}
						}
					}

					if (l_bStimulusMatchedNow)
					{
						// Queue the recognized stimulation to TCP Tagging to be sent after rendering
						const uint64 l_ui64SentStimulation = (m_int32RequestedImageID >= 0 ? m_vStimulationsId[m_int32RequestedImageID] : m_ui64ClearScreenStimulation);
						m_vStimuliQueue.push_back(l_ui64SentStimulation);
					}
					else
					{
						// Pass unrecognized stimulations to TCP Tagging. Be careful when modifying the code that the
						// stimuli received by AS keep their original time order despite the delays introduced to rendered stimuli.

						if (l_bStimulusMatchedBefore)
						{
							// We have queued a cue to be drawn, so we should delay this stimulation to TCP Tagging to be processed after the cue rendering to keep the time order
							m_vStimuliQueue.push_back(l_ui64StimID);
						}
						else
						{
							// We have not yet queued anything to be drawn, so we can forward immediately.
							m_pStimulusSender->sendStimulation(l_ui64StimID);
						}
					}

					const float64 l_f64Delay = ITimeArithmetics::timeToSeconds(l_ui64Time - l_ui64StimDate) * 1000; // delay in ms
					if (l_f64Delay>50)
						getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning << "Stimulation " << l_ui64StimID << " was late in processClock() : "<< l_f64Delay <<" ms \n";

					m_oPendingStimulationSet.removeStimulation(stim);
				}
				else
				{
					// Stim is still in the future, skip it for now
					stim++;
				}
			}

			// We should show the cue image now. How this works:
			// - The gtk drawing area is invalidated
			// - Gtk will request a redraw
			// - The redraw puts in instructions to render the new image
			// - The corresponding stimulation is buffered to TCP Tagging 
			// - Callback to flush the TCP Tagging buffer is registered to be run by gtk once after the rendering
			// - Gtk renders
			// - Callback to flush the TCP Tagging buffer is called by gtk
			if (m_bImageRequested && GTK_WIDGET(m_pDrawingArea)->window)
			{
				// this will trigger the callback redraw(). Since the draw will happen after the exit from this function, no point calling this in the loop above
				gdk_window_invalidate_rect(GTK_WIDGET(m_pDrawingArea)->window, NULL, true);
			}



			return true;
		}

		bool CDisplayCueImage::processInput(uint32 ui32InputIndex)
		{
			getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
			return true;
		}

		bool CDisplayCueImage::process()
		{
			IBoxIO* l_pBoxIO=getBoxAlgorithmContext()->getDynamicBoxContext();

			// We decode and save the received stimulations.
			for(uint32 input=0; input < getBoxAlgorithmContext()->getStaticBoxContext()->getInputCount(); input++)
			{
				for(uint32 chunk=0; chunk < l_pBoxIO->getInputChunkCount(input); chunk++)
				{
					m_oStimulationDecoder.decode(chunk,true);
					if(m_oStimulationDecoder.isHeaderReceived())
					{
						// nop
					}
					if(m_oStimulationDecoder.isBufferReceived())
					{
						for (uint32 stim = 0; stim < m_oStimulationDecoder.getOutputStimulationSet()->getStimulationCount(); stim++)
						{
							// We always add the stimulations to the set to allow passing them to TCP Tagging in order in processClock()
							const uint64 l_ui64StimID = m_oStimulationDecoder.getOutputStimulationSet()->getStimulationIdentifier(stim);
							const uint64 l_ui64StimDate = m_oStimulationDecoder.getOutputStimulationSet()->getStimulationDate(stim);
							const uint64 l_ui64StimDuration = m_oStimulationDecoder.getOutputStimulationSet()->getStimulationDuration(stim);

							const uint64 l_ui64Time = this->getPlayerContext().getCurrentTime();
							if (l_ui64StimDate < l_ui64Time)
							{
								const float64 l_f64Delay = ITimeArithmetics::timeToSeconds(l_ui64Time - l_ui64StimDate) * 1000; //delay in ms
								if (l_f64Delay>50)
									getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning << "Stimulation " << l_ui64StimID << " was received late: " << l_f64Delay << " ms \n";
							}

							if (l_ui64StimDate < l_pBoxIO->getInputChunkStartTime(input, chunk))
							{
								this->getLogManager() << LogLevel_ImportantWarning << "Input Stimulation Date before beginning of the buffer\n";
							}

							m_oPendingStimulationSet.appendStimulation(
								l_ui64StimID,
								l_ui64StimDate,
								l_ui64StimDuration);
						}
					}
				}
			}

			return true;
		}

		//Callback called by GTK
		void CDisplayCueImage::redraw()
		{
			if (m_int32RequestedImageID >= 0)
			{
				drawCuePicture(m_int32RequestedImageID);
			}
			if(m_bImageRequested)
			{
				m_bImageRequested = false;
				m_bImageDrawn = true;
				m_int32DrawnImageID = m_int32RequestedImageID;

				// Set the handler to push out the queued stims after the actual rendering
				if (m_uiIdleFuncTag == 0)
				{
					m_uiIdleFuncTag = g_idle_add(DisplayCueImage_flush_callback, this);
				}
			}
		}

		void CDisplayCueImage::drawCuePicture(OpenViBE::uint32 uint32CueID)
		{
			const gint l_iWindowWidth = m_pDrawingArea->allocation.width;
			const gint l_iWindowHeight = m_pDrawingArea->allocation.height;

			// Center image
			const gint l_iX = (l_iWindowWidth/2) - gdk_pixbuf_get_width(m_vScaledPicture[uint32CueID])/2;
			const gint l_iY = (l_iWindowHeight/2) - gdk_pixbuf_get_height(m_vScaledPicture[uint32CueID])/2;;
			gdk_draw_pixbuf(m_pDrawingArea->window, NULL, m_vScaledPicture[uint32CueID], 0, 0, l_iX, l_iY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
		}

		void CDisplayCueImage::resize(uint32 ui32Width, uint32 ui32Height)
		{
			for(uint32 i=0; i<m_vScaledPicture.size(); i++)
			{
				if(m_vScaledPicture[i]){ g_object_unref(G_OBJECT(m_vScaledPicture[i])); }
			}

			if (!m_bScaleImages)
			{
				for (uint32 i = 0; i < m_vScaledPicture.size(); i++)
				{
					m_vScaledPicture[i] = gdk_pixbuf_copy(m_vOriginalPicture[i]);
				}
				return;
			}

			// Scale
			if(m_bFullScreen)
			{
				for (uint32 i = 0; i < m_vScaledPicture.size(); i++)
				{
					// Keep aspect ratio when scaling
					const int l_iWidth = gdk_pixbuf_get_width(m_vOriginalPicture[i]);
					const int l_iHeight = gdk_pixbuf_get_height(m_vOriginalPicture[i]);

					const float64 l_iWidthScale = ui32Width / static_cast<float64>(l_iWidth);
					const float64 l_iHeightScale = ui32Height / static_cast<float64>(l_iHeight);

					const float64 l_fMinScale = std::min<float64>(l_iWidthScale, l_iHeightScale);

					const int l_iNewWidth = static_cast<int>(l_fMinScale * l_iWidth);
					const int l_iNewHeight = static_cast<int>(l_fMinScale * l_iHeight);

					// printf("Old aspect %f new aspect %f\n", l_iWidth / (float)l_iHeight, l_iNewWidth / (float)l_iNewHeight);
					m_vScaledPicture[i] = gdk_pixbuf_scale_simple(m_vOriginalPicture[i], l_iNewWidth, l_iNewHeight, GDK_INTERP_BILINEAR);
				}
			}
			else
			{
				const float l_fX = (float)(ui32Width<64?64:ui32Width);
				const float l_fY = (float)(ui32Height<64?64:ui32Height);
				for(uint32 i=0; i<m_vScaledPicture.size(); i++)
				{
					float l_fx = (float)gdk_pixbuf_get_width(m_vOriginalPicture[i]);
					float l_fy = (float)gdk_pixbuf_get_height(m_vOriginalPicture[i]);
					if((l_fX/l_fx) < (l_fY/l_fy))
					{
						l_fy = l_fX*l_fy/(3*l_fx);
						l_fx = l_fX/3;
					}
					else
					{
						l_fx = l_fY*l_fx/(3*l_fy);
						l_fy = l_fY/3;
					}
					m_vScaledPicture[i] = gdk_pixbuf_scale_simple(m_vOriginalPicture[i], (int)l_fx, (int)l_fy, GDK_INTERP_BILINEAR);
				}
			}
		}

		// Note that we don't need concurrency control here as gtk callbacks run in the main thread
		void CDisplayCueImage::flushQueue(void)
		{
			for(size_t i=0;i<m_vStimuliQueue.size();i++)
			{
				m_pStimulusSender->sendStimulation(m_vStimuliQueue[i]);
			}
			m_vStimuliQueue.clear();

			// This function will be automatically removed after completion, so set to 0
			m_uiIdleFuncTag = 0;
		}

	};
};
