#include "ovdCBoxConfigurationDialog.h"
#include "settings/ovdCSettingViewFactory.h"
#include "settings/ovdCAbstractSettingView.h"

#include <string>
#include <fstream>
#include <iostream>
#include <cstring>

#include <xml/IReader.h>
#include <xml/IWriter.h>
#include <xml/IXMLHandler.h>
#include <xml/IXMLNode.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEDesigner;
using namespace std;

// #define OV_DEBUG_BOX_DIALOG
#ifdef OV_DEBUG_BOX_DIALOG
#define DEBUG_PRINT(x) x
#else
#define DEBUG_PRINT(x)
#endif

namespace
{
	const char * const c_sRootName = "OpenViBE-SettingsOverride";
	const char *const c_sSettingName = "SettingValue";
}

// This callback is used to gray out the settings if file override is selected
static void on_file_override_check_toggled(::GtkToggleButton* pToggleButton, gpointer pUserData)
{
	const gboolean l_oPreviousValue = gtk_toggle_button_get_active(pToggleButton);
	gtk_widget_set_sensitive((::GtkWidget*)pUserData, !l_oPreviousValue);
}

static void on_button_load_clicked(::GtkButton* pButton, gpointer pUserData)
{
	static_cast<CBoxConfigurationDialog *>(pUserData)->loadConfiguration();
}

static void on_button_save_clicked(::GtkButton* pButton, gpointer pUserData)
{
	static_cast<CBoxConfigurationDialog *>(pUserData)->saveConfiguration();
}

static void on_override_browse_clicked(::GtkButton* pButton, gpointer pUserData)
{
	static_cast<CBoxConfigurationDialog *>(pUserData)->onOverrideBrowse();
}

static void collect_widget_cb(::GtkWidget* pWidget, gpointer pUserData)
{
	static_cast< std::vector< ::GtkWidget* > *>(pUserData)->push_back(pWidget);
}
// ----------- ----------- ----------- ----------- ----------- ----------- ----------- ----------- ----------- -----------

