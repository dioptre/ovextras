//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 
// @todo connect all the vertical and horizontal sliders and scales
// @todo make horizontal scale related to seconds rather than chunks
// @todo add rulers
// @todo need a much more clear design to handle multiple tracks with multiple streams of different sizes

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include <fs/Files.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <string.h> // strchr on Ubuntu

#include "GUI.h"

#include "TrackRenderer.h"

#include "TypeSignal.h"
#include "Stream.h."

#include <system/ovCMath.h>
#include <algorithm>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;


#define GTK_CALLBACK_MAPPER(MENUCHOICE, ACTION, MEMBERFUN) \
	auto MEMBERFUN = [](::GtkWidget* pMenuItem, gpointer pUserData) { static_cast<GUI*>(pUserData)->MEMBERFUN(); }; \
	g_signal_connect(G_OBJECT(gtk_builder_get_object(m_pInterface, MENUCHOICE)),  \
		ACTION, G_CALLBACK( (void (*)( ::GtkWidget* pMenuItem, gpointer pUserData)) MEMBERFUN ), this); 
#define GTK_CALLBACK_MAPPER_PARAM(MENUCHOICE, ACTION, MEMBERFUN) \
	auto MEMBERFUN = [](::GtkWidget* pMenuItem, gpointer pUserData) { static_cast<GUI*>(pUserData)->MEMBERFUN(pMenuItem); }; \
	g_signal_connect(G_OBJECT(gtk_builder_get_object(m_pInterface, MENUCHOICE)),  \
		ACTION, G_CALLBACK( (void (*)( ::GtkWidget* pMenuItem, gpointer pUserData)) MEMBERFUN ), this); 

gboolean fIdleApplicationLoop(gpointer pUserData)
{
	GUI *gui = static_cast<GUI*>(pUserData);

	gui->step();

	return TRUE;
}

GUI::~GUI() 
{ 	
	if(m_Renderer)
	{
		m_Renderer->uninitialize();
		delete m_Renderer;
	}
}

bool GUI::initGUI(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	m_pInterface=gtk_builder_new(); 
	const OpenViBE::CString l_sFilename = OpenViBE::Directories::getDataDir() + "/applications/tracker/tracker.ui";
	if(!gtk_builder_add_from_file(m_pInterface, l_sFilename, NULL)) {
		std::cout << "Problem loading [" << l_sFilename << "]\n";
		return false;
	}

	::GtkWidget* l_pMainWindow=GTK_WIDGET(gtk_builder_get_object(m_pInterface, "tracker"));
	g_signal_connect(G_OBJECT(l_pMainWindow), "destroy", gtk_main_quit, NULL);

	GTK_CALLBACK_MAPPER("tracker-menu_open_file", "activate", openFileCB);
	GTK_CALLBACK_MAPPER("tracker-menu_open_sink", "activate", openSinkCB);
	GTK_CALLBACK_MAPPER("tracker-menu_quit", "activate", quitCB);
	GTK_CALLBACK_MAPPER("tracker-button_play_pause", "clicked", playCB);
	GTK_CALLBACK_MAPPER("tracker-button_forward", "clicked", playFastCB);
	GTK_CALLBACK_MAPPER("tracker-button_stop", "clicked", stopCB);
	GTK_CALLBACK_MAPPER("tracker-sink-properties", "clicked", sinkPropertiesCB);	
	// GTK_CALLBACK_MAPPER_PARAM("adj-h-pos", "value-changed", hSliderCB);
	GTK_CALLBACK_MAPPER_PARAM("hscrollbar1", "value-changed", hSliderCB);
	GTK_CALLBACK_MAPPER_PARAM("hscale1", "value-changed", hScaleCB);

	char basename[512]; 

	GtkEntry *tmp = GTK_ENTRY(gtk_builder_get_object(m_pInterface, "tracker-data-file"));
	FS::Files::getFilename(m_rTracker.getWorkspace().getTrackFile(), basename, 512);
	gtk_entry_set_text(tmp, basename);

	tmp =  GTK_ENTRY(gtk_builder_get_object(m_pInterface, "tracker-sink-file"));
	FS::Files::getFilename(m_rTracker.getWorkspace().getSinkFile(), basename, 512);
	gtk_entry_set_text(tmp, basename);

	m_hScrollbar = GTK_WIDGET(gtk_builder_get_object(m_pInterface, "hscrollbar1"));
	m_hScale = GTK_WIDGET(gtk_builder_get_object(m_pInterface, "hscale1"));

	gtk_builder_connect_signals(m_pInterface, NULL);

	g_idle_add(fIdleApplicationLoop, this);

	::GtkWidget* l_pTrackWindow=GTK_WIDGET(gtk_builder_get_object(m_pInterface, "tracker-scenario_drawing_area"));

	//@todo refactor the whole thing that follows, here just to test the renderer
	m_Renderer = new TrackRenderer();
	m_Renderer->initialize(l_pTrackWindow);

	gtk_widget_show(l_pMainWindow);

	resetSliderLimits();
	redrawTrack();

	return true;
}

