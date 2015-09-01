#if defined(TARGET_HAS_ThirdPartyEEGOAPI)

#if defined TARGET_OS_Windows

#include <bitset>
#include <sstream>

#include "ovasCConfigurationEEGO.h"

using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;


// Function to set a predefined string in the combobox.
// Copied from ovasCConfigurationBuilder. Seems to be OK, albeit it is strange to have the code duplication,
// but other amplifier drivers do it too. 
// Would be nicer if the method would move to some utilities provider.
static void gtk_combo_box_set_active_text(::GtkComboBox* pComboBox, const gchar* sActiveText)
{
	::GtkTreeModel* l_pTreeModel=gtk_combo_box_get_model(pComboBox);
	::GtkTreeIter itComboEntry;
	int l_iIndex=0;
	gchar* l_sComboEntryName=NULL;
	if(gtk_tree_model_get_iter_first(l_pTreeModel, &itComboEntry))
	{
		do
		{
			gtk_tree_model_get(l_pTreeModel, &itComboEntry, 0, &l_sComboEntryName, -1);
			if(string(l_sComboEntryName)==string(sActiveText))
			{
				gtk_combo_box_set_active(pComboBox, l_iIndex);
				return;
			}
			else
			{
				l_iIndex++;
			}
		}
		while(gtk_tree_model_iter_next(l_pTreeModel, &itComboEntry));
	}
}

// If you added more reference attribute, initialize them here
CConfigurationEEGO::CConfigurationEEGO(
		IDriverContext& rDriverContext, 
		const char* sGtkBuilderFileName, 
		OpenViBEAcquisitionServer::CHeaderEEGO& rEEGOHeader)
	:CConfigurationBuilder(sGtkBuilderFileName)
	,m_rDriverContext(rDriverContext)
	,m_pEEGRangeComboBox(NULL)
	,m_pBIPRangeComboBox(NULL)
	,m_pBIPEntryMask(NULL)
	,m_pEEGEntryMask(NULL)
	,m_pNumChannelEntry(NULL)
	,m_rEEGOHeader(rEEGOHeader)
{
}



OpenViBE::boolean CConfigurationEEGO::preConfigure(void)
{
	if(!CConfigurationBuilder::preConfigure())
	{
		return false;
	}

	// Insert here the pre-configure code.
	// For example, you may want to check if a device is currently connected
	// and if more than one are connected. Then you can list in a dedicated combo-box 
	// the device currently connected so the user can choose which one he wants to acquire from.
	// Steffen Heimes: This is actually a TODO: ^^^^^^^^^^^

	// But currently we only get the pointers to our custom interface elements.
	GtkWidget* l_pWidget; // Temporay place of widgets before downcast to the real object holder;
	l_pWidget = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_signal_range_eeg"));
	m_pEEGRangeComboBox = GTK_COMBO_BOX(l_pWidget);
	
	l_pWidget = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_signal_range_bip"));
	m_pBIPRangeComboBox = GTK_COMBO_BOX(l_pWidget);
	
	l_pWidget = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface, "entry_eeg_mask"));
	m_pEEGEntryMask = GTK_ENTRY(l_pWidget);
	g_signal_connect(l_pWidget, "changed", G_CALLBACK(update_channel_num_cb), this);
			
	l_pWidget = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface, "entry_bip_mask"));
	m_pBIPEntryMask = GTK_ENTRY(l_pWidget);
	g_signal_connect(l_pWidget, "changed", G_CALLBACK(update_channel_num_cb), this);

	l_pWidget = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface, "entry_num_channels"));
	m_pNumChannelEntry = GTK_ENTRY(l_pWidget);

	if( !m_pEEGRangeComboBox )
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not connect to range widget \n";
	}
	else
	{
		if(m_rEEGOHeader.isEEGRangeSet())
		{
			char l_sEEGRange[1024];
			sprintf(l_sEEGRange, "%u", m_rEEGOHeader.getEEGRange());
			gtk_combo_box_set_active_text(m_pEEGRangeComboBox, l_sEEGRange);
		}
		else
		{
			gtk_combo_box_set_active(
				m_pEEGRangeComboBox,
				0);
		}

		if(m_rEEGOHeader.isBIPRangeSet())
		{
			char l_sBIPRange[1024];
			sprintf(l_sBIPRange, "%u", (int)m_rEEGOHeader.getBIPRange());
			gtk_combo_box_set_active_text(m_pBIPRangeComboBox, l_sBIPRange);
		}
		else
		{
			gtk_combo_box_set_active(
				m_pBIPRangeComboBox,
				0);
		}
	}

	if (m_rEEGOHeader.isBIPMaskSet())
	{
		gtk_entry_set_text(m_pBIPEntryMask, m_rEEGOHeader.getBIPMask());
	}

	if (m_rEEGOHeader.isEEGMaskSet())
	{
		gtk_entry_set_text(m_pEEGEntryMask, m_rEEGOHeader.getEEGMask());
	}

	return true;
}

