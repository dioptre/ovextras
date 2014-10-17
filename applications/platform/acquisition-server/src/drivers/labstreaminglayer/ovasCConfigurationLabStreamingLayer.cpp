
#if defined(TARGET_HAS_ThirdPartyLSL)

#include "ovasCConfigurationLabStreamingLayer.h"

#include <lsl_cpp.h>
#include "ovasIHeader.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;


CConfigurationLabStreamingLayer::CConfigurationLabStreamingLayer(IDriverContext& rDriverContext, const char* sGtkBuilderFileName, 
	IHeader& rHeader,
	CString& rStream,
	CString& rMarkerStream)
	:CConfigurationBuilder(sGtkBuilderFileName)
	,m_rDriverContext(rDriverContext)
	,m_rHeader(rHeader)
	,m_rStream(rStream)
	,m_rMarkerStream(rMarkerStream)
{
}

boolean CConfigurationLabStreamingLayer::preConfigure(void)
{
	if(! CConfigurationBuilder::preConfigure())
	{
		return false;
	}

	::GtkComboBox* l_pComboBox=GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_signal_stream"));
	if(!l_pComboBox)
	{
		return false;
	}

	::GtkComboBox* l_pMarkerComboBox=GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_marker_stream"));
	if(!l_pMarkerComboBox)
	{
		return false;
	}

	std::vector<lsl::stream_info> l_vStreams = lsl::resolve_streams(1.0);

	// See if any of the streams can be interpreted as signal or marker
	uint32 l_ui32nStreams = 0;
	uint32 l_ui32nMarkerStreams = 0;
	for(uint32 i=0; i<l_vStreams.size(); i++)
	{
		if(l_vStreams[i].channel_format() == lsl::cf_float32)
		{	
			// std::cout << "Signal " << m_vStreams[i].name().c_str() << "\n";
			::gtk_combo_box_append_text(l_pComboBox, l_vStreams[i].name().c_str());
			if(m_rStream==CString(l_vStreams[i].name().c_str()) || !l_ui32nStreams)
			{
				::gtk_combo_box_set_active(l_pComboBox,l_ui32nStreams);
			}		
			l_ui32nStreams++;
		}
		else if(l_vStreams[i].channel_format() == lsl::cf_int32)
		{
			// std::cout << "Marker " << m_vStreams[i].name().c_str() << "\n";
			::gtk_combo_box_append_text(l_pMarkerComboBox, l_vStreams[i].name().c_str());
			if(m_rMarkerStream==CString(l_vStreams[i].name().c_str()) || !l_ui32nMarkerStreams)
			{
				::gtk_combo_box_set_active(l_pMarkerComboBox,l_ui32nMarkerStreams);
			}
			l_ui32nMarkerStreams++;
		} 
		else 
		{
			// Only float32 and int32 are currently supported for signals and markers respectively
			m_rDriverContext.getLogManager() << LogLevel_Debug << "Channel format " << l_vStreams[i].channel_format() << " of stream [" << l_vStreams[i].name().c_str() << "] is not supported, skipped.\n";

			continue;
		}
	}

	return true;
}

boolean CConfigurationLabStreamingLayer::postConfigure(void)
{
	if(m_bApplyConfiguration)
	{
		::GtkComboBox* l_pComboBox=GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_signal_stream"));
		if(!l_pComboBox)
		{
			return false;
		}
		m_rStream = gtk_combo_box_get_active_text(l_pComboBox);

		::GtkComboBox* l_pMarkerComboBox=GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_marker_stream"));
		if(!l_pMarkerComboBox)
		{
			return false;
		}

		m_rMarkerStream = gtk_combo_box_get_active_text(l_pMarkerComboBox);
	}

	if(! CConfigurationBuilder::postConfigure()) // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed
	{
		return false;
	}

	return true;
}

#endif