bool GUI::step(void)
{
	bool retVal = m_rTracker.step();

	if(retVal)
	{
		uint64_t currentTime = m_rTracker.getWorkspace().getCurrentTime();
		if(currentTime - m_PreviousTime > ITimeArithmetics::secondsToTime(1.0))
		{
			GtkLabel* label = GTK_LABEL(gtk_builder_get_object(m_pInterface, "tracker-label_current_time"));

			char tmp[128];
			sprintf(tmp, "Time : %fs", ITimeArithmetics::timeToSeconds(currentTime));
		
			gtk_label_set(label, tmp);

			m_PreviousTime = currentTime;
		}
	} 
	else
	{
		GtkLabel* label = GTK_LABEL(gtk_builder_get_object(m_pInterface, "tracker-label_current_time"));	
		gtk_label_set(label, "Time : File ended");
	}

	return retVal;

}

bool GUI::run(void)
{
	gtk_main();

	return true;
}

bool GUI::openFileCB(void)
{
// 	m_KernelContext.getLogManager() << LogLevel_Debug << "openScenarioCB\n";

	::GtkFileFilter* l_pFileFilterSpecific=gtk_file_filter_new();

	gtk_file_filter_add_pattern(l_pFileFilterSpecific, "*.ov");
	gtk_file_filter_set_name(l_pFileFilterSpecific, "OV files");

	::GtkWidget* l_pWidgetDialogOpen=gtk_file_chooser_dialog_new(
			"Select file to open...",
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(l_pWidgetDialogOpen), l_pFileFilterSpecific);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(l_pWidgetDialogOpen), l_pFileFilterSpecific);

	// GTK 2 known bug: this won't work if  setCurrentFolder is also used on the dialog.
	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(l_pWidgetDialogOpen),true);

	const CString defaultPath = OpenViBE::Directories::getDataDir() + CString("/scenarios/signals/");

	gtk_file_chooser_set_current_folder(
			GTK_FILE_CHOOSER(l_pWidgetDialogOpen),
			defaultPath.toASCIIString());

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(l_pWidgetDialogOpen),false);

	if(gtk_dialog_run(GTK_DIALOG(l_pWidgetDialogOpen))==GTK_RESPONSE_ACCEPT)
	{
		//char* l_sFileName=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen));
		GSList * l_pFile, *l_pList;
		l_pFile = l_pList = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(l_pWidgetDialogOpen));
		while(l_pFile)
		{
			char* l_sFileName = (char*)l_pFile->data;
			char* l_pBackslash = NULL;
			while((l_pBackslash = ::strchr(l_sFileName, '\\'))!=NULL)
			{
				*l_pBackslash = '/';
			}
			m_rTracker.getWorkspace().setTrack(l_sFileName);

			GtkEntry *tmp = GTK_ENTRY(gtk_builder_get_object(m_pInterface, "tracker-data-file"));
			char basename[512]; 
			FS::Files::getFilename(l_sFileName, basename, 512);
			gtk_entry_set_text(tmp, basename);

			g_free(l_pFile->data);
			l_pFile=l_pFile->next;
		}
		g_slist_free(l_pList);
	}
	gtk_widget_destroy(l_pWidgetDialogOpen);

	resetSliderLimits();
	redrawTrack();

	return true;
}