CBoxConfigurationDialog::CBoxConfigurationDialog(const IKernelContext& rKernelContext, IBox& rBox, const char* sGUIFilename, const char* sGUISettingsFilename, bool bMode)
	:m_rKernelContext(rKernelContext)
	,m_rBox(rBox)
	,m_sGUIFilename(sGUIFilename)
	,m_sGUISettingsFilename(sGUISettingsFilename)
	,m_pWidget(NULL)
	,m_pWidgetToReturn(NULL)
	,m_bIsScenarioRunning(bMode)
	,m_oSettingFactory(m_sGUISettingsFilename.toASCIIString(), rKernelContext)
	,m_pFileOverrideCheck(NULL)
{
	if(m_rBox.getSettingCount())
	{
		::GtkBuilder* l_pBuilderInterfaceSetting=gtk_builder_new(); // glade_xml_new(m_sGUIFilename.toASCIIString(), "box_configuration", NULL);
		gtk_builder_add_from_file(l_pBuilderInterfaceSetting, m_sGUIFilename.toASCIIString(), NULL);
		gtk_builder_connect_signals(l_pBuilderInterfaceSetting, NULL);


#if 1 // this approach fails to set a modal dialog


		m_pWidget=GTK_WIDGET(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration"));//renamed
		m_pWidgetToReturn = GTK_WIDGET(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration-table"));//renamed
		char l_sTitle[1024];
		sprintf(l_sTitle, "Configure %s settings", m_rBox.getName().toASCIIString());
		gtk_window_set_title(GTK_WINDOW(m_pWidget), l_sTitle);
#else
		::GtkWidget *l_pSettingDialog = gtk_dialog_new_with_buttons(
			"Configure Box Settings",
			&m_rMainWindow, //set dialog transient for main window
			GTK_DIALOG_MODAL,
			"Revert", 0, "Apply", GTK_RESPONSE_APPLY, "Cancel", GTK_RESPONSE_CANCEL, NULL); //set up action buttons

		//unparent contents from builder interface
		::GtkWidget* l_pContents=GTK_WIDGET(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration-table"));
		gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(l_pContents)), l_pContents);

		//add contents to dialog
		::GtkWidget* l_pContentsArea=GTK_DIALOG(l_pSettingDialog)->vbox; //gtk_dialog_get_content_area() not available in current Gtk distribution
		gtk_container_add(GTK_CONTAINER(l_pContentsArea), l_pContents);

		//action buttons can't be unparented from builder interface and added to dialog, which is why they are added at dialog creation time
#endif
		m_pScrolledWindow=GTK_SCROLLED_WINDOW(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration-scrolledwindow"));
		m_pViewPort=GTK_VIEWPORT(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration-viewport"));
		m_pSettingsTable=GTK_TABLE(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration-table"));
		::GtkContainer* l_pFileOverrideContainer=GTK_CONTAINER(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration-hbox_filename_override"));
		::GtkButton* l_pButtonLoad=GTK_BUTTON(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration-button_load_current_from_file"));
		::GtkButton* l_pButtonSave=GTK_BUTTON(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration-button_save_current_to_file"));
		m_pFileOverrideCheck=GTK_CHECK_BUTTON(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration-checkbutton_filename_override"));

		generateSettingsTable();
		updateSize();

		// If a scenario is not running, we add the buttons to save/load etc
		if (!m_bIsScenarioRunning)
		{
			::GtkBuilder* l_pBuilderInterfaceSettingCollection=gtk_builder_new(); // glade_xml_new(m_sGUIFilename.toASCIIString(), l_sSettingOverrideWidgetName.c_str(), NULL);
			gtk_builder_add_from_file(l_pBuilderInterfaceSettingCollection, m_sGUISettingsFilename.toASCIIString(), NULL);
			gtk_builder_connect_signals(l_pBuilderInterfaceSettingCollection, NULL);

			::GtkWidget* l_pSettingOverrideValue = GTK_WIDGET(gtk_builder_get_object(l_pBuilderInterfaceSettingCollection, "settings_collection-hbox_setting_filename"));

			std::vector < ::GtkWidget* > l_vWidget;
			gtk_container_foreach(GTK_CONTAINER(l_pSettingOverrideValue), collect_widget_cb, &l_vWidget);
			m_pOverrideEntry = GTK_ENTRY(l_vWidget[0]);

			gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(l_pSettingOverrideValue)), l_pSettingOverrideValue);
			gtk_container_add(l_pFileOverrideContainer, l_pSettingOverrideValue);

			g_signal_connect(G_OBJECT(m_pFileOverrideCheck), "toggled", G_CALLBACK(on_file_override_check_toggled), GTK_WIDGET(m_pSettingsTable));
			g_signal_connect(G_OBJECT(l_vWidget[1]),         "clicked", G_CALLBACK(on_override_browse_clicked), this);
			g_signal_connect(G_OBJECT(l_pButtonLoad),        "clicked", G_CALLBACK(on_button_load_clicked), this);
			g_signal_connect(G_OBJECT(l_pButtonSave),        "clicked", G_CALLBACK(on_button_save_clicked), this);

			if(m_rBox.hasAttribute(OV_AttributeId_Box_SettingOverrideFilename))
			{
				::GtkExpander *l_pExpander = GTK_EXPANDER(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration-expander"));
				gtk_expander_set_expanded(l_pExpander, true);

				gtk_entry_set_text( m_pOverrideEntry, m_rBox.getAttributeValue(OV_AttributeId_Box_SettingOverrideFilename).toASCIIString());
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_pFileOverrideCheck), true);
				gtk_widget_set_sensitive(GTK_WIDGET(m_pSettingsTable), false);
			}
			else
			{
				gtk_entry_set_text( m_pOverrideEntry, "");
			}

			g_object_unref(l_pBuilderInterfaceSetting);
			g_object_unref(l_pBuilderInterfaceSettingCollection);
		}

		//if we are in a running scenario, we just need the WidgetToReturn and can destroy the rest
		if (m_bIsScenarioRunning)
		{
			//unparent widget
			::GtkWidget* l_pWidgetParent = gtk_widget_get_parent(m_pWidgetToReturn);
			if(GTK_IS_CONTAINER(l_pWidgetParent))
			{
				g_object_ref(m_pWidgetToReturn);
				gtk_container_remove(GTK_CONTAINER(l_pWidgetParent), m_pWidgetToReturn);
				gtk_widget_destroy(m_pWidget);
				m_pWidget = NULL;
			}
		}

	}
}

CBoxConfigurationDialog::~CBoxConfigurationDialog(void)
{
	if (m_pWidgetToReturn) 
	{
		gtk_widget_destroy(m_pWidgetToReturn);
		m_pWidgetToReturn=NULL;
	}
	if(m_pWidget) {
		gtk_widget_destroy(m_pWidget);
		m_pWidget = NULL;
	}

	m_rBox.deleteObserver(this);
}

boolean CBoxConfigurationDialog::run(bool bMode)
{
	DEBUG_PRINT(cout << "run dialog\n";)


	boolean l_bModified=false;
	if(m_pWidget==NULL)
	{
		//m_pWidget NULL means no widget (probably a box with no settings) so no need to run, quit now
		return l_bModified;//return false is default case
	}

	m_rBox.addObserver(this);
	m_rBox.storeState();
	::GtkBuilder* l_pBuilderInterfaceSetting=gtk_builder_new(); // glade_xml_new(m_sGUIFilename.toASCIIString(), "box_configuration", NULL);
	gtk_builder_add_from_file(l_pBuilderInterfaceSetting, m_sGUIFilename.toASCIIString(), NULL);

	::GtkTable* l_pSettingTable=GTK_TABLE(gtk_builder_get_object(l_pBuilderInterfaceSetting, "box_configuration-table"));
	g_object_unref(l_pBuilderInterfaceSetting);
	
	DEBUG_PRINT(cout << "Entering gtk dialog run\n";)

	boolean l_bFinished=false;
	while(!l_bFinished)
	{
		gint l_iResult=gtk_dialog_run(GTK_DIALOG(m_pWidget));
		if(l_iResult==GTK_RESPONSE_APPLY)
		{
			DEBUG_PRINT(cout << "Apply\n";)
			//We do not apply dynamically the override parameter, so we need to handle it now
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_pFileOverrideCheck)))
			{
				const gchar* l_sFilename = gtk_entry_get_text(m_pOverrideEntry);
				if(m_rBox.hasAttribute(OV_AttributeId_Box_SettingOverrideFilename))
				{
					m_rBox.setAttributeValue(OV_AttributeId_Box_SettingOverrideFilename, l_sFilename);
				}
				else
				{
					m_rBox.addAttribute(OV_AttributeId_Box_SettingOverrideFilename, l_sFilename);
				}
			}
			else
			{
				if(m_rBox.hasAttribute(OV_AttributeId_Box_SettingOverrideFilename))
				{
					m_rBox.removeAttribute(OV_AttributeId_Box_SettingOverrideFilename);
				}
			}

			l_bFinished=true;
			l_bModified=true;
		}
		else if(l_iResult==1) // default
		{
			DEBUG_PRINT(cout << "default\n";)
			uint32 ui32SettingCount = m_rBox.getSettingCount();
			//Find an other solution to do this
			for(uint32 i=0; i<ui32SettingCount; i++)
			{
				CString l_sSettingValue;

				m_rBox.getSettingDefaultValue(i, l_sSettingValue);
				m_rBox.setSettingValue(i, l_sSettingValue);

				if(m_rBox.getSettingCount() != ui32SettingCount)
				{
					ui32SettingCount = m_rBox.getSettingCount();
					i=0;
				}
			}

			gtk_entry_set_text( m_pOverrideEntry, "");
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_pFileOverrideCheck), false);
			gtk_widget_set_sensitive(GTK_WIDGET(l_pSettingTable), true);
			l_bModified=false;
		}
		else if(l_iResult==2) // revert
		{
			DEBUG_PRINT(cout << "Revert\n";)

			m_rBox.restoreState();
			if(m_rBox.hasAttribute(OV_AttributeId_Box_SettingOverrideFilename))
			{
				gtk_entry_set_text(m_pOverrideEntry, m_rBox.getAttributeValue(OV_AttributeId_Box_SettingOverrideFilename).toASCIIString());
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_pFileOverrideCheck), true);
				gtk_widget_set_sensitive(GTK_WIDGET(l_pSettingTable), false);
			}
			else
			{
				gtk_entry_set_text(m_pOverrideEntry, "");
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_pFileOverrideCheck), false);
				gtk_widget_set_sensitive(GTK_WIDGET(l_pSettingTable), true);
			}
		}
		else if(l_iResult==3) // load
		{
			DEBUG_PRINT(cout << "Load\n";)
			l_bModified=true;
		}
		else if(l_iResult==4) // save
		{
			DEBUG_PRINT(cout << "Save\n";)
		}
		else if(l_iResult == GTK_RESPONSE_CANCEL)
		{
			DEBUG_PRINT(cout << "Cancel\n";)

			m_rBox.restoreState();
			if(m_rBox.hasAttribute(OV_AttributeId_Box_SettingOverrideFilename))
			{
				gtk_entry_set_text(m_pOverrideEntry, m_rBox.getAttributeValue(OV_AttributeId_Box_SettingOverrideFilename).toASCIIString());
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_pFileOverrideCheck), true);
				gtk_widget_set_sensitive(GTK_WIDGET(l_pSettingTable), false);
			}
			else
			{
				gtk_entry_set_text(m_pOverrideEntry, "");
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_pFileOverrideCheck), false);
				gtk_widget_set_sensitive(GTK_WIDGET(l_pSettingTable), true);
			}
			l_bFinished=true;
		}
		else
		{
			DEBUG_PRINT(cout << "Finished\n";)
			l_bFinished=true;
		}
	}
	m_rBox.deleteObserver(this);

	return l_bModified;
}

