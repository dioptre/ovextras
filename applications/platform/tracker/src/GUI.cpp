//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

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
	auto MEMBERFUN = [](::GtkMenuItem* pMenuItem, gpointer pUserData) { static_cast<GUI*>(pUserData)->MEMBERFUN(); }; \
	g_signal_connect(G_OBJECT(gtk_builder_get_object(m_pInterface, MENUCHOICE)),  \
		ACTION, G_CALLBACK( (void (*)( ::GtkMenuItem* pMenuItem, gpointer pUserData)) MEMBERFUN ), this); 

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

	char basename[512]; 

	GtkEntry *tmp = GTK_ENTRY(gtk_builder_get_object(m_pInterface, "tracker-data-file"));
	FS::Files::getFilename(m_rTracker.getWorkspace().getTrackFile(), basename, 512);
	gtk_entry_set_text(tmp, basename);

	tmp =  GTK_ENTRY(gtk_builder_get_object(m_pInterface, "tracker-sink-file"));
	FS::Files::getFilename(m_rTracker.getWorkspace().getSinkFile(), basename, 512);
	gtk_entry_set_text(tmp, basename);

	gtk_builder_connect_signals(m_pInterface, NULL);

	g_idle_add(fIdleApplicationLoop, this);

	::GtkWidget* l_pTrackWindow=GTK_WIDGET(gtk_builder_get_object(m_pInterface, "tracker-scenario_drawing_area"));

	//@todo refactor the whole thing that follows, here just to test the renderer
	m_Renderer = new TrackRenderer();
	m_Renderer->initialize(l_pTrackWindow);

	gtk_widget_show(l_pMainWindow);

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

	redrawTrack();

	return true;
}

bool GUI::redrawTrack(void)
{
	uint64_t time = System::Time::zgetTime();

	m_Renderer->m_pRenderer.m_pRenderer->clear(0);

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
		uint64_t numChunks = std::min<uint64>(100,(uint64_t)ptr->getChunkCount());

		for(uint32_t i=0;i<numChunks;i++)
		{
			TypeSignal::Buffer* buf;

			ptr->getChunk(i, &buf);

			m_Renderer->push(buf->m_buffer);
		}
	}

	m_Renderer->m_pRenderer.m_pRendererContext->setScale(1);
	m_Renderer->m_pRenderer.m_pRenderer->refresh(*m_Renderer->m_pRenderer.m_pRendererContext);
	m_Renderer->redraw(true);

	std::cout << "Track redraw took " << ITimeArithmetics::timeToSeconds(System::Time::zgetTime() - time)*1000 << "ms\n";

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

