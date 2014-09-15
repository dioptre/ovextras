#include "ovdCBoxConfigurationDialog.h"
#include "settings/ovdCSettingViewFactory.h"
#include "settings/ovdCAbstractSettingView.h"

#include <string>
#include <fstream>
#include <iostream>

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

static const char * const c_sRootName = "OpenViBE-SettingsOverride";
static const char *const c_sSettingName = "SettingValue";

namespace
{
	// These callbacks are used for loading and saving the parameters to/from xml files
	class CXMLWriterCallback : public XML::IWriterCallback
	{
	public:

		CXMLWriterCallback(const string& sFilename)
		{
			m_pFile=fopen(sFilename.c_str(), "wt");
			if(!m_pFile)
			{
				cout << "Error opening " << sFilename << " for writing\n";
			}
		}

		virtual ~CXMLWriterCallback(void)
		{
			if(m_pFile)
			{
				fclose(m_pFile);
			}
		}

		virtual void write(const char* sString)
		{
			if(m_pFile)
			{
				fprintf(m_pFile, "%s", sString);
			}
		}

	protected:

		FILE* m_pFile;
	};

	class CXMLReaderCallback : public XML::IReaderCallback
	{
	public:

		CXMLReaderCallback(SButtonCB& rButtonCB)
			:m_ui32Status(Status_ParsingNone)
			,m_rButtonCB(rButtonCB)
		{
		}

		virtual ~CXMLReaderCallback(void)
		{
		}

		virtual void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
		{
			string l_sName(sName);
			switch(m_ui32Status)
			{
				case Status_ParsingNone:
					if(l_sName=="OpenViBE-SettingsOverride")
					{
						m_ui32Status=Status_ParsingSettingsOverride;
						m_ui32SettingIndex=(uint32)-1;
					}
					break;

				case Status_ParsingSettingsOverride:
					if(l_sName=="SettingValue")
					{
						m_ui32Status=Status_ParsingSettingValue;
						m_ui32SettingIndex++;
					}
					break;
			}
		}

		virtual void processChildData(const char* sData)
		{
			switch(m_ui32Status)
			{
				case Status_ParsingSettingValue:
					if(m_ui32SettingIndex<m_rButtonCB.m_rBox.getSettingCount())
					{
						CString l_sSettingName;
						CIdentifier l_oSettingType;

						m_rButtonCB.m_rBox.getSettingName(m_ui32SettingIndex, l_sSettingName);
						m_rButtonCB.m_rBox.getSettingType(m_ui32SettingIndex, l_oSettingType);
						m_rButtonCB.m_rHelper.setValue(l_oSettingType, m_rButtonCB.m_rSettingWidget[l_sSettingName], sData);

						m_rButtonCB.m_rBox.setSettingValue(m_ui32SettingIndex,sData);
					}
					break;
			}
		}

		virtual void closeChild(void)
		{
			switch(m_ui32Status)
			{
				case Status_ParsingSettingValue:
					m_ui32Status=Status_ParsingSettingsOverride;
					break;

				case Status_ParsingSettingsOverride:
					m_ui32Status=Status_ParsingNone;
					break;
			}
		}

	protected:

		enum
		{
			Status_ParsingNone,
			Status_ParsingSettingsOverride,
			Status_ParsingSettingValue,
		};

		uint32 m_ui32Status;
		uint32 m_ui32SettingIndex;
		SButtonCB& m_rButtonCB;
	};
};

// This callback is used to gray out the settings if file override is selected
static void on_file_override_check_toggled(::GtkToggleButton* pToggleButton, gpointer pUserData)
{
	const gboolean l_oPreviousValue = gtk_toggle_button_get_active(pToggleButton);
	gtk_widget_set_sensitive((::GtkWidget*)pUserData, !l_oPreviousValue);
}

static void on_button_load_clicked(::GtkButton* pButton, gpointer pUserData)
{
	SButtonCB* l_pUserData=static_cast < SButtonCB* >(pUserData);

	::GtkWidget* l_pWidgetDialogOpen=gtk_file_chooser_dialog_new(
		"Select file to load settings from...",
		NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	CString l_sInitialFileName=l_pUserData->m_rKernelContext.getConfigurationManager().expand(l_pUserData->m_rHelper.getValue(OV_TypeId_Filename, l_pUserData->m_pSettingOverrideValue));
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

		CXMLReaderCallback l_oCB(*l_pUserData);
		XML::IReader* l_pReader=XML::createReader(l_oCB);

		ifstream l_oFile(l_sFileName, ios::binary);
		if(l_oFile.is_open())
		{
			bool l_bStatusOk=true;
			char l_sBuffer[1024];
			size_t l_iBufferLen=0;
			size_t l_iFileLen;
			l_oFile.seekg(0, ios::end);
			l_iFileLen=(size_t)l_oFile.tellg();
			l_oFile.seekg(0, ios::beg);
			while(l_iFileLen && l_bStatusOk)
			{
				l_iBufferLen=(l_iFileLen>sizeof(l_sBuffer)?sizeof(l_sBuffer):l_iFileLen);
				l_oFile.read(l_sBuffer, l_iBufferLen);
				l_iFileLen-=l_iBufferLen;
				l_bStatusOk=l_pReader->processData(l_sBuffer, l_iBufferLen);
			}
			l_oFile.close();
		}
		else
		{
			cout << "Error opening " << l_sFileName << " for reading\n";
		}
		l_pReader->release();

		g_free(l_sFileName);
	}
	gtk_widget_destroy(l_pWidgetDialogOpen);
}

