#if defined(TARGET_OS_Windows)
#include "ovasCConfigurationMBTSmarting.h"

#include <windows.h>
#include <string.h>
#include <iostream>
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
	CConfigurationMBTSmarting* l_pConfig=static_cast<CConfigurationMBTSmarting*>(pUserData);
	l_pConfig->buttonCalibratePressedCB();
}

//Callback actually called:
void CConfigurationGTecGUSBamp::buttonCalibratePressedCB(void)
{
	// Connect to the hardware, ask for calibration, verify the return code, etc.
}
_________________________________________________*/

// If you added more reference attribute, initialize them here
CConfigurationMBTSmarting::CConfigurationMBTSmarting(IDriverContext& rDriverContext, const char* sGtkBuilderFileName, OpenViBE::uint32& rConnectionId)
	:CConfigurationBuilder(sGtkBuilderFileName)
	,m_rDriverContext(rDriverContext)
	,m_ui32ConnectionID(rConnectionId)
{
	m_pListStore = gtk_list_store_new(1, G_TYPE_STRING);
}

CConfigurationMBTSmarting::~CConfigurationMBTSmarting(void)
{
	g_object_unref(m_pListStore);
}

boolean CConfigurationMBTSmarting::preConfigure(void)
{
	if(! CConfigurationBuilder::preConfigure())
	{
		return false;
	}
	::GtkComboBox* l_pComboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_device"));

	g_object_unref(m_pListStore);
	m_pListStore = gtk_list_store_new(1, G_TYPE_STRING);
	
	gtk_combo_box_set_model(l_pComboBox, GTK_TREE_MODEL(m_pListStore));


	boolean l_bSelected = false;

	char lpTargetPath[50]; // buffer to store the path of the COMPORTS
    DWORD test;
    bool gotPort=0; // in case the port is not found

    for(int i = 0; i < 255; i++) // checking ports from COM0 to COM255
    {
       
       ::sprintf(lpTargetPath, "COM%i", i);
        test = QueryDosDevice(lpTargetPath, (LPSTR)lpTargetPath, 5000);

            // Test the return value and error if any
        if(test != 0) //QueryDosDevice returns zero if it didn't find an object
        {
			 char comPort[50];
			 ::sprintf(comPort, "COM%i", i);
			 ::gtk_combo_box_append_text(l_pComboBox, comPort);
             gotPort = 1; // found port
        }

        if(::GetLastError( )== ERROR_INSUFFICIENT_BUFFER)
        {
            lpTargetPath[10000]; // in case the buffer got filled, increase size of the buffer.
            continue;
        }
    }
	
	// Connect here all callbacks
	// Example:
	// g_signal_connect(gtk_builder_get_object(m_pBuilderConfigureInterface, "button_calibrate"), "pressed", G_CALLBACK(button_calibrate_pressed_cb), this);

	// Insert here the pre-configure code.
	// For example, you may want to check if a device is currently connected
	// and if more than one are connected. Then you can list in a dedicated combo-box 
	// the device currently connected so the user can choose which one he wants to acquire from.

	return true;
}

boolean CConfigurationMBTSmarting::postConfigure(void)
{
	::GtkComboBox* l_pComboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_device"));

	if(m_bApplyConfiguration)
	{
		// If the user pressed the "apply" button, you need to save the changes made in the configuration.
		// For example, you can save the connection ID of the selected device:
		// m_ui32ConnectionID = <value-from-gtk-widget>
		const gchar* port = gtk_combo_box_get_active_text(l_pComboBox);

		// TODO: FIX THIS SHIT
		if(!port)
		{
			std::cout << "Error: Port not specified or invalid\n";
		}
		else
		{
			int l_iUSBIndex = 0;
			char* port1 = (char*) port;
			if(strlen(port1) > 4)
				l_iUSBIndex = 10*((int)port[3] - '0') + (int)port[4] - '0' ;
			else
				l_iUSBIndex = (int)port[3] - '0';
			if(l_iUSBIndex>=0)
			{
				m_ui32ConnectionID = (uint32)l_iUSBIndex;
			}
		}

	}

	

	if(! CConfigurationBuilder::postConfigure()) // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed
	{
		return false;
	}

	return true;
}

#endif