::GtkWidget* CBoxConfigurationDialog::getWidget()
{
	return m_pWidgetToReturn;
}

boolean CBoxConfigurationDialog::update()
{
	for(uint32 i=0; i<m_rBox.getSettingCount(); i++)
	{
		boolean l_bSettingModifiable;
		m_rBox.getSettingMod(i, l_bSettingModifiable);

		if (l_bSettingModifiable) //if this setting is modifiable (this check should be redundant)
		{
			CString l_sSettingName;
			CString l_sSettingValue;

			m_rBox.getSettingName(i, l_sSettingName);
			m_rBox.getSettingValue(i, l_sSettingValue);

			m_mSettingViewMap[l_sSettingName]->setValue(l_sSettingValue);
		}
	}
	return true;
}

void CBoxConfigurationDialog::update(OpenViBE::CObservable &o, void* data)
{
	std::cout << "Entering update" << std::endl;
	const BoxEventMessage *l_pEvent = static_cast< BoxEventMessage * > (data);

	switch(l_pEvent->m_eType)
	{
		case SettingsAllChange:
			generateSettingsTable();
			updateSize();
			break;

		case SettingValueUpdate:
		{
			std::cout << "**** Value : " << l_pEvent->m_i32FirstIndex<< " " << std::endl;
			CString l_sSettingName;
			CString l_sSettingValue;

			m_rBox.getSettingName(l_pEvent->m_i32FirstIndex, l_sSettingName);
			m_rBox.getSettingValue(l_pEvent->m_i32FirstIndex, l_sSettingValue);

			m_mSettingViewMap[l_sSettingName]->setValue(l_sSettingValue);
			std::cout << " Fin de value" <<  std::endl<< std::endl;
			break;
		}

		case SettingDelete:
		std::cout << "**** Delete" << std::endl;
			removeSetting(l_pEvent->m_i32FirstIndex);
			std::cout << " Fin de delete" <<  std::endl << std::endl;
			break;

		case SettingAdd:
			addSetting(l_pEvent->m_i32FirstIndex);
			break;

		case SettingChange:
			std::cout << "**** Changement" << std::endl;
			settingChange(l_pEvent->m_i32FirstIndex);
			std::cout << " Fin de changement" <<  std::endl<< std::endl;
			break;

		default:
		std::cout << "Unhandle box event " << l_pEvent->m_eType << std::endl;
	}
}

