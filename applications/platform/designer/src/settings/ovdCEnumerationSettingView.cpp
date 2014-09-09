#include "ovdCEnumerationSettingView.h"
#include "../ovd_base.h"

#include <iostream>
#include <map>

using namespace OpenViBE;
using namespace OpenViBEDesigner;
using namespace OpenViBEDesigner::Setting;

static void on_change(::GtkEntry *entry, gpointer pUserData)
{
	static_cast<CEnumerationSettingView *>(pUserData)->onChange();
}

CEnumerationSettingView::CEnumerationSettingView(OpenViBE::Kernel::IBox &rBox, OpenViBE::uint32 ui32Index,
												 CString &rBuilderName, const Kernel::IKernelContext &rKernelContext,
												 const OpenViBE::CIdentifier &rTypeIdentifier):
	CAbstractSettingView(rBox, ui32Index, rBuilderName), m_oTypeIdentifier(rTypeIdentifier), m_rKernelContext(rKernelContext)
{
	p=false;
	setSettingWidgetName("settings_collection-comboboxentry_setting_enumeration");

	generateNameWidget();

	::GtkWidget* l_pSettingWidget=generateEntryWidget();

	m_pComboBox = GTK_COMBO_BOX(l_pSettingWidget);

	::GtkTreeIter l_oListIter;
	::GtkListStore* l_pList=GTK_LIST_STORE(gtk_combo_box_get_model(m_pComboBox));
	gtk_combo_box_set_wrap_width(m_pComboBox, 0);
	gtk_list_store_clear(l_pList);

	for(uint64 i=0; i<m_rKernelContext.getTypeManager().getEnumerationEntryCount(m_oTypeIdentifier); i++)
	{
		CString l_sEntryName;
		uint64 l_ui64EntryValue;
		if(m_rKernelContext.getTypeManager().getEnumerationEntry(m_oTypeIdentifier, i, l_sEntryName, l_ui64EntryValue))
		{
			gtk_list_store_append(l_pList, &l_oListIter);
			gtk_list_store_set(l_pList, &l_oListIter, 0, l_sEntryName.toASCIIString(), -1);

			m_mEntriesIndex[l_sEntryName] = i;
		}
	}

	initializeValue();

	g_signal_connect(G_OBJECT(m_pComboBox), "changed", G_CALLBACK(on_change), this);
}


void CEnumerationSettingView::getValue(OpenViBE::CString &rValue) const
{
	rValue = CString(gtk_combo_box_get_active_text(m_pComboBox));
}


void CEnumerationSettingView::setValue(const OpenViBE::CString &rValue)
{
	gtk_combo_box_set_active(m_pComboBox, (gint)m_mEntriesIndex[rValue]);
}

void CEnumerationSettingView::onChange()
{
	const gchar* l_sValue = gtk_combo_box_get_active_text(m_pComboBox);
	getBox().setSettingValue(getSettingsIndex(), l_sValue);
}

