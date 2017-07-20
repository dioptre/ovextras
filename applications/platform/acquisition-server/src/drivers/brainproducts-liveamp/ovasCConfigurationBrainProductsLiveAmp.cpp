/*
 * Brain Products LiveAmp driver for OpenViBE
 * Copyright (C) 2017 Brain Products - Author : Ratko Petrovic
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */
/*********************************************************************
* History
* [2017-03-29] ver 1.0									          - RP
* [2017-05-02] ver 1.1  Due to support for LiveAmp 8 and 16 channels, 
*                       a new variable "m_rCountBip" added        - RP
*
*********************************************************************/

#if defined TARGET_HAS_ThirdPartyLiveAmpAPI

#include "ovasCConfigurationBrainProductsLiveAmp.h"
#include <Amplifier_LIB.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;

/*_________________________________________________

Insert callback to specific widget here
Example with a button that launch a calibration of the device:

//Callback connected to a dedicated gtk button:
static void button_calibrate_pressed_cb(::GtkButton* pButton, void* pUserData)
{
	CConfigurationBrainProductsLiveAmp* l_pConfig=static_cast<CConfigurationBrainProductsLiveAmp*>(pUserData);
	l_pConfig->buttonCalibratePressedCB();
}
_________________________________________________*/

// If you added more reference attribute, initialize them here
CConfigurationBrainProductsLiveAmp::CConfigurationBrainProductsLiveAmp(
	CDriverBrainProductsLiveAmp& rDriverContext, 
	const char* sGtkBuilderFileName,	
	uint32& rPhysicalSampleRate,
	uint32& rCountEEG,
	uint32& rCountBip,
	uint32& rCountAUX,
	uint32& rCountACC,
	boolean& rUseAccChannels,
	uint32& rGoodImpedanceLimit,
	uint32& rBadImpedanceLimit,
	std::string& sSerialNumber,
	boolean& rUseBipolarChannels)
	:m_rDriverContext(rDriverContext)
	,CConfigurationBuilder(sGtkBuilderFileName)
	,m_rPhysicalSampleRate(rPhysicalSampleRate)
	,m_rCountEEG(rCountEEG)
	,m_rCountBip(rCountBip)
	,m_rCountAUX(rCountAUX)
	,m_rCountACC(rCountACC)
	,m_rUseAccChannels(rUseAccChannels)
	,m_rGoodImpedanceLimit(rGoodImpedanceLimit)
	,m_rBadImpedanceLimit(rBadImpedanceLimit)
	,m_rNumberEEGChannels(rCountEEG)
	,m_rNumberAUXChannels(rCountAUX)
	,m_rTriggers(rBadImpedanceLimit)
	,m_sSerialNumber(sSerialNumber) 
	,m_rUseBipolarChannels(rUseBipolarChannels)
{
}

