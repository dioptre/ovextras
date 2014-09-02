#include "ovdCEnumerationSettingView.h"
#include "../ovd_base.h"

#include <iostream>
#include <map>

using namespace OpenViBE;
using namespace OpenViBEDesigner;
using namespace OpenViBEDesigner::Setting;


CEnumerationSettingView::CEnumerationSettingView(OpenViBE::Kernel::IBox &rBox, OpenViBE::uint32 ui32Index,
												 CString &rBuilderName, const Kernel::IKernelContext &rKernelContext,
												 const OpenViBE::CIdentifier &rTypeIdentifier):
	CAbstractSettingView(rBox, ui32Index, rBuilderName), m_oTypeIdentifier(rTypeIdentifier), m_rKernelContext(rKernelContext)
{
	setSettingWidgetName("settings_collection-comboboxentry_setting_enumeration");

	generateNameWidget();

	::GtkWidget* l_pSettingWidget=generateEntryWidget();

	m_pComboBox = GTK_COMBO_BOX(l_pSettingWidget);

	initializeValue();
}


void CEnumerationSettingView::getValue(OpenViBE::CString &rValue) const
{
	rValue = CString(gtk_combo_box_get_active_text(m_pComboBox));
}


void CEnumerationSettingView::setValue(const OpenViBE::CString &rValue)
{
	::GtkTreeIter l_oListIter;
	::GtkListStore* l_pList=GTK_LIST_STORE(gtk_combo_box_get_model(m_pComboBox));
	uint64 l_ui64Value=m_rKernelContext.getTypeManager().getEnumerationEntryValueFromName(m_oTypeIdentifier, rValue);
	uint64 i;

#if 0
	if(rTypeIdentifier==OV_TypeId_Stimulation)
	{
#endif
		std::map < CString, uint64 > l_mListEntries;
		std::map < CString, uint64 >::const_iterator it;

		for(i=0; i<m_rKernelContext.getTypeManager().getEnumerationEntryCount(m_oTypeIdentifier); i++)
		{
			CString l_sEntryName;
			uint64 l_ui64EntryValue;
			if(m_rKernelContext.getTypeManager().getEnumerationEntry(m_oTypeIdentifier, i, l_sEntryName, l_ui64EntryValue))
			{
				l_mListEntries[l_sEntryName]=l_ui64EntryValue;
			}
		}

		gtk_combo_box_set_wrap_width(m_pComboBox, 0);
		gtk_list_store_clear(l_pList);
		for(i=0, it=l_mListEntries.begin(); it!=l_mListEntries.end(); it++, i++)
		{
			gtk_list_store_append(l_pList, &l_oListIter);
			gtk_list_store_set(l_pList, &l_oListIter, 0, it->first.toASCIIString(), -1);

			if(l_ui64Value==it->second)
			{
				gtk_combo_box_set_active(m_pComboBox, (gint)i);
			}
		}
#if 0
	}
	else
	{
		gtk_list_store_clear(l_pList);
		for(i=0; i<m_rKernelContext.getTypeManager().getEnumerationEntryCount(rTypeIdentifier); i++)
		{
			CString l_sEntryName;
			uint64 l_ui64EntryValue;
			if(m_rKernelContext.getTypeManager().getEnumerationEntry(rTypeIdentifier, i, l_sEntryName, l_ui64EntryValue))
			{
				gtk_list_store_append(l_pList, &l_oListIter);
				gtk_list_store_set(l_pList, &l_oListIter, 0, l_sEntryName.toASCIIString(), -1);

				if(l_ui64Value==l_ui64EntryValue)
				{
					gtk_combo_box_set_active(l_pWidget, (gint)i);
				}
			}
		}
	}
#endif
	if(gtk_combo_box_get_active(m_pComboBox)==-1)
	{
		gtk_list_store_append(l_pList, &l_oListIter);
		gtk_list_store_set(l_pList, &l_oListIter, 0, rValue.toASCIIString(), -1);
		gtk_combo_box_set_active(m_pComboBox, (gint)i); // $$$ i should be ok :)
	}
}