void remove_widgets(GtkWidget *widget, gpointer data)
{
	gtk_container_remove(GTK_CONTAINER(data), widget);
}

void remove_rows(GtkWidget* data)
{
	gtk_container_foreach(GTK_CONTAINER(data), remove_widgets, data);
}

void CBoxConfigurationDialog::generateSettingsTable()
{
	clearSettingWrappersVector();
	remove_rows(GTK_WIDGET(m_pSettingsTable));

	// count the number of modifiable settings
	uint32 l_ui32TableSize = 0;
	for(uint32 i=0; i<m_rBox.getSettingCount(); i++)
	{
		if (m_bIsScenarioRunning)
		{
			boolean l_IsMod = false;
			m_rBox.getSettingMod(i, l_IsMod);
			if (l_IsMod)
			{
				l_ui32TableSize++;
			}
		}
		else
		{
			l_ui32TableSize++;
		}
	}
	gtk_table_resize(m_pSettingsTable, l_ui32TableSize+2, 4);


	// Iterate over box settings, generate corresponding gtk widgets. If the scenario is running, we are making a
	// 'modifiable settings' dialog and use a subset of widgets with a slightly different layout and buttons.
	for(uint32 i=0,j=0; i<m_rBox.getSettingCount(); i++)
	{
		if(this->addSettingsToView(i, j))
		{
			++j;
		}
	}
	std::cout << "Number of setting display " << m_vSettingWrappers.size() << std::endl;
}