boolean CConfigurationBrainProductsLiveAmp::preConfigure(void)
{
	if(! CConfigurationBuilder::preConfigure())
	{
		return false;
	}

	m_tSerialNumber = GTK_ENTRY (::gtk_builder_get_object(m_pBuilderConfigureInterface, "entrySerialNr"));
	m_pComboBoxPhysicalSampleRate=GTK_COMBO_BOX(::gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_sampling_frequency"));

	m_pNumberEEGChannels = GTK_SPIN_BUTTON(::gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_number_of_channels"));
	m_pNumberBipolar = GTK_SPIN_BUTTON(::gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_number_of_bipolar"));
	m_pNumberAUXChannels = GTK_SPIN_BUTTON(::gtk_builder_get_object(m_pBuilderConfigureInterface, "spbBtnAUXchn"));
	
	m_pEnableACCChannels = GTK_TOGGLE_BUTTON(::gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_acc"));
	m_pUseBipolarChannels = GTK_TOGGLE_BUTTON(::gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_use_bipolar_channels"));

	if (gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_impedance"))
		m_pImpedanceCheck = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_impedance"));

	m_pSpinButtonGoodImpedanceLimit = GTK_SPIN_BUTTON(::gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_good_imp"));
	m_pSpinButtonBadImpedanceLimit  = GTK_SPIN_BUTTON(::gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_bad_imp"));
	
	// Connect here all callbacks
	// Example:
	// g_signal_connect(gtk_builder_get_object(m_pBuilderConfigureInterface, "button_calibrate"), "pressed", G_CALLBACK(button_calibrate_pressed_cb), this);

	// Insert here the pre-configure code.
	// For example, you may want to check if a device is currently connected
	// and if more than one are connected. Then you can list in a dedicated combo-box 
	// the device currently connected so the user can choose which one he wants to acquire from.
	::gtk_entry_set_text(m_tSerialNumber, m_sSerialNumber.c_str());	
	::gtk_combo_box_set_active(m_pComboBoxPhysicalSampleRate, m_rPhysicalSampleRate);
	::gtk_spin_button_set_value(m_pNumberEEGChannels, m_rCountEEG);
	::gtk_spin_button_set_value(m_pNumberBipolar, m_rCountBip);
	::gtk_spin_button_set_value(m_pNumberAUXChannels, m_rCountAUX);
	::gtk_spin_button_set_value(m_pSpinButtonGoodImpedanceLimit, m_rGoodImpedanceLimit);
	::gtk_spin_button_set_value(m_pSpinButtonBadImpedanceLimit, m_rBadImpedanceLimit);
	::gtk_toggle_button_set_active(m_pEnableACCChannels, m_rUseAccChannels);
	
	return true;
}

boolean CConfigurationBrainProductsLiveAmp::postConfigure(void)
{
	if(m_bApplyConfiguration)
	{
		// If the user pressed the "apply" button, you need to save the changes made in the configuration.
		// For example, you can save the connection ID of the selected device:
		// m_ui32ConnectionID = <value-from-gtk-widget>
		gtk_spin_button_update(GTK_SPIN_BUTTON(m_pSpinButtonGoodImpedanceLimit));
		gtk_spin_button_update(GTK_SPIN_BUTTON(m_pSpinButtonGoodImpedanceLimit));
		gtk_spin_button_update(GTK_SPIN_BUTTON(m_pNumberEEGChannels));
		gtk_spin_button_update(GTK_SPIN_BUTTON(m_pNumberBipolar));
		gtk_spin_button_update(GTK_SPIN_BUTTON(m_pNumberAUXChannels));
		
		const char*  serialNumber = ::gtk_entry_get_text (m_tSerialNumber);
		m_sSerialNumber.assign(serialNumber, strlen(serialNumber));

		
		m_rPhysicalSampleRate=::gtk_combo_box_get_active(m_pComboBoxPhysicalSampleRate);
		::GtkTreeModel* l_pWidgetResolutionFull = ::gtk_combo_box_get_model (m_pComboBoxPhysicalSampleRate);

		m_rCountEEG = uint32(::gtk_spin_button_get_value(m_pNumberEEGChannels));
		m_rCountBip = uint32(::gtk_spin_button_get_value(m_pNumberBipolar));
		m_rCountAUX = uint32(::gtk_spin_button_get_value(m_pNumberAUXChannels));
		
		m_rUseAccChannels = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_pEnableACCChannels)) != 0;

		if(m_rUseAccChannels)
			m_rCountACC = 3;  // can be 3 or 6
		else
			m_rCountACC = 0;
		
		m_rGoodImpedanceLimit = uint32(::gtk_spin_button_get_value(m_pSpinButtonGoodImpedanceLimit));
		m_rBadImpedanceLimit  = uint32(::gtk_spin_button_get_value(m_pSpinButtonBadImpedanceLimit));		
	}

	if(! CConfigurationBuilder::postConfigure()) // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed
	{
		return false;
	}

	return true;
}


#endif // TARGET_HAS_ThirdPartyLiveAmpAPI