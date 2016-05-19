
#if defined(TARGET_HAS_ThirdPartyGNEEDaccessAPI)

#include "ovasCConfigurationgNautilusInterface.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;

using namespace std;

/*_________________________________________________

Insert callback to specific widget here
Example with a button that launch a calibration of the device:
*/

//Callback connected to a dedicated gtk button (button_select_channels_bipolar_car_noise):
static void button_channel_settings_cb(GtkButton* pButton, void* pUserData)
{
	CConfigurationgNautilusInterface* l_pConfig=static_cast<CConfigurationgNautilusInterface*>(pUserData);
	l_pConfig->buttonChannelSettingsPressedCB();
}

//Callback actually called:
void CConfigurationgNautilusInterface::buttonChannelSettingsPressedCB(void)
{
	// Connect to the hardware, ask for calibration, verify the return code, etc.

	GtkWidget *l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface,"dialog_select_channels_bipolar_car_noise"));
	gint l_i32Resp = gtk_dialog_run(GTK_DIALOG(l_pDialog));

	gtk_widget_hide(l_pDialog);
}

//Callback connected to a dedicated gtk button (button_select_sensitivity_filters):
static void button_sensitivity_filters_cb(GtkButton* pButton, void* pUserData)
{
	CConfigurationgNautilusInterface* l_pConfig=static_cast<CConfigurationgNautilusInterface*>(pUserData);
	l_pConfig->buttonSensitivityFiltersPressedCB();
}

//Callback actually called:
void CConfigurationgNautilusInterface::buttonSensitivityFiltersPressedCB(void)
{
	// get bandpass and notch filters for currenty selected sampling rate and set to filter list in corresponding dialog
	GtkWidget *l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface,"dialog_sensitivity_filters"));
	gint l_i32Resp = gtk_dialog_run(GTK_DIALOG(l_pDialog));

	gtk_widget_hide(l_pDialog);
}

//Callback connected to a dedicated gtk button (button_sensitivity_filters_apply):
static void button_sensitivity_filters_apply_cb(GtkButton* pButton, void* pUserData)
{
	CConfigurationgNautilusInterface* l_pConfig=static_cast<CConfigurationgNautilusInterface*>(pUserData);
	l_pConfig->buttonSensitivityFiltersApplyPressedCB();
}

//Callback actually called:
void CConfigurationgNautilusInterface::buttonSensitivityFiltersApplyPressedCB(void)
{
	// get handle to sensitivity and filters dialog and close it
	GtkWidget *l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface,"dialog_sensitivity_filters"));
	gtk_widget_hide(l_pDialog);
}

//Callback connected to a dedicated gtk button (button_channel_apply):
static void button_channel_settings_apply_cb(GtkButton* pButton, void* pUserData)
{
	CConfigurationgNautilusInterface* l_pConfig=static_cast<CConfigurationgNautilusInterface*>(pUserData);
	l_pConfig->buttonChannelSettingsApplyPressedCB();
}

//Callback actually called:
void CConfigurationgNautilusInterface::buttonChannelSettingsApplyPressedCB(void)
{
	// get handle to channel settings dialog and close it
	GtkWidget *l_pDialog = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface,"dialog_select_channels_bipolar_car_noise"));
	gtk_widget_hide(l_pDialog);

	// get number of channels selected and set range as number of channels starting at number of channels
	unsigned __int16 l_ui16NumberOfChannels = 0;
	unsigned __int16 i;
	char l_sTemporary[30];
	for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
	{
		// set electrode names as channel names in channel selection dialog
		sprintf_s(&l_sTemporary[0],30,"checkbutton_channel_%d",(i + 1));
		GtkButton *l_pCheckButton = GTK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		if (l_pCheckButton)
		{
			if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton))) && (gtk_widget_get_visible(GTK_WIDGET(l_pCheckButton))))
			{
				l_ui16NumberOfChannels += 1;
			}
		}
	}

	GtkSpinButton *l_pSpinButton = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"spinbutton_number_of_channels"));
	gtk_spin_button_set_range(l_pSpinButton,l_ui16NumberOfChannels,l_ui16NumberOfChannels);
	gtk_spin_button_set_value(l_pSpinButton,l_ui16NumberOfChannels);
}

// catch combobox sampling rate changed signal and call function which handles event
static void sample_rate_changed_cb(GtkComboBox* pComboBox, void *pUserData)
{
	CConfigurationgNautilusInterface* l_pConfig=static_cast<CConfigurationgNautilusInterface*>(pUserData);
	l_pConfig->comboboxSampleRateChangedCB();
}

// Callback actually called
void CConfigurationgNautilusInterface::comboboxSampleRateChangedCB()
{
	// get hardware filters according to sampling frequency currently selected
	OpenViBE::boolean l_bFunctionReturn = getFiltersForNewSamplingRate();
	if (!l_bFunctionReturn)
	{
		// error logged in getFiltersForNewSamplingRate
	}
}

// catch noise reduction checkbox toggled signal and call function which handles event
static void noise_reduction_changed_cb(GtkCheckButton* pCheckbutton, void* pUserData)
{
	CConfigurationgNautilusInterface* l_pConfig = static_cast<CConfigurationgNautilusInterface*>(pUserData);
	l_pConfig->checkbuttonNoiseReductionChangedCB();
}

// Callback actually called
void CConfigurationgNautilusInterface::checkbuttonNoiseReductionChangedCB()
{
	// activate/deactivate noise reduction checkboxes in dialog_select_channels_bipolar_car_noise
	GtkCheckButton *l_pCheckButtonNoise = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"checkbutton_noise_reduction"));
	gboolean l_bCheckButtonValue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButtonNoise));
	GtkCheckButton *l_pCheckButtonNoiseChannel;
	char l_sTemporary[45];
	for (unsigned __int16 i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
	{
		sprintf_s(&l_sTemporary[0],45,"checkbutton_noise_channel_%d",(i + 1));
		l_pCheckButtonNoiseChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		
		if (l_pCheckButtonNoiseChannel)
		{
			if (gtk_widget_get_visible(GTK_WIDGET(l_pCheckButtonNoiseChannel)))
			{
				gtk_widget_set_sensitive(GTK_WIDGET(l_pCheckButtonNoiseChannel),l_bCheckButtonValue);
			}
		}
	}
}

// catch car checkbox toggled signal and call function which handles event
static void car_changed_cb(GtkCheckButton* pCheckbutton, void* pUserData)
{
	CConfigurationgNautilusInterface* l_pConfig = static_cast<CConfigurationgNautilusInterface*>(pUserData);
	l_pConfig->checkbuttonCARChangedCB();
}