static void on_button_save_clicked(::GtkButton* pButton, gpointer pUserData)
{
	static_cast<CBoxConfigurationDialog *>(pUserData)->saveConfiguration();
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
	,m_pButtonCB(NULL)
	,m_pFileOverrideCheck(NULL)
{
	m_pHelper = new CSettingCollectionHelper(m_rKernelContext, m_sGUISettingsFilename.toASCIIString());

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
		g_object_unref(l_pBuilderInterfaceSetting);

		generateSettingsTable();
		updateSize();

		// If a scenario is not running, we add the buttons to save/load etc
		if (!m_bIsScenarioRunning)
		{
			string l_sSettingOverrideWidgetName=m_pHelper->getSettingWidgetName(OV_TypeId_Filename).toASCIIString();
			::GtkBuilder* l_pBuilderInterfaceSettingCollection=gtk_builder_new(); // glade_xml_new(m_sGUIFilename.toASCIIString(), l_sSettingOverrideWidgetName.c_str(), NULL);
			gtk_builder_add_from_file(l_pBuilderInterfaceSettingCollection, m_sGUISettingsFilename.toASCIIString(), NULL);
			gtk_builder_connect_signals(l_pBuilderInterfaceSettingCollection, NULL);

			//::GtkWidget* l_pSettingOverrideValue=GTK_WIDGET(gtk_builder_get_object(l_pBuilderInterfaceSettingCollection, l_sSettingOverrideWidgetName.c_str()));
			m_pSettingOverrideValue=GTK_WIDGET(gtk_builder_get_object(l_pBuilderInterfaceSettingCollection, l_sSettingOverrideWidgetName.c_str()));

			gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(m_pSettingOverrideValue)), m_pSettingOverrideValue);
			gtk_container_add(l_pFileOverrideContainer, m_pSettingOverrideValue);

			g_object_unref(l_pBuilderInterfaceSettingCollection);

			DEBUG_PRINT(cout << "Creating CB\n";)
			m_pButtonCB = new SButtonCB(m_rKernelContext, m_mSettingWidget, m_mSettingViewMap, *m_pHelper, m_pSettingOverrideValue, m_rBox);

			g_signal_connect(G_OBJECT(m_pFileOverrideCheck), "toggled", G_CALLBACK(on_file_override_check_toggled), GTK_WIDGET(m_pSettingsTable));
			g_signal_connect(G_OBJECT(l_pButtonLoad),        "clicked", G_CALLBACK(on_button_load_clicked), m_pButtonCB);
			g_signal_connect(G_OBJECT(l_pButtonSave),        "clicked", G_CALLBACK(on_button_save_clicked), this);

			if(m_rBox.hasAttribute(OV_AttributeId_Box_SettingOverrideFilename))
			{
				m_pHelper->setValue(OV_TypeId_Filename, m_pSettingOverrideValue, m_rBox.getAttributeValue(OV_AttributeId_Box_SettingOverrideFilename));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_pFileOverrideCheck), true);
				gtk_widget_set_sensitive(GTK_WIDGET(m_pSettingsTable), false);
			}
			else
			{
				m_pHelper->setValue(OV_TypeId_Filename, m_pSettingOverrideValue, "");
			}
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

	if(m_pHelper) 
	{
		delete m_pHelper;
		m_pHelper = NULL;
	}
	if(m_pButtonCB) 
	{
		delete m_pButtonCB;
		m_pButtonCB = NULL;
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

//			// Here we go through the box settings of the GUI, and set them to the box. As changing the value in the
//			// box may change the number of settings, we redo until the setting count stays the same. This probably
//			// has to be reworked later...
//			//FIXME should only call for changed argument to avoid to overwrite some value
			uint32 l_ui32InitialSettingCount = m_rBox.getSettingCount();
			for(uint32 i=0; i<l_ui32InitialSettingCount; i++)
			{
				CString l_sSettingName;
				CString l_sSettingValue;

				m_rBox.getSettingName(i, l_sSettingName);

				if(m_mSettingViewMap[l_sSettingName])
				{
					CString l_sValue;
					Setting::CAbstractSettingView *l_pView =  m_mSettingViewMap[l_sSettingName];
					l_pView->getValue(l_sValue);
					std::cout << "Value set : " << l_sValue << std::endl;
					m_rBox.setSettingValue(i, l_sValue);
				}
				else
				{
					DEBUG_PRINT(cout << "Setting no " << i << " '" << l_sSettingName << "' not found in the param map, this means new params were added after gtk constructed the param widget...\n";)
					DEBUG_PRINT(m_rBox.getSettingValue(i, l_sSettingValue); cout << "Old value is " << l_sSettingValue << "\n";)
				}

				if(m_rBox.getSettingCount() != l_ui32InitialSettingCount)
				{
					// Do again, setting values changed the amount of entries
					i = 0;
					l_ui32InitialSettingCount = m_rBox.getSettingCount();
				}
			}

			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_pFileOverrideCheck)))
			{
				if(m_rBox.hasAttribute(OV_AttributeId_Box_SettingOverrideFilename))
				{
					m_rBox.setAttributeValue(OV_AttributeId_Box_SettingOverrideFilename, m_pHelper->getValue(OV_TypeId_Filename, m_pSettingOverrideValue));
				}
				else
				{
					m_rBox.addAttribute(OV_AttributeId_Box_SettingOverrideFilename, m_pHelper->getValue(OV_TypeId_Filename, m_pSettingOverrideValue));
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
				CString l_sSettingName;
				CString l_sSettingValue;

				m_rBox.getSettingName(i, l_sSettingName);
				m_rBox.getSettingDefaultValue(i, l_sSettingValue);
				m_rBox.setSettingValue(i, l_sSettingValue);

				if(m_rBox.getSettingCount() != ui32SettingCount)
				{
					ui32SettingCount = m_rBox.getSettingCount();
					i=0;
				}
			}

			m_pHelper->setValue(OV_TypeId_Filename, m_pSettingOverrideValue, "");
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
				m_pHelper->setValue(OV_TypeId_Filename, m_pSettingOverrideValue, m_rBox.getAttributeValue(OV_AttributeId_Box_SettingOverrideFilename));
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_pFileOverrideCheck), true);
				gtk_widget_set_sensitive(GTK_WIDGET(l_pSettingTable), false);
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
				++(m_vSettingWrappers[i].m_ui32SettingIndex);

				gtk_container_remove(GTK_CONTAINER(m_pSettingsTable), l_oView->getNameWidget());
				gtk_table_attach(m_pSettingsTable, l_oView->getNameWidget() ,   0, 1, i+1, i+2, ::GtkAttachOptions(GTK_FILL), ::GtkAttachOptions(GTK_FILL), 0, 0);

				gtk_container_remove(GTK_CONTAINER(m_pSettingsTable), l_oView->getEntryWidget());
				gtk_table_attach(m_pSettingsTable, l_oView->getEntryWidget(),   1, 4, i+1, i+2, ::GtkAttachOptions(GTK_SHRINK|GTK_FILL|GTK_EXPAND), ::GtkAttachOptions(GTK_SHRINK), 0, 0);
			}
		}
		addSettingsToView(l_ui32TableIndex, ui32SettingIndex);
		updateSize();
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
	uint32 ui32TableIndex = getTableIndex(ui32SettingIndex);

	CSettingViewWrapper &l_rViewWrapper = m_vSettingWrappers[ui32TableIndex];
	::GtkWidget* l_pName = l_rViewWrapper.m_pView->getNameWidget();
	::GtkWidget* l_pEntry = l_rViewWrapper.m_pView->getEntryWidget();

	gtk_container_remove(GTK_CONTAINER(m_pSettingsTable), l_pName);
	gtk_container_remove(GTK_CONTAINER(m_pSettingsTable), l_pEntry);

	delete l_rViewWrapper.m_pView;
	m_vSettingWrappers.erase(m_vSettingWrappers.begin() + ui32TableIndex);

	//Now if we need to do it we shift everything to avoid an empty row in the table
	if(bShift){

		for(size_t i = ui32TableIndex; i < m_vSettingWrappers.size() ; ++i)
		{
			Setting::CAbstractSettingView *l_oView = m_vSettingWrappers[i].m_pView;
			--(m_vSettingWrappers[i].m_ui32SettingIndex);

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

uint32 CBoxConfigurationDialog::getTableIndex(uint32 ui32SettingIndex)
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
	return 0xFFFFFFF;
}

uint32 CBoxConfigurationDialog::getTableSize()
{
	return m_vSettingWrappers.size();
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

	CString l_sInitialFileNameToExpand = m_pHelper->getValue(OV_TypeId_Filename, m_pSettingOverrideValue);
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

const CIdentifier CBoxConfigurationDialog::getBoxID() const
{
	return m_rBox.getIdentifier();
}
