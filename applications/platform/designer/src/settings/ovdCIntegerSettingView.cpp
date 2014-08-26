#include "ovdCIntegerSettingView.h"
#include "../ovd_base.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBEDesigner;
using namespace OpenViBEDesigner::Setting;

static void collect_widget_cb(::GtkWidget* pWidget, gpointer pUserData)
{
	static_cast< std::vector< ::GtkWidget* > *>(pUserData)->push_back(pWidget);
}

static void on_button_setting_integer_up_pressed(::GtkButton* pButton, gpointer pUserData)
{
	static_cast<CIntegerSettingView *>(pUserData)->adjustValue(1);
}

static void on_button_setting_integer_down_pressed(::GtkButton* pButton, gpointer pUserData)
{
	static_cast<CIntegerSettingView *>(pUserData)->adjustValue(-1);
}

CIntegerSettingView::CIntegerSettingView(OpenViBE::Kernel::IBox &rBox, OpenViBE::uint32 ui32Index, CString &rBuilderName, const Kernel::IKernelContext &rKernelContext):
	CAbstractSettingView(rBox, ui32Index), m_rKernelContext(rKernelContext)
{
	setSettingWidgetName("settings_collection-hbox_setting_integer");

	::GtkBuilder *l_pBuilder = gtk_builder_new();
	gtk_builder_add_from_file(l_pBuilder, rBuilderName.toASCIIString(), NULL);
	gtk_builder_connect_signals(l_pBuilder, NULL);

	::GtkWidget* l_pSettingWidget=GTK_WIDGET(gtk_builder_get_object(l_pBuilder, getSettingWidgetName().toASCIIString()));

	::GtkTable* m_pTable = GTK_TABLE(gtk_table_new(1, 3, false));

	::GtkWidget* l_pSettingName=GTK_WIDGET(gtk_builder_get_object(l_pBuilder, "settings_collection-label_setting_name"));
	::GtkWidget* l_pSettingRevert=GTK_WIDGET(gtk_builder_get_object(l_pBuilder, "settings_collection-button_setting_revert"));
	::GtkWidget* l_pSettingDefault=GTK_WIDGET(gtk_builder_get_object(l_pBuilder, "settings_collection-button_setting_default"));

	std::cout << l_pSettingName <<std::endl;

	gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(l_pSettingName)), l_pSettingName);
	gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(l_pSettingWidget)), l_pSettingWidget);
	gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(l_pSettingRevert)), l_pSettingRevert);
	gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(l_pSettingDefault)), l_pSettingDefault);

	gtk_table_attach(m_pTable, l_pSettingWidget,  0, 1, 0, 1, ::GtkAttachOptions(GTK_FILL|GTK_EXPAND), ::GtkAttachOptions(GTK_FILL|GTK_EXPAND), 0, 0);
	gtk_table_attach(m_pTable, l_pSettingDefault, 1, 2, 0, 1, ::GtkAttachOptions(GTK_SHRINK),          ::GtkAttachOptions(GTK_SHRINK),          0, 0);
	gtk_table_attach(m_pTable, l_pSettingRevert,  2, 3, 0, 1, ::GtkAttachOptions(GTK_SHRINK),          ::GtkAttachOptions(GTK_SHRINK),          0, 0);

	setEntryWidget(GTK_WIDGET(m_pTable));
	gtk_widget_set_visible(getEntryWidget(), true);

	setNameWidget(l_pSettingName);

	CString l_sSettingName;
	getBox().getSettingName(ui32Index, l_sSettingName);
	gtk_label_set_text(GTK_LABEL(l_pSettingName), l_sSettingName);

	std::vector< ::GtkWidget* > l_vWidget;
	gtk_container_foreach(GTK_CONTAINER(l_pSettingWidget), collect_widget_cb, &l_vWidget);
	m_pEntry = GTK_ENTRY(l_vWidget[0]);

	g_signal_connect(G_OBJECT(l_vWidget[1]), "clicked", G_CALLBACK(on_button_setting_integer_up_pressed), this);
	g_signal_connect(G_OBJECT(l_vWidget[2]), "clicked", G_CALLBACK(on_button_setting_integer_down_pressed), this);

	CString l_sSettingValue;
	getBox().getSettingValue(ui32Index, l_sSettingValue);
	setValue(l_sSettingValue);
}


void CIntegerSettingView::getValue(OpenViBE::CString &rValue) const
{
	rValue = CString(gtk_entry_get_text(m_pEntry));
}


void CIntegerSettingView::setValue(const OpenViBE::CString &rValue)
{
	gtk_entry_set_text(m_pEntry, rValue);
}

void CIntegerSettingView::adjustValue(int amount)
{
	char l_sValue[1024];
	int64 l_i64lValue=m_rKernelContext.getConfigurationManager().expandAsInteger(gtk_entry_get_text(m_pEntry), 0);
	l_i64lValue+=amount;
	::sprintf(l_sValue, "%lli", l_i64lValue);
	gtk_entry_set_text(m_pEntry, l_sValue);
}
