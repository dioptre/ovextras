#include "ovdCStringSettingView.h"
#include "../ovd_base.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBEDesigner;
using namespace OpenViBEDesigner::Setting;

CStringSettingView::CStringSettingView(OpenViBE::Kernel::IBox &rBox, OpenViBE::uint32 ui32Index, CString &rBuilderName):
	CAbstractSettingView(rBox, ui32Index, rBuilderName)
{
	setSettingWidgetName("settings_collection-entry_setting_string");

	generateNameWidget();
	::GtkWidget* l_pSettingWidget=generateEntryWidget();

	m_pEntry = GTK_ENTRY(l_pSettingWidget);

	initializeValue();
}


void CStringSettingView::getValue(OpenViBE::CString &rValue) const
{
	rValue = CString(gtk_entry_get_text(m_pEntry));
}


void CStringSettingView::setValue(const OpenViBE::CString &rValue)
{
	gtk_entry_set_text(m_pEntry, rValue);
}
