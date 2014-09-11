#include "ovdCAbstractSettingView.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBEDesigner;
using namespace OpenViBEDesigner::Setting;


static void collect_widget_cb(::GtkWidget* pWidget, gpointer pUserData)
{
	static_cast< std::vector< ::GtkWidget* > *>(pUserData)->push_back(pWidget);
}

OpenViBE::CString CAbstractSettingView::getSettingWidgetName(void)
{
	return m_sSettingWidgetName;
}

CAbstractSettingView::~CAbstractSettingView()
{
	if(GTK_IS_WIDGET(m_pNameWidget)){
		gtk_widget_destroy(m_pNameWidget);
	}
	if(GTK_IS_WIDGET(m_pEntryWidget)){
		gtk_widget_destroy(m_pEntryWidget);
	}
}

CAbstractSettingView::CAbstractSettingView(OpenViBE::Kernel::IBox& rBox, OpenViBE::uint32 ui32Index, CString &rBuilderName):
	m_rBox(rBox),
	m_ui32Index(ui32Index),
	m_sSettingWidgetName(""),
	m_pNameWidget(NULL),
	m_pEntryWidget(NULL),
	m_bOnValueSetting(false)
{
	m_pBuilder = gtk_builder_new();
	gtk_builder_add_from_file(m_pBuilder, rBuilderName.toASCIIString(), NULL);
	gtk_builder_connect_signals(m_pBuilder, NULL);
}



OpenViBE::Kernel::IBox& CAbstractSettingView::getBox(void)
{
	return m_rBox;
}

OpenViBE::uint32 CAbstractSettingView::getSettingsIndex(void)
{
	return m_ui32Index;
}

void CAbstractSettingView::setSettingWidgetName(const CString &rWidgetName)
{
	m_sSettingWidgetName = rWidgetName;
}



void CAbstractSettingView::setNameWidget(::GtkWidget* pWidget)
{
	if(m_pNameWidget){
		gtk_widget_destroy(m_pNameWidget);
	}
	m_pNameWidget = pWidget;
}

::GtkWidget* CAbstractSettingView::getNameWidget(void)
{
	return m_pNameWidget;
}

void CAbstractSettingView::setEntryWidget(::GtkWidget* pWidget)
{
	if(m_pEntryWidget){
		gtk_widget_destroy(m_pEntryWidget);
	}
	m_pEntryWidget = pWidget;
}

void CAbstractSettingView::generateNameWidget(void)
{
	::GtkWidget* l_pSettingName=GTK_WIDGET(gtk_builder_get_object(m_pBuilder, "settings_collection-label_setting_name"));
	gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(l_pSettingName)), l_pSettingName);
	setNameWidget(l_pSettingName);

	CString l_sSettingName;
	getBox().getSettingName(m_ui32Index, l_sSettingName);
	gtk_label_set_text(GTK_LABEL(l_pSettingName), l_sSettingName);
}

GtkWidget *CAbstractSettingView::generateEntryWidget(void)
{
	::GtkTable* m_pTable = GTK_TABLE(gtk_table_new(1, 3, false));

	::GtkWidget* l_pSettingWidget=GTK_WIDGET(gtk_builder_get_object(m_pBuilder, getSettingWidgetName().toASCIIString()));
	::GtkWidget* l_pSettingRevert=GTK_WIDGET(gtk_builder_get_object(m_pBuilder, "settings_collection-button_setting_revert"));
	::GtkWidget* l_pSettingDefault=GTK_WIDGET(gtk_builder_get_object(m_pBuilder, "settings_collection-button_setting_default"));

	gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(l_pSettingWidget)), l_pSettingWidget);
	gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(l_pSettingRevert)), l_pSettingRevert);
	gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(l_pSettingDefault)), l_pSettingDefault);

	gtk_table_attach(m_pTable, l_pSettingWidget,  0, 1, 0, 1, ::GtkAttachOptions(GTK_FILL|GTK_EXPAND), ::GtkAttachOptions(GTK_FILL|GTK_EXPAND), 0, 0);
	gtk_table_attach(m_pTable, l_pSettingDefault, 1, 2, 0, 1, ::GtkAttachOptions(GTK_SHRINK),          ::GtkAttachOptions(GTK_SHRINK),          0, 0);
	gtk_table_attach(m_pTable, l_pSettingRevert,  2, 3, 0, 1, ::GtkAttachOptions(GTK_SHRINK),          ::GtkAttachOptions(GTK_SHRINK),          0, 0);

	setEntryWidget(GTK_WIDGET(m_pTable));
	gtk_widget_set_visible(getEntryWidget(), true);
	//If we don't increase the ref counter it will cause trouble when we gonna move it later
	g_object_ref(G_OBJECT(m_pTable));
	return l_pSettingWidget;
}

void CAbstractSettingView::initializeValue()
{
	CString l_sSettingValue;
	getBox().getSettingValue(m_ui32Index, l_sSettingValue);
	setValue(l_sSettingValue);
}

void CAbstractSettingView::extractWidget(GtkWidget *pWidget, std::vector< ::GtkWidget* > &rVector)
{
	gtk_container_foreach(GTK_CONTAINER(pWidget), collect_widget_cb, &rVector);
}

::GtkWidget* CAbstractSettingView::getEntryWidget(void)
{
	return m_pEntryWidget;
}
