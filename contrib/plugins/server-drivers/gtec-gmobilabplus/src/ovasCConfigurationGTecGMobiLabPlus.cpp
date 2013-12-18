/**
 * The gMobilab driver was contributed
 * by Lucie Daubigney from Supelec Metz
 */

#include "ovasCConfigurationGTecGMobiLabPlus.h"

#if defined TARGET_HAS_ThirdPartyGMobiLabPlusAPI

#define boolean OpenViBE::boolean

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

CConfigurationGTecGMobiLabPlus::CConfigurationGTecGMobiLabPlus(const char* sGtkBuilderFileName, const char* sPortName)
	:CConfigurationBuilder(sGtkBuilderFileName)
	,m_oPortName(sPortName)
{
}

boolean CConfigurationGTecGMobiLabPlus::preConfigure(void)
{
	if(!CConfigurationBuilder::preConfigure())
	{
		return false;
	}

	::GtkEntry* l_pEntryPort=GTK_ENTRY(gtk_builder_get_object(m_pBuilderConfigureInterface, "entry_port"));

	::gtk_entry_set_text(l_pEntryPort, m_oPortName.c_str());

	return true;
}


boolean CConfigurationGTecGMobiLabPlus::postConfigure(void)
{
	::GtkEntry* l_pEntryPort=GTK_ENTRY(gtk_builder_get_object(m_pBuilderConfigureInterface, "entry_port"));
	if(m_bApplyConfiguration)
	{
		m_oPortName=::gtk_entry_get_text(l_pEntryPort);
	}

	if(!CConfigurationBuilder::postConfigure())
	{
		return false;
	}

	return true;
}

std::string CConfigurationGTecGMobiLabPlus::getPortName(void)
{
  return m_oPortName;
}

#endif // TARGET_HAS_ThirdPartyGMobiLabPlusAPI
