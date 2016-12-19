/*
* NeuroServo driver for OpenViBE
*
* \author (NeuroServo)
* \date Wed Nov 23 00:24:00 2016
*
*/

#if defined TARGET_OS_Windows
#if defined TARGET_HAS_ThirdPartyNeuroServo

#include "ovasCConfigurationNeuroServoHid.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;


// Automatic Shutdown callback
static void radio_automatic_shutdown_cb(GtkToggleButton* pButton, CConfigurationNeuroServoHid* pConfigurationNeuroServoHid)
{
	pConfigurationNeuroServoHid->checkRadioAutomaticShutdown(gtk_toggle_button_get_active(pButton) == 1);
}

// Shutdown on driver disconnect callback
static void radio_shutdown_on_driver_disconnect_cb(GtkToggleButton* pButton, CConfigurationNeuroServoHid* pConfigurationNeuroServoHid)
{
	pConfigurationNeuroServoHid->checkRadioShutdownOnDriverDisconnect(gtk_toggle_button_get_active(pButton) == 1);
}

// Device Light Enable  callback
static void radio_device_light_enable_cb(GtkToggleButton* pButton, CConfigurationNeuroServoHid* pConfigurationNeuroServoHid)
{
	pConfigurationNeuroServoHid->checkRadioDeviceLightEnable(gtk_toggle_button_get_active(pButton) == 1);
}

// If you added more reference attribute, initialize them here
CConfigurationNeuroServoHid::CConfigurationNeuroServoHid(IDriverContext& rDriverContext, const char* sGtkBuilderFileName)
	:CConfigurationBuilder(sGtkBuilderFileName)
	,m_rDriverContext(rDriverContext)
{
}

boolean CConfigurationNeuroServoHid::preConfigure(void)
{
	if(! CConfigurationBuilder::preConfigure())
	{
		return false;
	}

	// callbacks connection

	// Connection of "Automatic Shutdown" toggle button
	GtkToggleButton* l_pToggleButtonAutomaticShutdown = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_automatic_shutdown"));
	gtk_toggle_button_set_active(l_pToggleButtonAutomaticShutdown, m_bAutomaticShutdown ? true : false);

	g_signal_connect(::gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_automatic_shutdown"), "toggled", G_CALLBACK(radio_automatic_shutdown_cb), this);
	this->checkRadioAutomaticShutdown(m_bAutomaticShutdown);

	// Connection of "Shutdown on driver disconnect" toggle button
	GtkToggleButton* l_pToggleButtonShutdownOnDriverDisconnect = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_shutdown_on_driver_disconnect"));
	gtk_toggle_button_set_active(l_pToggleButtonShutdownOnDriverDisconnect, m_bShutdownOnDriverDisconnect ? true : false);

	g_signal_connect(::gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_shutdown_on_driver_disconnect"), "toggled", G_CALLBACK(radio_shutdown_on_driver_disconnect_cb), this);
	this->checkRadioShutdownOnDriverDisconnect(m_bShutdownOnDriverDisconnect);

	// Connection of "Device Light Enable" toggle button
	GtkToggleButton* l_pToggleButtonDeviceLightEnable = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_device_light_enable"));
	gtk_toggle_button_set_active(l_pToggleButtonDeviceLightEnable, m_bDeviceLightEnable ? true : false);

	g_signal_connect(::gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_device_light_enable"), "toggled", G_CALLBACK(radio_device_light_enable_cb), this);
	this->checkRadioDeviceLightEnable(m_bDeviceLightEnable);

	return true;
}

boolean CConfigurationNeuroServoHid::postConfigure(void)
{

	if(m_bApplyConfiguration)
	{
		// Automatic Shutdown
		GtkToggleButton* l_pToggleButtonAutomaticShutdown = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_automatic_shutdown"));
		m_bAutomaticShutdown = ::gtk_toggle_button_get_active(l_pToggleButtonAutomaticShutdown) ? true : false;

		// Shutdown on driver disconnect
		GtkToggleButton* l_pToggleButtonShutdownOnDriverDisconnect = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_shutdown_on_driver_disconnect"));
		m_bShutdownOnDriverDisconnect = ::gtk_toggle_button_get_active(l_pToggleButtonShutdownOnDriverDisconnect) ? true : false;

		// Device Light Enable
		GtkToggleButton* l_pToggleButtonDeviceLightEnable = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_device_light_enable"));
		m_bDeviceLightEnable = ::gtk_toggle_button_get_active(l_pToggleButtonDeviceLightEnable) ? true : false;
	}

	if(! CConfigurationBuilder::postConfigure())
	{
		return false;
	}

	return true;
}

//___________________________________________________________________//
//                                                                   //
// NEUROSERVO SPECIFIC METHODS IMPLEMENTATION

// Automatic Shutdown
void CConfigurationNeuroServoHid::checkRadioAutomaticShutdown(const OpenViBE::boolean bAutoShutdownStatus)
{
	m_bAutomaticShutdown = bAutoShutdownStatus;
}

OpenViBE::boolean CConfigurationNeuroServoHid::getAutomaticShutdownStatus()
{
	return m_bAutomaticShutdown;
}

void CConfigurationNeuroServoHid::setRadioAutomaticShutdown(OpenViBE::boolean state)
{
	m_bAutomaticShutdown = state;
}

// Shutdown on driver disconnect
void CConfigurationNeuroServoHid::checkRadioShutdownOnDriverDisconnect(const OpenViBE::boolean bShutdownOnDriverDisconnectStatus)
{
	m_bShutdownOnDriverDisconnect = bShutdownOnDriverDisconnectStatus;
}

OpenViBE::boolean CConfigurationNeuroServoHid::getShutdownOnDriverDisconnectStatus()
{
	return m_bShutdownOnDriverDisconnect;
}

void CConfigurationNeuroServoHid::setRadioShutdownOnDriverDisconnect(OpenViBE::boolean state)
{
	m_bShutdownOnDriverDisconnect = state;
}

// Device Light Enable
void CConfigurationNeuroServoHid::checkRadioDeviceLightEnable(const OpenViBE::boolean bDeviceLightEnableStatus)
{
	m_bDeviceLightEnable = bDeviceLightEnableStatus;
}

OpenViBE::boolean CConfigurationNeuroServoHid::getDeviceLightEnableStatus()
{
	return m_bDeviceLightEnable;
}

void CConfigurationNeuroServoHid::setRadioDeviceLightEnable(OpenViBE::boolean state)
{
	m_bDeviceLightEnable = state;
}

#endif
#endif // TARGET_OS_Windows