// Callback actually called
void CConfigurationgNautilusInterface::checkbuttonCARChangedCB()
{
	// activate/deactivate car checkboxes in dialog_select_channels_bipolar_car_noise
	GtkCheckButton *l_pCheckButtonCAR = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"checkbutton_car"));
	gboolean l_bCheckButtonValue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButtonCAR));
	GtkCheckButton *l_pCheckButtonCarChannel;
	char l_sTemporary[45];
	for (unsigned __int16 i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
	{
		sprintf_s(&l_sTemporary[0],45,"checkbutton_car_channel_%d",(i + 1));
		l_pCheckButtonCarChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		if (l_pCheckButtonCarChannel)
		{
			if (gtk_widget_get_visible(GTK_WIDGET(l_pCheckButtonCarChannel)))
			{
				gtk_widget_set_sensitive(GTK_WIDGET(l_pCheckButtonCarChannel),l_bCheckButtonValue);
			}
		}
	}
}

// get hardware related settings from GDS
OpenViBE::boolean CConfigurationgNautilusInterface::getHardwareSettings(void)
{
	GtkTreeIter l_iter;
	unsigned __int32 i;

	// get network channel and set in dialog and set one as active in dialog
	unsigned __int32 l_ui32SupportedNWChannelsCount;
	unsigned __int32* l_pSupportedNWChannels;
	// get number of supported network channels to allocate memory to hold supported network channels
	m_oGdsResult = GDS_GNAUTILUS_GetSupportedNetworkChannels(m_ui64DeviceHandle, m_sDeviceNames, NULL, &l_ui32SupportedNWChannelsCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	l_pSupportedNWChannels = new unsigned __int32[l_ui32SupportedNWChannelsCount];
	m_oGdsResult = GDS_GNAUTILUS_GetSupportedNetworkChannels(m_ui64DeviceHandle, m_sDeviceNames, l_pSupportedNWChannels, &l_ui32SupportedNWChannelsCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	// get network channel currently used between base station and headstage
	unsigned __int32 l_ui32NetworkChannel;
	m_oGdsResult = GDS_GNAUTILUS_GetNetworkChannel(m_ui64DeviceHandle, m_sDeviceNames, &l_ui32NetworkChannel);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	// put network channels to combobox
	GtkComboBox *l_pComboBoxNetworkChannel = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_network_channel"));
	GtkTreeModel *l_pListStoreNetworkChannel = gtk_combo_box_get_model(l_pComboBoxNetworkChannel);
	gtk_list_store_clear(GTK_LIST_STORE(l_pListStoreNetworkChannel));
	stringstream l_sNetworkChannel;
	if (m_vComboBoxNetworkChannels.size() > 0)
		m_vComboBoxNetworkChannels.clear();

	// fill network channel combobox with available network channels
	for (i = 0; i < l_ui32SupportedNWChannelsCount; i++)
	{
		l_sNetworkChannel << l_pSupportedNWChannels[i];
		gtk_list_store_append(GTK_LIST_STORE(l_pListStoreNetworkChannel), &l_iter);
		gtk_list_store_set(GTK_LIST_STORE(l_pListStoreNetworkChannel),&l_iter,0,l_sNetworkChannel.str().c_str(),-1);
		l_sNetworkChannel.clear();
		l_sNetworkChannel.str("");

		m_vComboBoxNetworkChannels.push_back(l_pSupportedNWChannels[i]);
	}
	vector<OpenViBE::uint32>::iterator l_NetworkChannelIterator;
	l_NetworkChannelIterator = find(m_vComboBoxNetworkChannels.begin(),m_vComboBoxNetworkChannels.end(),l_ui32NetworkChannel);
	__int32 nw_ch_index = distance(m_vComboBoxNetworkChannels.begin(),l_NetworkChannelIterator);
	gtk_combo_box_set_active(l_pComboBoxNetworkChannel,nw_ch_index);

	delete [] l_pSupportedNWChannels;

	// get supported sample rates
	unsigned __int32 *l_pSupportedSamplingRates;
	unsigned __int32 l_ui32SupportedSamplingRatesCount;
	stringstream l_sSupportedSamplingRates;

	// get number of supported sample rates
	m_oGdsResult = GDS_GNAUTILUS_GetSupportedSamplingRates(m_ui64DeviceHandle, m_sDeviceNames, NULL, &l_ui32SupportedSamplingRatesCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	l_pSupportedSamplingRates = new unsigned __int32[l_ui32SupportedSamplingRatesCount];
	// get supported sample rates
	m_oGdsResult = GDS_GNAUTILUS_GetSupportedSamplingRates(m_ui64DeviceHandle, m_sDeviceNames, l_pSupportedSamplingRates, &l_ui32SupportedSamplingRatesCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	
	// set sample rates as content of combo box in corresponding dialog
	GtkComboBox *l_pComboBoxSamplingRates = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_sampling_frequency"));
	GtkTreeModel *l_pListStoreSamplingRates = gtk_combo_box_get_model(l_pComboBoxSamplingRates);
	gtk_list_store_clear(GTK_LIST_STORE(l_pListStoreSamplingRates));
	for (i = 0; i < l_ui32SupportedSamplingRatesCount; i++)
	{
		l_sSupportedSamplingRates << l_pSupportedSamplingRates[i];
		gtk_list_store_append(GTK_LIST_STORE(l_pListStoreSamplingRates), &l_iter);
		gtk_list_store_set(GTK_LIST_STORE(l_pListStoreSamplingRates), &l_iter, 0, l_sSupportedSamplingRates.str().c_str(),-1);
		l_sSupportedSamplingRates.clear();
		l_sSupportedSamplingRates.str("");
	}
	gtk_combo_box_set_active(l_pComboBoxSamplingRates,0);

	boolean l_bFunctionReturn = getFiltersForNewSamplingRate();
	if (!l_bFunctionReturn)
	{
		return false;
	}

	// set list of sensitivities according to sensitivities returned by gds function
	double *l_pSupportedSensitivities;
	unsigned __int32 l_ui32SupportedSensitivitiesCount;
	std::stringstream l_sSensitivities;
	// get number of sensitivities
	m_oGdsResult = GDS_GNAUTILUS_GetSupportedSensitivities(m_ui64DeviceHandle, m_sDeviceNames, NULL, &l_ui32SupportedSensitivitiesCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	// get sensitivities
	l_pSupportedSensitivities = new double[l_ui32SupportedSensitivitiesCount];
	m_oGdsResult = GDS_GNAUTILUS_GetSupportedSensitivities(m_ui64DeviceHandle, m_sDeviceNames, l_pSupportedSensitivities, &l_ui32SupportedSensitivitiesCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	// set items in dialog item combobox_select_sensitivity
	GtkComboBox *l_pComboBoxSensitivities = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_select_sensitivity"));
	GtkTreeModel *l_pListStoreSensitivities = gtk_combo_box_get_model(l_pComboBoxSensitivities);
	gtk_list_store_clear(GTK_LIST_STORE(l_pListStoreSensitivities));
	for (i = 0; i < l_ui32SupportedSensitivitiesCount; i++)
	{
		l_sSensitivities << (l_pSupportedSensitivities[i] / 1000);
		gtk_list_store_append(GTK_LIST_STORE(l_pListStoreSensitivities), &l_iter);
		gtk_list_store_set(GTK_LIST_STORE(l_pListStoreSensitivities), &l_iter, 0, l_sSensitivities.str().c_str(),-1);
		m_vComboBoxSensitivityValues.push_back(l_pSupportedSensitivities[i]);
		l_sSensitivities.clear();
		l_sSensitivities.str("");
	}
	gtk_combo_box_set_active(l_pComboBoxSensitivities,0);

	delete [] l_pSupportedSensitivities;

	// set input signals in input sources combobox in main configuration dialog
	GDS_GNAUTILUS_INPUT_SIGNAL *l_pSupportedInputSources;
	unsigned __int32 l_ui32SupportedInputSourcesCount;
	// first get number of supported input sources (call with third parameter set to NULL)
	m_oGdsResult = GDS_GNAUTILUS_GetSupportedInputSources(m_ui64DeviceHandle, m_sDeviceNames, NULL, &l_ui32SupportedInputSourcesCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	// allocate memory to hold input sources
	l_pSupportedInputSources = new GDS_GNAUTILUS_INPUT_SIGNAL[l_ui32SupportedInputSourcesCount];
	// now get input sources
	m_oGdsResult = GDS_GNAUTILUS_GetSupportedInputSources(m_ui64DeviceHandle, m_sDeviceNames, l_pSupportedInputSources, &l_ui32SupportedInputSourcesCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	// set values to combobox_input_source (there are only three allowed at the moment, see code below)
	GtkComboBox *l_pComboBoxInputSources = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_input_source"));
	GtkTreeModel *l_pListStoreInputSources = gtk_combo_box_get_model(l_pComboBoxInputSources);
	gtk_list_store_clear(GTK_LIST_STORE(l_pListStoreInputSources));
	stringstream l_sInputSource;
	for (i = 0; i < l_ui32SupportedInputSourcesCount; i++)
	{
		if (l_pSupportedInputSources[i] == 0)
		{
			// electrode input
			m_vComboBoxInputSources.push_back(l_pSupportedInputSources[i]);
			l_sInputSource << "Electrode";
			gtk_list_store_append(GTK_LIST_STORE(l_pListStoreInputSources), &l_iter);
			gtk_list_store_set(GTK_LIST_STORE(l_pListStoreInputSources), &l_iter, 0, l_sInputSource.str().c_str(),-1);
		}
		else if (l_pSupportedInputSources[i] == 1)
		{
			// shortcut
			m_vComboBoxInputSources.push_back(l_pSupportedInputSources[i]);
			l_sInputSource << "Shortcut";
			gtk_list_store_append(GTK_LIST_STORE(l_pListStoreInputSources), &l_iter);
			gtk_list_store_set(GTK_LIST_STORE(l_pListStoreInputSources), &l_iter, 0, l_sInputSource.str().c_str(),-1);
		}
		else if (l_pSupportedInputSources[i] == 5)
		{
			// test signal
			m_vComboBoxInputSources.push_back(l_pSupportedInputSources[i]);
			l_sInputSource << "Test Signal";
			gtk_list_store_append(GTK_LIST_STORE(l_pListStoreInputSources), &l_iter);
			gtk_list_store_set(GTK_LIST_STORE(l_pListStoreInputSources), &l_iter, 0, l_sInputSource.str().c_str(),-1);
		}
		l_sInputSource.clear();
		l_sInputSource.str("");
	}
	gtk_combo_box_set_active(l_pComboBoxInputSources,0);

	return true;
}

// get channel names for hardware currently connected (cannot be changed as electrode grid is fixed)
OpenViBE::boolean CConfigurationgNautilusInterface::getChannelNames(void)
{
	unsigned __int32 l_ui32MountedModulesCount, l_ui32ElectrodeNamesCount;
	char l_sTemporary[30];
	stringstream l_sBipolarEntry;

	// get number of mounted modules and electrode names currently available
	l_ui32MountedModulesCount = 0;
	l_ui32ElectrodeNamesCount = 0;
	m_oGdsResult = GDS_GNAUTILUS_GetChannelNames(m_ui64DeviceHandle,m_sDeviceNames,&l_ui32MountedModulesCount,NULL,&l_ui32ElectrodeNamesCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	// set array of electrode names according to names currently available
	char (*l_pElectrodeNames)[GDS_GNAUTILUS_ELECTRODE_NAME_LENGTH_MAX] = new char[l_ui32ElectrodeNamesCount][GDS_GNAUTILUS_ELECTRODE_NAME_LENGTH_MAX];
	// get electrode names
	m_oGdsResult = GDS_GNAUTILUS_GetChannelNames(m_ui64DeviceHandle,m_sDeviceNames,&l_ui32MountedModulesCount,l_pElectrodeNames,&l_ui32ElectrodeNamesCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	unsigned __int16 i;
	GtkComboBox *l_pComboBoxBipolarChannels = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface,"combobox_channel_1"));
	GtkTreeModel *l_pListStoreBipolarChannels = gtk_combo_box_get_model(l_pComboBoxBipolarChannels);
	gtk_list_store_clear(GTK_LIST_STORE(l_pListStoreBipolarChannels));

	l_sBipolarEntry << "none";
	GtkTreeIter l_iter;
	gtk_list_store_append(GTK_LIST_STORE(l_pListStoreBipolarChannels), &l_iter);
	gtk_list_store_set(GTK_LIST_STORE(l_pListStoreBipolarChannels), &l_iter, 0, l_sBipolarEntry.str().c_str(),-1);
	for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
	{
		// set electrode names as channel names in channel selection dialog
		sprintf_s(&l_sTemporary[0],30,"checkbutton_channel_%d",(i + 1));
		GtkButton *l_pCheckButtonChannel = GTK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		if ((l_pCheckButtonChannel) && (i < l_ui32ElectrodeNamesCount))
		{
			gtk_button_set_label(l_pCheckButtonChannel,&l_pElectrodeNames[i][0]);
			gtk_list_store_append(GTK_LIST_STORE(l_pListStoreBipolarChannels), &l_iter);
			gtk_list_store_set(GTK_LIST_STORE(l_pListStoreBipolarChannels), &l_iter, 0, &l_pElectrodeNames[i][0],-1);
		}
		else if ((l_pCheckButtonChannel) && (i >= l_ui32ElectrodeNamesCount))
		{
			gtk_widget_set_sensitive(GTK_WIDGET(l_pCheckButtonChannel),false);
			gtk_button_set_label(l_pCheckButtonChannel, "");
		}
	}
	for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
	{
		sprintf_s(&l_sTemporary[0],30,"combobox_channel_%d",(i + 1));
		l_pComboBoxBipolarChannels = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		if (l_pComboBoxBipolarChannels)
		{
			gtk_combo_box_set_model(l_pComboBoxBipolarChannels,l_pListStoreBipolarChannels);
			gtk_combo_box_set_active(l_pComboBoxBipolarChannels,0);
		}
	}
	// adapt number of recorded channels in main configuration dialog according to number of actual selected channels
	GtkSpinButton *l_pSpinButtonNumberOfChannels = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"spinbutton_number_of_channels"));
	gtk_spin_button_set_range(l_pSpinButtonNumberOfChannels,l_ui32ElectrodeNamesCount,l_ui32ElectrodeNamesCount);
	gtk_spin_button_set_value(l_pSpinButtonNumberOfChannels,l_ui32ElectrodeNamesCount);

	// set electrode names as channel names
	m_vChannelName.clear();
	for (i = 0; i < l_ui32ElectrodeNamesCount; i++)
	{
		m_vChannelName[i] = l_pElectrodeNames[i];
	}

	delete [] l_pElectrodeNames;

	return true;
}

// get channels currenly available for g.Nautilus used
OpenViBE::boolean CConfigurationgNautilusInterface::getAvailableChannels(void)
{
	char l_sTemporary[30];
	
	// get channels currently available on connected device
	BOOL l_bAvailableChannels[GDS_GNAUTILUS_CHANNELS_MAX];
	m_oGdsResult = GDS_GNAUTILUS_GetAvailableChannels(m_ui64DeviceHandle,m_sDeviceNames,&l_bAvailableChannels);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	m_vAvailableChannels.resize(GDS_GNAUTILUS_CHANNELS_MAX);

	for (unsigned __int16 i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
	{
		sprintf_s(&l_sTemporary[0],30,"checkbutton_channel_%d",(i + 1));
		GtkCheckButton *l_pCheckButtonChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		sprintf_s(&l_sTemporary[0],30,"label_channel_%d",(i + 1));
		GtkLabel *l_pLabelChannel = GTK_LABEL(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		sprintf_s(&l_sTemporary[0],30,"combobox_channel_%d",(i + 1));
		GtkComboBox *l_pComboBoxChannel = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		sprintf_s(&l_sTemporary[0],30,"checkbutton_car_channel_%d",(i + 1));
		GtkCheckButton *l_pCheckButtonCARChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		sprintf_s(&l_sTemporary[0],30,"checkbutton_noise_channel_%d",(i + 1));
		GtkCheckButton *l_pCheckButtonNoiseChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		if (l_bAvailableChannels[i] == 1)
		{
			if (l_pCheckButtonChannel)
			{
				gtk_widget_set_visible(GTK_WIDGET(l_pCheckButtonChannel),true);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l_pCheckButtonChannel),true);
				gtk_widget_set_visible(GTK_WIDGET(l_pLabelChannel),true);
				gtk_widget_set_visible(GTK_WIDGET(l_pComboBoxChannel),true);
				gtk_widget_set_visible(GTK_WIDGET(l_pCheckButtonCARChannel),true);
				gtk_widget_set_visible(GTK_WIDGET(l_pCheckButtonNoiseChannel),true);
			}
			m_vAvailableChannels[i] = true;
		}
		else
		{
			if (l_pCheckButtonChannel)
			{
				gtk_widget_set_visible(GTK_WIDGET(l_pCheckButtonChannel),false);
				gtk_widget_set_visible(GTK_WIDGET(l_pLabelChannel),false);
				gtk_widget_set_visible(GTK_WIDGET(l_pComboBoxChannel),false);
				gtk_widget_set_visible(GTK_WIDGET(l_pCheckButtonCARChannel),false);
				gtk_widget_set_visible(GTK_WIDGET(l_pCheckButtonNoiseChannel),false);
			}
			m_vAvailableChannels[i] = false;
		}
	}

	return true;
}

// if sampling rate changed filters in filter settings dialog have to be updated according to new sampling rate
boolean CConfigurationgNautilusInterface::getFiltersForNewSamplingRate(void)
{
	GtkTreeIter l_iter;
	unsigned __int32 i;

	// get current sample rate
	GtkComboBox *l_pComboBoxSamplingRate = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_sampling_frequency"));
	const char *l_sSamplingRate = gtk_combo_box_get_active_text(l_pComboBoxSamplingRate);
	double l_dSamplingRate = atof(l_sSamplingRate);

	// get notch and bandpass filters available
	GDS_FILTER_INFO *l_pBandPassFilters, *l_pNotchFilters;
	unsigned __int32 l_ui32BandPassFiltersCount, l_ui32NotchFiltersCount;
	std::stringstream l_sFilterDescription;
	l_sFilterDescription << "no bandpass filter";

	// get number of bandpass filters and allocate filter array correspondingly
	m_oGdsResult = GDS_GNAUTILUS_GetBandpassFilters(m_ui64DeviceHandle, m_sDeviceNames, NULL, &l_ui32BandPassFiltersCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	l_pBandPassFilters = new GDS_FILTER_INFO[l_ui32BandPassFiltersCount];
	// get bandpass filters
	m_oGdsResult = GDS_GNAUTILUS_GetBandpassFilters(m_ui64DeviceHandle, m_sDeviceNames, l_pBandPassFilters, &l_ui32BandPassFiltersCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage;
		return false;
	}
	// get number of notch filters and allocate filter array correspondingly
	m_oGdsResult = GDS_GNAUTILUS_GetNotchFilters(m_ui64DeviceHandle, m_sDeviceNames, NULL, &l_ui32NotchFiltersCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	l_pNotchFilters = new GDS_FILTER_INFO[l_ui32NotchFiltersCount];
	// get notch filters
	m_oGdsResult = GDS_GNAUTILUS_GetNotchFilters(m_ui64DeviceHandle, m_sDeviceNames, l_pNotchFilters, &l_ui32NotchFiltersCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	// set filters as combobox entries depending on the current sample rate selected
	GtkComboBox *l_pComboBoxBandPass = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface,"combobox_select_bandpass_filter"));
	GtkTreeModel *l_pListStoreBandPass = gtk_combo_box_get_model(l_pComboBoxBandPass);
	gtk_list_store_clear(GTK_LIST_STORE(l_pListStoreBandPass));

	GtkComboBox *l_pComboBoxNotch = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface,"combobox_select_notch_filter"));
	GtkTreeModel *l_pListStoreNotch = gtk_combo_box_get_model(l_pComboBoxNotch);
	gtk_list_store_clear(GTK_LIST_STORE(l_pListStoreNotch));

	if(m_vComboBoxBandpassFilterIndex.size() > 0)
		m_vComboBoxBandpassFilterIndex.clear();
	if (m_vComboBoxNotchFilterIndex.size() > 0)
		m_vComboBoxNotchFilterIndex.clear();
	if (m_vComboBoxSensitivityValues.size() > 0)
		m_vComboBoxSensitivityValues.clear();

	// fill bandpass filter combobox with available filters
	gtk_list_store_append(GTK_LIST_STORE(l_pListStoreBandPass), &l_iter);
	gtk_list_store_set(GTK_LIST_STORE(l_pListStoreBandPass),&l_iter,0,l_sFilterDescription.str().c_str(),-1);
	l_sFilterDescription.clear();
	l_sFilterDescription.str("");

	m_vComboBoxBandpassFilterIndex.push_back(-1);
	for (i = 0; i < l_ui32BandPassFiltersCount; i++)
	{
		if (l_dSamplingRate == l_pBandPassFilters[i].SamplingRate)
		{
			if (l_pBandPassFilters[i].TypeId == 1)
				l_sFilterDescription << "Butterworth - ";
			if (l_pBandPassFilters[i].TypeId == 2)
				l_sFilterDescription << "Chebyshev   - ";
			
			l_sFilterDescription << l_pBandPassFilters[i].Order << " - [";
			l_sFilterDescription << l_pBandPassFilters[i].LowerCutoffFrequency << "; ";
			l_sFilterDescription << l_pBandPassFilters[i].UpperCutoffFrequency << "] - ";
			l_sFilterDescription << l_pBandPassFilters[i].SamplingRate;
			gtk_list_store_append(GTK_LIST_STORE(l_pListStoreBandPass), &l_iter);
			gtk_list_store_set(GTK_LIST_STORE(l_pListStoreBandPass), &l_iter, 0, l_sFilterDescription.str().c_str(),-1);
			m_vComboBoxBandpassFilterIndex.push_back(i);
		}
		l_sFilterDescription.clear();
		l_sFilterDescription.str("");
	}
	gtk_combo_box_set_active(l_pComboBoxBandPass,m_rBandpassFilterIndex+1); // +1 because -1 is for "no filter".

	// fill notch filter combobox with available fliters
	l_sFilterDescription << "no notch filter";
	gtk_list_store_append(GTK_LIST_STORE(l_pListStoreNotch), &l_iter);
	gtk_list_store_set(GTK_LIST_STORE(l_pListStoreNotch),&l_iter,0,l_sFilterDescription.str().c_str(),-1);
	l_sFilterDescription.clear();
	l_sFilterDescription.str("");

	m_vComboBoxNotchFilterIndex.push_back(-1);
	for (i = 0; i < l_ui32NotchFiltersCount; i++)
	{
		if (l_dSamplingRate == l_pNotchFilters[i].SamplingRate)
		{
			if (l_pNotchFilters[i].TypeId == 1)
				l_sFilterDescription << "Butterworth - ";
			if (l_pNotchFilters[i].TypeId == 2)
				l_sFilterDescription << "Chebyshev   - ";
			
			l_sFilterDescription << l_pNotchFilters[i].Order << " - [";
			l_sFilterDescription << l_pNotchFilters[i].LowerCutoffFrequency << "; ";
			l_sFilterDescription << l_pNotchFilters[i].UpperCutoffFrequency << "] - ";
			l_sFilterDescription << l_pNotchFilters[i].SamplingRate;
			gtk_list_store_append(GTK_LIST_STORE(l_pListStoreNotch), &l_iter);
			gtk_list_store_set(GTK_LIST_STORE(l_pListStoreNotch), &l_iter, 0, l_sFilterDescription.str().c_str(),-1);
			m_vComboBoxNotchFilterIndex.push_back(i);
		}
		l_sFilterDescription.clear();
		l_sFilterDescription.str("");
	}
	gtk_combo_box_set_active(l_pComboBoxNotch,m_rNotchFilterIndex+1); // +1 because -1 is for "no filter".

	delete [] l_pBandPassFilters;
	delete [] l_pNotchFilters;

	return true;
}

// If you added more reference attribute, initialize them here
CConfigurationgNautilusInterface::CConfigurationgNautilusInterface(IDriverContext& rDriverContext, const char* sGtkBuilderFileName, string& rDeviceSerial, OpenViBE::int32& rInputSource, OpenViBE::uint32& rNetworkChannel, OpenViBE::int32& rBandpassFilterIndex,
																   OpenViBE::int32& rNotchFilterIndex, OpenViBE::float64& rSensitivity, OpenViBE::boolean& rDigitalInputEnabled, OpenViBE::boolean& rNoiseReductionEnabled, OpenViBE::boolean& rCAREnabled,
																   OpenViBE::boolean& rAccelerationDataEnabled, OpenViBE::boolean& rCounterEnabled, OpenViBE::boolean& rLinkQualityEnabled, OpenViBE::boolean& rBatteryLevelEnabled, OpenViBE::boolean& rValidationIndicatorEnabled,
																   vector<OpenViBE::uint16>& rSelectedChannels, vector<OpenViBE::int32>& rBipolarChannels, vector<OpenViBE::boolean>& rCAR, vector<OpenViBE::boolean>& rNoiseReduction)
	: CConfigurationBuilder(sGtkBuilderFileName)
	,m_rDriverContext(rDriverContext)
	,m_rDeviceSerial(rDeviceSerial)
	,m_rInputSource(rInputSource)
	,m_rNetworkChannel(rNetworkChannel)
	,m_rBandpassFilterIndex(rBandpassFilterIndex)
	,m_rNotchFilterIndex(rNotchFilterIndex)
	,m_rSensitivity(rSensitivity)
	,m_rDigitalInputEnabled(rDigitalInputEnabled)
	,m_rNoiseReductionEnabled(rNoiseReductionEnabled)
	,m_rCAREnabled(rCAREnabled)
	,m_rAccelerationDataEnabled(rAccelerationDataEnabled)
	,m_rCounterEnabled(rCounterEnabled)
	,m_rLinkQualityEnabled(rLinkQualityEnabled)
	,m_rBatteryLevelEnabled(rBatteryLevelEnabled)
	,m_rValidationIndicatorEnabled(rValidationIndicatorEnabled)
	,m_vSelectedChannels(rSelectedChannels)
	,m_vBipolarChannels(rBipolarChannels)
	,m_vCAR(rCAR)
	,m_vNoiseReduction(rNoiseReduction)
{
	m_bConnectionOpen = false;
}

boolean CConfigurationgNautilusInterface::preConfigure(void)
{
	if(! CConfigurationBuilder::preConfigure())
	{
		return false;
	}

	// Connect here all callbacks
	// Example:
	// g_signal_connect(gtk_builder_get_object(m_pBuilderConfigureInterface, "button_calibrate"), "pressed", G_CALLBACK(button_calibrate_pressed_cb), this);

	// Insert here the pre-configure code.
	// For example, you may want to check if a device is currently connected
	// and if more than one are connected. Then you can list in a dedicated combo-box 
	// the device currently connected so the user can choose which one he wants to acquire from.

	boolean l_bFunctionReturn;

	// open connection to device if not done yet before reading filters, channel names etc.
	if (!m_bConnectionOpen)
	{
		// open device handle and get connected device
		l_bFunctionReturn = openDevice();
		if (!l_bFunctionReturn)
		{
			// error logged in openDevice;
			return false;
		}
	}

	// get settings for connected hardware
	l_bFunctionReturn = getHardwareSettings();
	if (!l_bFunctionReturn)
	{
		// error logged in getHardwareSettings
		return false;
	}

	// get available channels
	l_bFunctionReturn = getAvailableChannels();
	if(!l_bFunctionReturn)
	{
		// error logged in getAvailableChannels
		return false;
	}

	l_bFunctionReturn = getChannelNames();
	if (!l_bFunctionReturn)
	{
		// error logged in getChannelNames
		return false;
	}

	// activate checkboxes in g.Nautilus Configuration dialog
	::GtkCheckButton *l_pCheckbuttonEventChannel = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_event_channel"));
	::gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l_pCheckbuttonEventChannel),m_rDigitalInputEnabled);

	::GtkCheckButton *l_pCheckbuttonNoiseReduction = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_noise_reduction"));
	::gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l_pCheckbuttonNoiseReduction),m_rNoiseReductionEnabled);

	::GtkCheckButton *l_pCheckbuttonCAR = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_car"));
	::gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l_pCheckbuttonCAR),m_rCAREnabled);
	
	// activate/deactivate channel, noise reduction and CAR checkboxes in dialog_select_channels_bipolar_car_noise
	// as well as bipolar combo boxes according to available channels
	gboolean l_bCheckbuttonNoiseReductionValue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckbuttonNoiseReduction));
	gboolean l_bCheckbuttonCARValue = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckbuttonCAR));
	char l_sTemporary[45];
	GtkCheckButton *l_pCheckButton;
	GtkComboBox *l_pComboBox;
	for (unsigned __int16 i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
	{
		sprintf_s(&l_sTemporary[0],45,"checkbutton_channel_%d",(i + 1));
		l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		if ((l_pCheckButton) && (m_vAvailableChannels[i]))
			gtk_widget_set_sensitive(GTK_WIDGET(l_pCheckButton),true);
		else if (l_pCheckButton)
			gtk_widget_set_sensitive(GTK_WIDGET(l_pCheckButton),false);

		sprintf_s(&l_sTemporary[0],45,"checkbutton_noise_channel_%d",(i + 1));
		l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		if ((l_pCheckButton) && (m_vAvailableChannels[i]))
			gtk_widget_set_sensitive(GTK_WIDGET(l_pCheckButton),l_bCheckbuttonNoiseReductionValue);
		else if (l_pCheckButton)
			gtk_widget_set_sensitive(GTK_WIDGET(l_pCheckButton),false);

		sprintf_s(&l_sTemporary[0],45,"checkbutton_car_channel_%d",(i + 1));
		l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		if ((l_pCheckButton) && (m_vAvailableChannels[i]))
			gtk_widget_set_sensitive(GTK_WIDGET(l_pCheckButton),l_bCheckbuttonCARValue);
		else if (l_pCheckButton)
			gtk_widget_set_sensitive(GTK_WIDGET(l_pCheckButton),false);

		sprintf_s(&l_sTemporary[0],45,"combobox_channel_%d",(i + 1));
		l_pComboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
		if ((l_pComboBox) && (m_vAvailableChannels[i]))
			gtk_widget_set_sensitive(GTK_WIDGET(l_pComboBox),true);
		else if (l_pComboBox)
			gtk_widget_set_sensitive(GTK_WIDGET(l_pComboBox),false);
	}

	::GtkCheckButton *l_pCheckbuttonAccelerationData = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_acceleration_data"));
	::gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l_pCheckbuttonAccelerationData),m_rAccelerationDataEnabled);

	::GtkCheckButton *l_pCheckbuttonCounter = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_counter"));
	::gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l_pCheckbuttonCounter),m_rCounterEnabled);

	::GtkCheckButton *l_pCheckbuttonLinkQuality = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_link_quality"));
	::gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l_pCheckbuttonLinkQuality),m_rLinkQualityEnabled);

	::GtkCheckButton *l_pCheckbuttonBatteryLevel= GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_battery_level"));
	::gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l_pCheckbuttonBatteryLevel),m_rBatteryLevelEnabled);

	::GtkCheckButton *l_pCheckbuttonValidationIndicator = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_validation_indicator"));
	::gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(l_pCheckbuttonValidationIndicator),m_rValidationIndicatorEnabled);

	g_signal_connect(gtk_builder_get_object(m_pBuilderConfigureInterface,"button_select_channels_bipolar_car_noise"),"pressed",G_CALLBACK(button_channel_settings_cb),this);
	g_signal_connect(gtk_builder_get_object(m_pBuilderConfigureInterface,"button_select_sensitivity_filters"),"pressed",G_CALLBACK(button_sensitivity_filters_cb),this);

	g_signal_connect(gtk_builder_get_object(m_pBuilderConfigureInterface,"button_select_sensitivity_filters_apply"),"pressed",G_CALLBACK(button_sensitivity_filters_apply_cb),this);
	g_signal_connect(gtk_builder_get_object(m_pBuilderConfigureInterface,"button_channel_apply"),"pressed",G_CALLBACK(button_channel_settings_apply_cb),this);

	// set device serial in device serial text entry in g.Nautilus Configuration dialog
	::GtkEntry *l_pEntryDeviceSerial = GTK_ENTRY(gtk_builder_get_object(m_pBuilderConfigureInterface, "entry_device_serial"));
	::gtk_entry_set_text(l_pEntryDeviceSerial,m_rDeviceSerial.c_str());
	::gtk_entry_set_editable(l_pEntryDeviceSerial,false);

	// deactivate buttons if no device is detected
	if (gtk_entry_get_text_length(l_pEntryDeviceSerial) < 13)
	{
		GtkWidget *l_pButton = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface,"button_select_channels_bipolar_car_noise"));
		gtk_widget_set_sensitive(l_pButton,false);

		l_pButton = GTK_WIDGET(gtk_builder_get_object(m_pBuilderConfigureInterface,"button_select_sensitivity_filters"));
		gtk_widget_set_sensitive(l_pButton,false);
	}

	// catch event when sample rate is changed to adjust available filters
	g_signal_connect(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_sampling_frequency"),"changed",G_CALLBACK(sample_rate_changed_cb),this);

	// catch events when car and noise reduction checkboxes are toggled in main config dialog to enable/disable corresponding checkboxes in related dialog
	g_signal_connect(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_noise_reduction"),"toggled",G_CALLBACK(noise_reduction_changed_cb),this);
	g_signal_connect(gtk_builder_get_object(m_pBuilderConfigureInterface, "checkbutton_car"), "toggled", G_CALLBACK(car_changed_cb),this);

	return true;
}

boolean CConfigurationgNautilusInterface::postConfigure(void)
{
	if(m_bApplyConfiguration)
	{
		// If the user pressed the "apply" button, you need to save the changes made in the configuration.
		// For example, you can save the connection ID of the selected device:
		// m_ui32ConnectionID = <value-from-gtk-widget>
		GtkCheckButton *l_pCheckButton;
		unsigned __int16 i;
		unsigned __int32 l_ui32ComboBoxIndex;
		gboolean l_bButtonValue = false;
		gdouble l_dNumberOfChannels = 0;

		// get serial number from configuration dialog
		GtkEntry *l_pEntrySerial = GTK_ENTRY(gtk_builder_get_object(m_pBuilderConfigureInterface, "entry_device_serial"));
		m_rDeviceSerial = gtk_entry_get_text(l_pEntrySerial);

		// get selected input source from dialog
		GtkComboBox *l_pComboBoxInputSources = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_input_source"));
		l_ui32ComboBoxIndex = gtk_combo_box_get_active(l_pComboBoxInputSources);
		m_rInputSource = m_vComboBoxInputSources[l_ui32ComboBoxIndex];

		// get selected channels
		char l_sTemporary[45];
		m_vSelectedChannels.clear();
		for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
		{
			sprintf_s(&l_sTemporary[0],45,"checkbutton_channel_%d",(i + 1));
			l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
			if (l_pCheckButton)
			{
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton)))
				{
					m_vSelectedChannels.push_back(i + 1);
					l_dNumberOfChannels += 1;
				}
				else
				{
					m_vSelectedChannels.push_back(0);
					m_vChannelName.erase(i);
				}
			}
		}

		// get bipolar channels
		m_vBipolarChannels.clear();
		for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
		{
			sprintf_s(&l_sTemporary[0],45,"combobox_channel_%d", (i + 1));
			GtkComboBox *l_pComboBoxBipolarChannels = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
			if (l_pComboBoxBipolarChannels)
			{
				// bipolar is 0 based for the channels, -1 indicates no bipolar derivation
				m_vBipolarChannels.push_back(gtk_combo_box_get_active(l_pComboBoxBipolarChannels) - 1);
			}
		}

		// set bandpass and notch filter indices to correponding variables
		GtkComboBox *l_pComboBoxBandpassFilters = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface,"combobox_select_bandpass_filter"));
		l_ui32ComboBoxIndex = gtk_combo_box_get_active(l_pComboBoxBandpassFilters);
		m_rBandpassFilterIndex = m_vComboBoxBandpassFilterIndex.at(l_ui32ComboBoxIndex);

		GtkComboBox *l_pComboBoxNotchFilters = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface,"combobox_select_notch_filter"));
		l_ui32ComboBoxIndex = gtk_combo_box_get_active(l_pComboBoxNotchFilters);
		m_rNotchFilterIndex = m_vComboBoxNotchFilterIndex.at(l_ui32ComboBoxIndex);

		// sensitivity
		GtkComboBox *l_pComboBoxSensitivities = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface,"combobox_select_sensitivity"));
		l_ui32ComboBoxIndex = gtk_combo_box_get_active(l_pComboBoxSensitivities);
		m_rSensitivity = m_vComboBoxSensitivityValues.at(l_ui32ComboBoxIndex);

		// digital inputs
		l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"checkbutton_event_channel"));
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton)) == 1)
			m_rDigitalInputEnabled = true;
		else
			m_rDigitalInputEnabled = false;

		// noise reduction
		l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"checkbutton_noise_reduction"));
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton)) == 1)
			m_rNoiseReductionEnabled = true;
		else
			m_rNoiseReductionEnabled = false;

		// if noise reduction is active, check for which channels noise reduction is enabled
		m_vNoiseReduction.clear();
		for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
		{
			sprintf_s(&l_sTemporary[0],45,"checkbutton_noise_channel_%d",(i + 1));
			l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
			if (l_pCheckButton)
			{
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton)) == 1)
					m_vNoiseReduction.push_back(true);
				else
					m_vNoiseReduction.push_back(false);
			}
		}

		// CAR
		l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"checkbutton_car"));
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton)) == 1)
			m_rCAREnabled = true;
		else
			m_rCAREnabled = false;
		
		// if CAR is active, check for which channels CAR is enabled
		m_vCAR.clear();
		for (i = 0; i < GDS_GNAUTILUS_CHANNELS_MAX; i++)
		{
			sprintf_s(&l_sTemporary[0],45,"checkbutton_car_channel_%d",(i + 1));
			l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,l_sTemporary));
			if (l_pCheckButton)
			{
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton)) == 1)
					m_vCAR.push_back(true);
				else
					m_vCAR.push_back(false);
			}
		}

		// acceleration data
		l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"checkbutton_acceleration_data"));
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton)) == 1)
			m_rAccelerationDataEnabled = true;
		else
			m_rAccelerationDataEnabled = false;

		// counter
		l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"checkbutton_counter"));
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton)) == 1)
			m_rCounterEnabled = true;
		else
			m_rCounterEnabled = false;

		// link quality
		l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"checkbutton_link_quality"));
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton)) == 1)
			m_rLinkQualityEnabled = true;
		else
			m_rLinkQualityEnabled = false;

		// battery level
		l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"checkbutton_battery_level"));
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton)) == 1)
			m_rBatteryLevelEnabled = true;
		else
			m_rBatteryLevelEnabled = false;

		// validation indicator
		l_pCheckButton = GTK_CHECK_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface,"checkbutton_validation_indicator"));
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_pCheckButton)) == 1)
			m_rValidationIndicatorEnabled = true;
		else
			m_rValidationIndicatorEnabled = false;

		// network channel
		GtkComboBox *l_pComboBoxNetworkChannel = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface,"combobox_network_channel"));
		const char* l_sNetworkChannel = gtk_combo_box_get_active_text(l_pComboBoxNetworkChannel);
		m_rNetworkChannel = (unsigned __int32)atoi(l_sNetworkChannel);

		// set number of channels in main configuration dialog (spinbutton still remains disabled, user cannot change value there)
		GtkSpinButton *l_pSpinButtonChannels = GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_number_of_channels"));
		gtk_spin_button_set_value(l_pSpinButtonChannels, l_dNumberOfChannels);
		gtk_spin_button_set_range(l_pSpinButtonChannels, l_dNumberOfChannels, l_dNumberOfChannels);
	}
	if (m_bConnectionOpen)
	{
		// close connection handle
		boolean l_bFunctionReturn = closeDevice();
		if (!l_bFunctionReturn)
		{
			// error logged in closeDevice
			return false;
		}
	}

	if(! CConfigurationBuilder::postConfigure()) // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed
	{
		return false;
	}

	return true;
}