bool GUI::resetSliderLimits(void)
{

	m_totalChunks = 0;
	m_numChannels = 0;
	m_chunkSize = 0;

	const Track& tr = m_rTracker.getWorkspace().getTrack();
	const Stream<TypeSignal>* ptr = nullptr;

	for(size_t i=0;i<tr.getNumStreams();i++)
	{
		if(tr.getStream(i) && tr.getStream(i)->getTypeIdentifier()==OV_TypeId_Signal)
		{
			ptr = reinterpret_cast< const Stream<TypeSignal>* >(tr.getStream(i));
			break;
		}
	}
	if(ptr)
	{
		// Count samples first
		TypeSignal::Buffer* buf;
		ptr->getChunk(0, &buf);
		if(buf && buf->m_buffer.getDimensionCount()==2) 
		{
			m_totalChunks = ptr->getChunkCount();
			m_numChannels = buf->m_buffer.getDimensionSize(0);
			m_chunkSize = buf->m_buffer.getDimensionSize(1);
		}
	}

	if(m_totalChunks<2) { std::cout << "Warning: File has less than 2 chunks, ranges may behave oddly\n"; }

	const uint32_t chunksPerView = std::min<uint32_t>(20,m_totalChunks-1);

	gtk_range_set_range(GTK_RANGE(m_hScale), 1, m_totalChunks-1);
	gtk_range_set_value(GTK_RANGE(m_hScale), chunksPerView);

	gtk_range_set_range(GTK_RANGE(m_hScrollbar), 0, m_totalChunks-chunksPerView);
	gtk_range_set_value(GTK_RANGE(m_hScrollbar), 0);

	// gtk_range_set_value(GTK_RANGE(m_hScrollbar), 0);
	// gtk_range_set_round_digits(GTK_RANGE(m_hScrollbar), 0); //not in gtk ver we use on Windows
	gtk_range_set_increments(GTK_RANGE(m_hScrollbar), chunksPerView, 10*chunksPerView);

	return true;

}

bool GUI::redrawTrack(void)
{
	uint64_t time = System::Time::zgetTime();

	const Track& tr = m_rTracker.getWorkspace().getTrack();
	const Stream<TypeSignal>* ptr = nullptr;

	for(size_t i=0;i<tr.getNumStreams();i++)
	{
		if(tr.getStream(i) && tr.getStream(i)->getTypeIdentifier()==OV_TypeId_Signal)
		{
			ptr = reinterpret_cast< const Stream<TypeSignal>* >(tr.getStream(i));
			break;
		}
	}
	if(ptr)
	{
		const uint32_t firstChunk = static_cast<uint32_t>( gtk_range_get_value(GTK_RANGE(m_hScrollbar)));
		const uint32_t chunksPerView = static_cast<uint32_t>( gtk_range_get_value(GTK_RANGE(m_hScale)));
		const uint32_t lastChunk = std::min<uint32_t>(firstChunk+chunksPerView, m_totalChunks);
		const uint32_t chunksInRange = lastChunk-firstChunk;

		m_Renderer->reset(m_numChannels, chunksInRange*m_chunkSize);

		// Push all chunks to renderer
		for(uint32_t i=firstChunk;i<lastChunk;i++)
		{
			TypeSignal::Buffer* buf;

			ptr->getChunk(i, &buf);

			m_Renderer->push(buf->m_buffer);
		}
	}

	m_Renderer->m_pRenderer.m_pRendererContext->setScale(0.5);
	m_Renderer->m_pRenderer.m_pRenderer->refresh(*m_Renderer->m_pRenderer.m_pRendererContext);
	m_Renderer->redraw(true);

	// std::cout << "Track redraw took " << ITimeArithmetics::timeToSeconds(System::Time::zgetTime() - time)*1000 << "ms\n";

	return true;
}

