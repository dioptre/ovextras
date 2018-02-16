#include "ovasCConfigurationDriverSimulatedDeviator.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;

CConfigurationDriverSimulatedDeviator::CConfigurationDriverSimulatedDeviator(IDriverContext& rDriverContext, const char* sGtkBuilderFileName
	,boolean& rSendPeriodicStimulations
	,float64& rOffset
	,float64& rSpread
	,float64& rMaxDev
	,float64& rPullback
	,float64& rUpdate
	,uint64& rWavetype
	,float64& rFreezeFrequency
	,float64& rFreezeDuration
	
	)
	:CConfigurationBuilder(sGtkBuilderFileName)
	 ,m_rDriverContext(rDriverContext)
	 ,m_rSendPeriodicStimulations(rSendPeriodicStimulations)
	 ,m_Offset(rOffset)
	 ,m_Spread(rSpread)
	 ,m_MaxDev(rMaxDev)
	 ,m_Pullback(rPullback)
	 ,m_Update(rUpdate)
	 ,m_Wavetype(rWavetype)
	 ,m_FreezeFrequency(rFreezeFrequency)
	 ,m_FreezeDuration(rFreezeDuration)

{
}

boolean CConfigurationDriverSimulatedDeviator::preConfigure(void)
{
	if (!CConfigurationBuilder::preConfigure())
	{
		return false;
	}

	::GtkToggleButton* l_pToggleSendPeriodicStimulations = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_send_periodic_stimulations"));

	gtk_toggle_button_set_active(l_pToggleSendPeriodicStimulations, m_rSendPeriodicStimulations);

	::GtkSpinButton* tmp;
	
	tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_offset"));
	gtk_spin_button_set_digits(tmp,2);
	gtk_spin_button_set_value(tmp, m_Offset);

	tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_spread"));
	gtk_spin_button_set_digits(tmp,3);
	gtk_spin_button_set_value(tmp, m_Spread);

	tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_maxdev"));
	gtk_spin_button_set_digits(tmp,3);
	gtk_spin_button_set_value(tmp, m_MaxDev);

	tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_pullback"));
	gtk_spin_button_set_digits(tmp,3);
	gtk_spin_button_set_value(tmp, m_Pullback);

	tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_update"));
	gtk_spin_button_set_digits(tmp,3);
	gtk_spin_button_set_value(tmp, m_Update);

	GtkComboBox *wavetype = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_wavetype"));
	gtk_combo_box_set_active(wavetype,static_cast<gint>(m_Wavetype) );

	tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_freeze_frequency"));
	gtk_spin_button_set_digits(tmp,3);
	gtk_spin_button_set_value(tmp, m_FreezeFrequency);

	tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_freeze_duration"));
	gtk_spin_button_set_digits(tmp,3);
	gtk_spin_button_set_value(tmp, m_FreezeDuration);

	return true;
}

boolean CConfigurationDriverSimulatedDeviator::postConfigure(void)
{
	if (m_bApplyConfiguration)
	{
		::GtkToggleButton* l_pToggleSendPeriodicStimulations = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_send_periodic_stimulations"));

		m_rSendPeriodicStimulations = (::gtk_toggle_button_get_active(l_pToggleSendPeriodicStimulations)>0);

		::GtkSpinButton* tmp;	
	
		tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_offset"));
		gtk_spin_button_update(tmp);
		m_Offset = gtk_spin_button_get_value(tmp);
		tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_spread"));
		gtk_spin_button_update(tmp);
		m_Spread = gtk_spin_button_get_value(tmp);
		tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_maxdev"));
		gtk_spin_button_update(tmp);
		m_MaxDev = gtk_spin_button_get_value(tmp);
		tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_pullback"));
		gtk_spin_button_update(tmp);
		m_Pullback = gtk_spin_button_get_value(tmp);
		tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_update"));
		gtk_spin_button_update(tmp);
		m_Update = gtk_spin_button_get_value(tmp);

		GtkComboBox *wavetype = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_wavetype"));
		m_Wavetype = static_cast<uint64>( gtk_combo_box_get_active(wavetype) );
		
		tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_freeze_frequency"));
		gtk_spin_button_update(tmp);
		m_FreezeFrequency = gtk_spin_button_get_value(tmp);

		tmp = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_freeze_duration"));
		gtk_spin_button_update(tmp);
		m_FreezeDuration = gtk_spin_button_get_value(tmp);

	}

	if (!CConfigurationBuilder::postConfigure())
	{
		return false;
	}

	return true;
}