boolean CBoxConfigurationDialog::addSettingsToView(uint32 ui32SettingIndex, OpenViBE::uint32 ui32TableIndex)
{
	std::cout << "Add Setting " << ui32SettingIndex << " to place " << ui32TableIndex << std::endl;
	boolean l_bSettingModifiable;
	m_rBox.getSettingMod(ui32SettingIndex, l_bSettingModifiable);

	//if the scenario is not running we take all the settings
	//otherwise, we take only the modifiable ones
	if( (!m_bIsScenarioRunning) || (m_bIsScenarioRunning && l_bSettingModifiable) )
	{
		CString l_sSettingName;

		m_rBox.getSettingName(ui32SettingIndex, l_sSettingName);
		Setting::CAbstractSettingView* l_oView = m_oSettingFactory.getSettingView(m_rBox, ui32SettingIndex);

		gtk_table_attach(m_pSettingsTable, l_oView->getNameWidget() ,   0, 1, ui32TableIndex, ui32TableIndex+1, ::GtkAttachOptions(GTK_FILL), ::GtkAttachOptions(GTK_FILL), 0, 0);
		gtk_table_attach(m_pSettingsTable, l_oView->getEntryWidget(),   1, 4, ui32TableIndex, ui32TableIndex+1, ::GtkAttachOptions(GTK_SHRINK|GTK_FILL|GTK_EXPAND), ::GtkAttachOptions(GTK_SHRINK), 0, 0);

		m_mSettingViewMap[l_sSettingName] = l_oView;
		m_mSettingViewIndexMap[l_sSettingName] = ui32TableIndex;

		m_vSettingWrappers.insert(m_vSettingWrappers.begin()+ui32TableIndex, CSettingViewWrapper(ui32SettingIndex, l_oView));

		return true;
	}
	return false;
}

void CBoxConfigurationDialog::settingChange(uint32 ui32SettingIndex)
{
	//We remeber the place to add the new setting at the same place
	uint32 l_ui32IndexTable = getTableIndex(ui32SettingIndex);

	removeSetting(ui32SettingIndex, false);
	addSettingsToView(ui32SettingIndex, l_ui32IndexTable);
}

