#include "ovdCBitMaskSettingView.h"
#include "../ovd_base.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBEDesigner;
using namespace OpenViBEDesigner::Setting;

CBitMaskSettingView::CBitMaskSettingView(OpenViBE::Kernel::IBox &rBox, OpenViBE::uint32 ui32Index,
										 CString &rBuilderName, const Kernel::IKernelContext &rKernelContext,
										 const OpenViBE::CIdentifier &rTypeIdentifier):
	CAbstractSettingView(rBox, ui32Index, rBuilderName), m_oTypeIdentifier(rTypeIdentifier), m_rKernelContext(rKernelContext)
{
	setSettingWidgetName("settings_collection-table_setting_bitmask");

	generateNameWidget();
	::GtkWidget* l_pSettingWidget=generateEntryWidget();

	gint l_iTableSize=(guint)((m_rKernelContext.getTypeManager().getBitMaskEntryCount(m_oTypeIdentifier)+1)>>1);
	::GtkTable* l_pBitMaskTable=GTK_TABLE(l_pSettingWidget);
	gtk_table_resize(l_pBitMaskTable, 2, l_iTableSize);;

	for(uint64 i=0; i<m_rKernelContext.getTypeManager().getBitMaskEntryCount(m_oTypeIdentifier); i++)
	{
		CString l_sEntryName;
		uint64 l_ui64EntryValue;
		if(m_rKernelContext.getTypeManager().getBitMaskEntry(m_oTypeIdentifier, i, l_sEntryName, l_ui64EntryValue))
		{
			::GtkWidget* l_pSettingButton=::gtk_check_button_new();
			gtk_table_attach_defaults(l_pBitMaskTable, l_pSettingButton, (guint)(i&1), (guint)((i&1)+1), (guint)(i>>1), (guint)((i>>1)+1));
			gtk_button_set_label(GTK_BUTTON(l_pSettingButton), (const char*)l_sEntryName);
			m_vToggleButton.push_back(GTK_TOGGLE_BUTTON(l_pSettingButton));
		}
	}
	gtk_widget_show_all(GTK_WIDGET(l_pBitMaskTable));

	initializeValue();
}


void CBitMaskSettingView::getValue(OpenViBE::CString &rValue) const
{
	std::string l_sResult;

	for(size_t i = 0 ; i < m_vToggleButton.size() ; ++i)
	{
		if(gtk_toggle_button_get_active(m_vToggleButton[i]))
		{
			if(l_sResult != "")
			{
				l_sResult += ':';
			}
			l_sResult += gtk_button_get_label(GTK_BUTTON(m_vToggleButton[i]));
		}
	}
	std::cout << l_sResult << std::endl;
	rValue = CString(l_sResult.c_str());
}


void CBitMaskSettingView::setValue(const OpenViBE::CString &rValue)
{
	std::string l_sValue(rValue);
	std::cout << rValue << std::endl;

	for(size_t i = 0 ; i < m_vToggleButton.size() ; ++i)
	{
		const gchar* l_sLabel = gtk_button_get_label(GTK_BUTTON(m_vToggleButton[i]));
		std::cout << l_sLabel << std::endl;
		if(l_sValue.find(l_sLabel)!=std::string::npos)
		{
			std::cout << "Find active" << std::endl;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_vToggleButton[i]), true);
		}
		else
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_vToggleButton[i]), false);
		}
	}
}