boolean CConfigurationgNautilusInterface::openDevice(void)
{
	GDS_ENDPOINT l_epHostEp, l_epLocalEp;
	GDS_DEVICE_CONNECTION_INFO *l_pConnectedDevicesInfo;
	unsigned __int32 l_ui32DeviceCount;
	string l_sDeviceSerial;
	BOOL l_bOpenExclusively = true;
	BOOL l_bIsCreator;
	m_sDeviceNames = new char[1][DEVICE_NAME_LENGTH_MAX];

	uint8 l_ui8ByteIP[4];
	l_ui8ByteIP[0] = 1;
	l_ui8ByteIP[1] = 0;
	l_ui8ByteIP[2] = 0;
	l_ui8ByteIP[3] = 127;
	char l_sTemporaryIP[16];

	_snprintf_s(l_sTemporaryIP,IP_ADDRESS_LENGTH_MAX,"%d.%d.%d.%d",l_ui8ByteIP[3],l_ui8ByteIP[2],l_ui8ByteIP[1],l_ui8ByteIP[0]);

	for(unsigned __int32 i = 0; i < IP_ADDRESS_LENGTH_MAX; i++)
	{
		l_epHostEp.IpAddress[i] = l_sTemporaryIP[i];
		l_epLocalEp.IpAddress[i] = l_sTemporaryIP[i];
	}

	l_epHostEp.Port = 50223;
	l_epLocalEp.Port = 50224;

	// get connected device
	m_oGdsResult = GDS_GetConnectedDevices(l_epHostEp, l_epLocalEp, &l_pConnectedDevicesInfo, &l_ui32DeviceCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}
	
	for(unsigned __int32 i = 0; i < l_ui32DeviceCount; i++)
	{
		// if devices are in use they cannot be used for a new acquisition
		if((l_pConnectedDevicesInfo[i].InUse) && (i < l_ui32DeviceCount))
			continue;
		// only one device can be used for data acquisition, as g.Nautilus cannot be synchronized
		if ((l_pConnectedDevicesInfo[i].InUse) && (l_pConnectedDevicesInfo[i].ConnectedDevicesLength > 1))
			continue;
		GDS_DEVICE_INFO *l_pDeviceInfo = l_pConnectedDevicesInfo[i].ConnectedDevices;
		if(l_pDeviceInfo[0].DeviceType == GDS_DEVICE_TYPE_GNAUTILUS)
		{
			l_sDeviceSerial = l_pDeviceInfo[0].Name;
			l_ui32DeviceCount = 1;
			break;
		}
	}

	if (l_ui32DeviceCount == 0)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "No g.Nautilus connected\n";
		return false;
	}

	strncpy_s(m_sDeviceNames[0], l_sDeviceSerial.c_str(), DEVICE_NAME_LENGTH_MAX);
	m_rDeviceSerial = l_sDeviceSerial;

	// connect to device
	m_oGdsResult = GDS_Connect(l_epHostEp, l_epLocalEp, m_sDeviceNames, l_ui32DeviceCount, l_bOpenExclusively, &m_ui64DeviceHandle, &l_bIsCreator);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	// free connected devices list allocated by GDS_GetConnectedDevices
	m_oGdsResult = GDS_FreeConnectedDevicesList(&l_pConnectedDevicesInfo,l_ui32DeviceCount);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	m_bConnectionOpen = true;

	return true;
}

boolean CConfigurationgNautilusInterface::closeDevice(void)
{
	// disconnect device
	m_oGdsResult = GDS_Disconnect(&m_ui64DeviceHandle);
	if (m_oGdsResult.ErrorCode != GDS_ERROR_SUCCESS)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << m_oGdsResult.ErrorMessage << "\n";
		return false;
	}

	m_bConnectionOpen = false;
	delete [] m_sDeviceNames;

	return true;
}

#endif // TARGET_HAS_ThirdPartyGNEEDaccessAPI

