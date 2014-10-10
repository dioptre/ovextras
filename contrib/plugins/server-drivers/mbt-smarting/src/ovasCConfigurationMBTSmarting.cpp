
#if defined(TARGET_OS_Windows)

#include "ovasCConfigurationMBTSmarting.h"
#include <windows.h>
#include <string.h>
#include <iostream>

#define boolean OpenViBE::boolean

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

CConfigurationMBTSmarting::CConfigurationMBTSmarting(const char* sGtkBuilderFileName, OpenViBE::uint32& rUSBIndex)
	:CConfigurationBuilder(sGtkBuilderFileName)
	,m_rUSBIndex(rUSBIndex)
{
	m_pListStore=gtk_list_store_new(1, G_TYPE_STRING);
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

	::GtkComboBox* l_pComboBox=GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_device"));

	g_object_unref(m_pListStore);
	m_pListStore=gtk_list_store_new(1, G_TYPE_STRING);
	
	gtk_combo_box_set_model(l_pComboBox, GTK_TREE_MODEL(m_pListStore));

	char l_sBuffer[1024];
	boolean l_bSelected=false;

	char lpTargetPath[50]; // buffer to store the path of the COMPORTS
    DWORD test;
    bool gotPort=0; // in case the port is not found

    for(int i=0; i<255; i++) // checking ports from COM0 to COM255
    {
       
       ::sprintf(lpTargetPath, "COM%i", i);
        test = QueryDosDevice(lpTargetPath, (LPSTR)lpTargetPath, 5000);

            // Test the return value and error if any
        if(test!=0) //QueryDosDevice returns zero if it didn't find an object
        {
			 char comPort[50];
			 ::sprintf(comPort, "COM%i", i);
			 ::gtk_combo_box_append_text(l_pComboBox, comPort);
             gotPort=1; // found port
        }

        if(::GetLastError()==ERROR_INSUFFICIENT_BUFFER)
        {
            lpTargetPath[10000]; // in case the buffer got filled, increase size of the buffer.
            continue;
        }

    }

	if(!l_bSelected)
	{
		::gtk_combo_box_set_active(l_pComboBox, 0);
	}

	return true;
}

boolean CConfigurationMBTSmarting::postConfigure(void)
{
	::GtkComboBox* l_pComboBox=GTK_COMBO_BOX(gtk_builder_get_object(m_pBuilderConfigureInterface, "combobox_device"));

	if(m_bApplyConfiguration)
	{		
		const gchar* port=gtk_combo_box_get_active_text (l_pComboBox);
		int l_iUSBIndex = 0;
		char* port1 = (char*) port;
		if(strlen(port1)>4)
			l_iUSBIndex = 10*((int)port[3] - '0') + (int)port[4] - '0' ;
		else
			l_iUSBIndex = (int)port[3] - '0';
		if(l_iUSBIndex>=0)
		{
			m_rUSBIndex=(uint32)l_iUSBIndex;
		}

	}

	if(! CConfigurationBuilder::postConfigure()) // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed
	{
		return false;
	}

	return true;
}

#endif
