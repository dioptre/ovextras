
#include "ovdCFloatSettingView.h"
#include "../ovd_base.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBEDesigner;
using namespace OpenViBEDesigner::Setting;

static void on_button_setting_float_up_pressed(::GtkButton* pButton, gpointer pUserData)
{
	static_cast<CFloatSettingView *>(pUserData)->adjustValue(1.0);
}

static void on_button_setting_float_down_pressed(::GtkButton* pButton, gpointer pUserData)
{
	static_cast<CFloatSettingView *>(pUserData)->adjustValue(-1.0);
}

static void on_change(::GtkEntry *entry, gpointer pUserData)
{
	static_cast<CFloatSettingView *>(pUserData)->onChange();
}


CFloatSettingView::CFloatSettingView(OpenViBE::Kernel::IBox &rBox, OpenViBE::uint32 ui32Index, CString &rBuilderName, const Kernel::IKernelContext &rKernelContext):
	CAbstractSettingView(rBox, ui32Index, rBuilderName), m_rKernelContext(rKernelContext)
{
	setSettingWidgetName("settings_collection-hbox_setting_float");

	generateNameWidget();

	::GtkWidget* l_pSettingWidget=generateEntryWidget();

	std::vector< ::GtkWidget* > l_vWidget;
	extractWidget(l_pSettingWidget, l_vWidget);
	m_pEntry = GTK_ENTRY(l_vWidget[0]);

	g_signal_connect(G_OBJECT(m_pEntry), "changed", G_CALLBACK(on_change), this);

	g_signal_connect(G_OBJECT(l_vWidget[1]), "clicked", G_CALLBACK(on_button_setting_float_up_pressed), this);
	g_signal_connect(G_OBJECT(l_vWidget[2]), "clicked", G_CALLBACK(on_button_setting_float_down_pressed), this);

	initializeValue();
}


void CFloatSettingView::getValue(OpenViBE::CString &rValue) const
{
	rValue = CString(gtk_entry_get_text(m_pEntry));
}


void CFloatSettingView::setValue(const OpenViBE::CString &rValue)
{
	gtk_entry_set_text(m_pEntry, rValue);
}

void CFloatSettingView::adjustValue(float64 amount)
{
	char l_sValue[1024];
	float64 l_f64lValue=m_rKernelContext.getConfigurationManager().expandAsFloat(gtk_entry_get_text(m_pEntry), 0);
	l_f64lValue+=amount;
	::sprintf(l_sValue, "%lf", l_f64lValue);

	getBox().setSettingValue(getSettingsIndex(), l_sValue);
}
void CFloatSettingView::onChange()
{
	const gchar* l_sValue = gtk_entry_get_text(m_pEntry);
	getBox().setSettingValue(getSettingsIndex(), l_sValue);
}