bool GUI::openSinkCB(void)
{
// 	m_KernelContext.getLogManager() << LogLevel_Debug << "openScenarioCB\n";

	::GtkFileFilter* l_pFileFilterSpecific=gtk_file_filter_new();

	gtk_file_filter_add_pattern(l_pFileFilterSpecific, "*.xml");
	gtk_file_filter_set_name(l_pFileFilterSpecific, "Scenario files");

	::GtkWidget* l_pWidgetDialogOpen=gtk_file_chooser_dialog_new(
			"Select file to open...",
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(l_pWidgetDialogOpen), l_pFileFilterSpecific);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(l_pWidgetDialogOpen), l_pFileFilterSpecific);

	// GTK 2 known bug: this won't work if  setCurrentFolder is also used on the dialog.
	gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(l_pWidgetDialogOpen),true);

	const CString defaultPath = OpenViBE::Directories::getDataDir() + CString("/applications/tracker/");

	gtk_file_chooser_set_current_folder(
			GTK_FILE_CHOOSER(l_pWidgetDialogOpen),
			defaultPath.toASCIIString());

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(l_pWidgetDialogOpen),false);

	if(gtk_dialog_run(GTK_DIALOG(l_pWidgetDialogOpen))==GTK_RESPONSE_ACCEPT)
	{
		//char* l_sFileName=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen));
		GSList * l_pFile, *l_pList;
		l_pFile = l_pList = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(l_pWidgetDialogOpen));
		while(l_pFile)
		{
			char* l_sFileName = (char*)l_pFile->data;
			char* l_pBackslash = NULL;
			while((l_pBackslash = ::strchr(l_sFileName, '\\'))!=NULL)
			{
				*l_pBackslash = '/';
			}
			m_rTracker.getWorkspace().setSink(l_sFileName);

			GtkEntry *tmp = GTK_ENTRY(gtk_builder_get_object(m_pInterface, "tracker-sink-file"));
			char basename[512]; 
			FS::Files::getFilename(l_sFileName, basename, 512);
			gtk_entry_set_text(tmp, basename);

			g_free(l_pFile->data);
			l_pFile=l_pFile->next;
		}
		g_slist_free(l_pList);
	}
	gtk_widget_destroy(l_pWidgetDialogOpen);

	return true;
}


bool GUI::quitCB(void)
{
	gtk_main_quit();
	return true;
}

bool GUI::playCB(void)
{
	m_PreviousTime = 0;
	return m_rTracker.play(false);
}

bool GUI::playFastCB(void)
{
	return m_rTracker.play(true);
}


bool GUI::stopCB(void)
{
	GtkLabel* label = GTK_LABEL(gtk_builder_get_object(m_pInterface, "tracker-label_current_time"));	
	gtk_label_set(label, "Time : 0");

	return m_rTracker.stop();
}

bool GUI::sinkPropertiesCB(void)
{
	return m_rTracker.getWorkspace().configureSink();
}

bool GUI::hSliderCB(GtkWidget* widget)
{
//	std::cout << "Hslider " << gtk_range_get_value(GTK_RANGE(widget)) << "\n";

	redrawTrack();
	return true;
}

bool GUI::hScaleCB(GtkWidget* widget)
{
	// @since we store the pointers in the class, no need to pass in widget...
//	std::cout << "Hscale " << gtk_range_get_value(GTK_RANGE(widget)) << "\n";

	const uint32_t chunksPerView = static_cast<uint32_t>( gtk_range_get_value(GTK_RANGE(widget)));

	gtk_range_set_range(GTK_RANGE(m_hScrollbar), 0, m_totalChunks-chunksPerView);

	redrawTrack();
	return true;
}


#if 0 

int main(int argc, char *argv[])
{
	KernelWrapper kernelWrapper;

	if(!kernelWrapper.initialize())
	{
		return 1;
	}

	Workspace wp(*kernelWrapper.m_KernelContext);

// 	TestClass tmp(*kernelWrapper.m_KernelContext);

/*

	const CString eegFile = OpenViBE::Directories::getDataDir() + CString("/scenarios/signals/bci-motor-imagery.ov");
//	const CString eegFile = CString("E:/jl/noise-test.ov");
	const CString scenarioFile = OpenViBE::Directories::getDataDir() + CString("/applications/tracker/tracker-debug-display.xml");
	
	if(!wp.setTrack(eegFile.toASCIIString()))
	{
		return 2;
	}
	if(!wp.setSink(scenarioFile.toASCIIString()))
	{
		return 3;
	}

	// Push some chunks to selection
	Selection& selection = wp.m_track.m_Selection;
	selection.addRange(Range(3,5));
	selection.addRange(Range(9,11));

	if(!wp.play())
	{
		return 4;
	}
*/
	
	initGUI(argc, argv);

	return 0;
}

#endif