void CBoxConfigurationDialog::addSetting(uint32 ui32SettingIndex)
{
	boolean l_bSettingModifiable;
	m_rBox.getSettingMod(ui32SettingIndex, l_bSettingModifiable);
	//If we don't have to print the setting we just do nothing
	if( (!m_bIsScenarioRunning) || (m_bIsScenarioRunning && l_bSettingModifiable) )
	{
		uint32 l_ui32TableSize = getTableSize();
		/*There is two case.
		1) we just add at the end of the setting box
		2) we add it in the middle end we need to shift
		*/
		uint32 l_ui32TableIndex;
		if(ui32SettingIndex > m_vSettingWrappers[l_ui32TableSize-1].m_ui32SettingIndex){
			l_ui32TableIndex = l_ui32TableSize;
		}
		else
		{
			l_ui32TableIndex = getTableIndex(ui32SettingIndex);
		}

		gtk_table_resize(m_pSettingsTable, l_ui32TableSize+2, 4);

		if(ui32SettingIndex <= m_vSettingWrappers[l_ui32TableSize-1].m_ui32SettingIndex)
		{
			for(size_t i = l_ui32TableSize-1; i >= l_ui32TableIndex ; --i)
			{
				std::cout << "Move setting " << i << " to place " << i+1 << std::endl;
				Setting::CAbstractSettingView *l_oView = m_vSettingWrappers[i].m_pView;

				//We need to update the index
				++(m_vSettingWrappers[i].m_ui32SettingIndex);
				l_oView->setSettingIndex(m_vSettingWrappers[i].m_ui32SettingIndex);

				gtk_container_remove(GTK_CONTAINER(m_pSettingsTable), l_oView->getNameWidget());
				gtk_table_attach(m_pSettingsTable, l_oView->getNameWidget() ,   0, 1, i+1, i+2, ::GtkAttachOptions(GTK_FILL), ::GtkAttachOptions(GTK_FILL), 0, 0);

				gtk_container_remove(GTK_CONTAINER(m_pSettingsTable), l_oView->getEntryWidget());
				gtk_table_attach(m_pSettingsTable, l_oView->getEntryWidget(),   1, 4, i+1, i+2, ::GtkAttachOptions(GTK_SHRINK|GTK_FILL|GTK_EXPAND), ::GtkAttachOptions(GTK_SHRINK), 0, 0);
			}
		}
		addSettingsToView(l_ui32TableIndex, ui32SettingIndex);
		updateSize();
	}
	//Even if nothing is add to the interface, we still need to update index
	else
	{
		for(size_t i = 0; i < m_vSettingWrappers.size() ; ++i)
		{
			CSettingViewWrapper& l_rTemp = m_vSettingWrappers[i];
			if(l_rTemp.m_ui32SettingIndex >= ui32SettingIndex)
			{
				++l_rTemp.m_ui32SettingIndex;
				l_rTemp.m_pView->setSettingIndex(l_rTemp.m_ui32SettingIndex);
			}
		}
	}
}

void CBoxConfigurationDialog::clearSettingWrappersVector(void)
{
	for (std::vector<CSettingViewWrapper>::iterator it = m_vSettingWrappers.begin() ; it != m_vSettingWrappers.end(); ++it)
	{
		CSettingViewWrapper &l_oView = *it;
		delete l_oView.m_pView;
	}
	m_vSettingWrappers.clear();
}


