#include "ovdCRenameDialog.h"

using namespace OpenViBEDesigner;
using namespace OpenViBE;
using namespace OpenViBE::Plugins;
using namespace OpenViBE::Kernel;

CRenameDialog::CRenameDialog(const IKernel& rKernel, const CString& rInitialName, const CString& rDefaultName, const char* sGUIFilename)
	:m_rKernel(rKernel)
	,m_sInitialName(rInitialName)
	,m_sDefaultName(rDefaultName)
	,m_sResult(rInitialName)
	,m_sGUIFilename(sGUIFilename)
{
}

CRenameDialog::~CRenameDialog(void)
{
}

boolean CRenameDialog::run(void)
{
	::GladeXML* l_pInterface=glade_xml_new(m_sGUIFilename.toASCIIString(), "rename", NULL);
	::GtkWidget* l_pDialog=glade_xml_get_widget(l_pInterface, "rename");
	::GtkWidget* l_pName=glade_xml_get_widget(l_pInterface, "rename-entry");
	g_object_unref(l_pInterface);

	gtk_entry_set_text(GTK_ENTRY(l_pName), m_sInitialName.toASCIIString());

	boolean l_bFinished=false;
	boolean l_bResult;
	while(!l_bFinished)
	{
		gint l_iResult=gtk_dialog_run(GTK_DIALOG(l_pDialog));
		if(l_iResult==0) // revert
		{
			gtk_entry_set_text(GTK_ENTRY(l_pName), m_sDefaultName.toASCIIString());
		}
		else if(l_iResult==GTK_RESPONSE_APPLY)
		{
			m_sResult=gtk_entry_get_text(GTK_ENTRY(l_pName));
			l_bFinished=true;
			l_bResult=true;
		}
		else
		{
			l_bFinished=true;
			l_bResult=false;
		}
	}

	gtk_widget_destroy(l_pDialog);

	return l_bResult;
}

CString CRenameDialog::getResult(void)
{
	return m_sResult;
}