OpenViBE::boolean CConfigurationEEGO::postConfigure(void)
{
	if(m_bApplyConfiguration)
	{
		const gchar* l_sRangeEEG=gtk_combo_box_get_active_text(m_pEEGRangeComboBox);
		const gchar* l_sRangeBIP=gtk_combo_box_get_active_text(m_pBIPRangeComboBox);
		const gchar* l_sMaskBIP=gtk_entry_get_text(m_pBIPEntryMask);
		const gchar* l_sMaskEEG=gtk_entry_get_text(m_pEEGEntryMask);

		m_rEEGOHeader.setBIPMask(l_sMaskBIP);
		m_rEEGOHeader.setEEGMask(l_sMaskEEG);
		m_rEEGOHeader.setEEGRange(l_sRangeEEG?atoi(l_sRangeEEG):0);
		m_rEEGOHeader.setBIPRange(l_sRangeBIP?atoi(l_sRangeBIP):0);
	}
	
	if(! CConfigurationBuilder::postConfigure()) // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are released
	{
		return false;
	}

	// get sum of max active channels. It would be a good idea to use the amplifier connected as a source of the maximum of available channels.
	const OpenViBE::uint64 l_i64MaskBIP=m_rEEGOHeader.getBIPMaskInt();
	const OpenViBE::uint64 l_i64MaskEEG=m_rEEGOHeader.getEEGMaskInt();

	const std::bitset<64> l_oBitsetEEG(l_i64MaskEEG);
	const std::bitset<24> l_oBitsetBIP(l_i64MaskBIP);
	
	m_pHeader->setChannelCount(l_oBitsetEEG.count()+l_oBitsetBIP.count()+2); // Plus status channels: trigger and sample counter
	
	return true;
}

/// GTK Callbacks
/* static */
void CConfigurationEEGO::update_channel_num_cb(GtkWidget *widget, CConfigurationEEGO* pThis)
{
	// get the values
	const gchar* l_sMaskBIP=gtk_entry_get_text(pThis->m_pBIPEntryMask);
	const gchar* l_sMaskEEG=gtk_entry_get_text(pThis->m_pEEGEntryMask);

	OpenViBE::uint64 l_i64MaskBIP;
	OpenViBE::uint64 l_i64MaskEEG;

	if(!CHeaderEEGO::convertMask(l_sMaskBIP, l_i64MaskBIP))
	{
		// TODO set error flag
	}

	if(!CHeaderEEGO::convertMask(l_sMaskEEG, l_i64MaskEEG))
	{
		// TODO set error flag
	}

	const std::bitset<64> l_oBitsetEEG(l_i64MaskEEG);
	const std::bitset<24> l_oBitsetBIP(l_i64MaskBIP);
	
	// format them
	std::stringstream l_ss;

	l_ss << l_oBitsetEEG.count() << " + " << l_oBitsetBIP.count() << " + 2; (EEG + BIP + STATUS)";
	// set text
	gtk_entry_set_text(pThis->m_pNumChannelEntry, l_ss.str().c_str());

	const OpenViBE::uint32 l_oNumChannels = l_oBitsetEEG.count()+l_oBitsetBIP.count()+2;
	pThis->m_pHeader->setChannelCount(l_oNumChannels);
	
	// Workaround! The current channel number is not derived from the channel count. It is retrieved from the /here hidden/ 
	// widget when the channel editing window is opening. Thus we have to set the value there too.
	if(GTK_SPIN_BUTTON(pThis->m_pNumberOfChannels))
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(pThis->m_pNumberOfChannels), l_oNumChannels);
	}
}


#endif // TARGET_OS_Windows

#endif