void CBoxConfigurationDialog::removeSetting(uint32 ui32SettingIndex, boolean bShift)
{
	int32 i32TableIndex = getTableIndex(ui32SettingIndex);

	if(i32TableIndex != -1)
	{
		CSettingViewWrapper &l_rViewWrapper = m_vSettingWrappers[i32TableIndex];
		::GtkWidget* l_pName = l_rViewWrapper.m_pView->getNameWidget();
		::GtkWidget* l_pEntry = l_rViewWrapper.m_pView->getEntryWidget();

		gtk_container_remove(GTK_CONTAINER(m_pSettingsTable), l_pName);
		gtk_container_remove(GTK_CONTAINER(m_pSettingsTable), l_pEntry);

		delete l_rViewWrapper.m_pView;
		m_vSettingWrappers.erase(m_vSettingWrappers.begin() + i32TableIndex);

		//Now if we need to do it we shift everything to avoid an empty row in the table
		if(bShift){

			for(size_t i = i32TableIndex; i < m_vSettingWrappers.size() ; ++i)
			{
				Setting::CAbstractSettingView *l_oView = m_vSettingWrappers[i].m_pView;
				--(m_vSettingWrappers[i].m_ui32SettingIndex);
				l_oView->setSettingIndex(m_vSettingWrappers[i].m_ui32SettingIndex);

				gtk_container_remove(GTK_CONTAINER(m_pSettingsTable), l_oView->getNameWidget());
				gtk_table_attach(m_pSettingsTable, l_oView->getNameWidget() ,   0, 1, i, i+1, ::GtkAttachOptions(GTK_FILL), ::GtkAttachOptions(GTK_FILL), 0, 0);

				gtk_container_remove(GTK_CONTAINER(m_pSettingsTable), l_oView->getEntryWidget());
				gtk_table_attach(m_pSettingsTable, l_oView->getEntryWidget(),   1, 4, i, i+1, ::GtkAttachOptions(GTK_SHRINK|GTK_FILL|GTK_EXPAND), ::GtkAttachOptions(GTK_SHRINK), 0, 0);
			}
			//Now let's resize everything
			gtk_table_resize(m_pSettingsTable, m_vSettingWrappers.size()+2, 4);
			updateSize();
		}
	}
	//Even if we delete an "invisible" setting we need to update every index.
	else
	{
		for(size_t i = 0; i < m_vSettingWrappers.size() ; ++i)
		{
			CSettingViewWrapper& l_rTemp = m_vSettingWrappers[i];
			if(l_rTemp.m_ui32SettingIndex >= ui32SettingIndex)
			{
				--l_rTemp.m_ui32SettingIndex;
				l_rTemp.m_pView->setSettingIndex(l_rTemp.m_ui32SettingIndex);
			}
		}
	}
}

int32 CBoxConfigurationDialog::getTableIndex(uint32 ui32SettingIndex)
{
	uint32 ui32TableIndex=0;
	for (std::vector<CSettingViewWrapper>::iterator it = m_vSettingWrappers.begin() ; it != m_vSettingWrappers.end(); ++it, ++ui32TableIndex)
	{
		CSettingViewWrapper &l_rViewWrapper = *it;
		if(l_rViewWrapper.m_ui32SettingIndex == ui32SettingIndex){
			return ui32SettingIndex;
		}
	}
	//Find a better code return or switch to signed int
	return -1;
}

uint32 CBoxConfigurationDialog::getTableSize()
{
	return m_vSettingWrappers.size();
}

void CBoxConfigurationDialog::onOverrideBrowse()
{
	::GtkWidget* l_pWidgetDialogOpen=gtk_file_chooser_dialog_new(
		"Select file to open...",
		NULL,
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	CString l_sInitialFileName=m_rKernelContext.getConfigurationManager().expand(gtk_entry_get_text(m_pOverrideEntry));
	if(g_path_is_absolute(l_sInitialFileName.toASCIIString()))
	{
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen), l_sInitialFileName.toASCIIString());
	}
	else
	{
		char* l_sFullPath=g_build_filename(g_get_current_dir(), l_sInitialFileName.toASCIIString(), NULL);
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen), l_sFullPath);
		g_free(l_sFullPath);
	}

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(l_pWidgetDialogOpen), false);

	if(gtk_dialog_run(GTK_DIALOG(l_pWidgetDialogOpen))==GTK_RESPONSE_ACCEPT)
	{
		char* l_sFileName=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen));
		char* l_pBackslash = NULL;
		while((l_pBackslash = ::strchr(l_sFileName, '\\'))!=NULL)
		{
			*l_pBackslash = '/';
		}

		gtk_entry_set_text(m_pOverrideEntry, l_sFileName);
		g_free(l_sFileName);
	}
	gtk_widget_destroy(l_pWidgetDialogOpen);
}



void CBoxConfigurationDialog::updateSize()
{
	// Resize the window to fit as much of the table as possible, but keep the max size
	// limited so it doesn't get outside the screen. For safety, we cap to 800x600
	// anyway to hopefully prevent the window from going under things such as the gnome toolbar.
	// The ui file at the moment does not allow resize of this window because the result
	// looked ugly if the window was made overly large, and no satisfying solution at the time was
	// found by the limited intellectual resources available.
	const uint32 l_ui32MaxWidth = std::min(800,gdk_screen_get_width(gdk_screen_get_default()));
	const uint32 l_ui32MaxHeight = std::min(600,gdk_screen_get_height(gdk_screen_get_default()));
	GtkRequisition l_oSize;
	gtk_widget_size_request(GTK_WIDGET(m_pViewPort), &l_oSize);
	gtk_widget_set_size_request(GTK_WIDGET(m_pScrolledWindow),
		std::min(l_ui32MaxWidth,(uint32)l_oSize.width),
								std::min(l_ui32MaxHeight,(uint32)l_oSize.height));
}

void CBoxConfigurationDialog::saveConfiguration()
{
	::GtkWidget* l_pWidgetDialogOpen=gtk_file_chooser_dialog_new(
		"Select file to save settings to...",
		NULL,
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	const gchar* l_sInitialFileNameToExpand = gtk_entry_get_text(m_pOverrideEntry);
	CString l_sInitialFileName=m_rKernelContext.getConfigurationManager().expand(l_sInitialFileNameToExpand);
	if(g_path_is_absolute(l_sInitialFileName.toASCIIString()))
	{
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen), l_sInitialFileName.toASCIIString());
	}
	else
	{
		char* l_sFullPath=g_build_filename(g_get_current_dir(), l_sInitialFileName.toASCIIString(), NULL);
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen), l_sFullPath);
		g_free(l_sFullPath);
	}

	if(gtk_dialog_run(GTK_DIALOG(l_pWidgetDialogOpen))==GTK_RESPONSE_ACCEPT)
	{
		char* l_sFileName=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen));

		XML::IXMLHandler *l_pHandler = XML::createXMLHandler();
		XML::IXMLNode *l_pRootNode = XML::createNode(c_sRootName);
		for(size_t i = 0; i < m_rBox.getSettingCount() ; ++i)
		{
			XML::IXMLNode *l_pTempNode = XML::createNode(c_sSettingName);
			CString l_sValue;
			m_rBox.getSettingValue(i, l_sValue);
			l_pTempNode->setPCData(l_sValue.toASCIIString());

			l_pRootNode->addChild(l_pTempNode);
		}

		l_pHandler->writeXMLInFile(*l_pRootNode, l_sFileName);

		l_pHandler->release();
		l_pRootNode->release();
		g_free(l_sFileName);

	}
	gtk_widget_destroy(l_pWidgetDialogOpen);
}

void CBoxConfigurationDialog::loadConfiguration()
{
	::GtkWidget* l_pWidgetDialogOpen=gtk_file_chooser_dialog_new(
		"Select file to save settings to...",
		NULL,
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	const gchar* l_sInitialFileNameToExpand = gtk_entry_get_text(m_pOverrideEntry);

	CString l_sInitialFileName=m_rKernelContext.getConfigurationManager().expand(l_sInitialFileNameToExpand);
	if(g_path_is_absolute(l_sInitialFileName.toASCIIString()))
	{
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen), l_sInitialFileName.toASCIIString());
	}
	else
	{
		char* l_sFullPath=g_build_filename(g_get_current_dir(), l_sInitialFileName.toASCIIString(), NULL);
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen), l_sFullPath);
		g_free(l_sFullPath);
	}

	if(gtk_dialog_run(GTK_DIALOG(l_pWidgetDialogOpen))==GTK_RESPONSE_ACCEPT)
	{
		char* l_sFileName=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(l_pWidgetDialogOpen));

		XML::IXMLHandler *l_pHandler = XML::createXMLHandler();
		XML::IXMLNode *l_pRootNode = l_pHandler->parseFile(l_sFileName);

		for(size_t i = 0 ; i<l_pRootNode->getChildCount() ; ++i)
		{
			//Hope everything will fit in the right place
			m_rBox.setSettingValue(i, l_pRootNode->getChild(i)->getPCData());
		}

		l_pRootNode->release();
		l_pHandler->release();
		g_free(l_sFileName);

	}
	gtk_widget_destroy(l_pWidgetDialogOpen);
}

const CIdentifier CBoxConfigurationDialog::getBoxID() const
{
	return m_rBox.getIdentifier();
}
