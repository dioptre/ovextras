#ifndef __OpenViBE_AcquisitionServer_CConfigurationOpenBCI_H__
#define __OpenViBE_AcquisitionServer_CConfigurationOpenBCI_H__

#include "../ovasCConfigurationBuilder.h"

#include <gtk/gtk.h>

namespace OpenViBEAcquisitionServer
{
	class CConfigurationOpenBCI : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
	public:
		CConfigurationOpenBCI(const char* sGtkBuilderFileName, OpenViBE::uint32& rUSBIndex);
		virtual ~CConfigurationOpenBCI(void);

		virtual OpenViBE::boolean preConfigure(void);
		virtual OpenViBE::boolean postConfigure(void);
		
		OpenViBE::boolean setComInit(const OpenViBE::CString& sComInit);
		OpenViBE::CString getComInit(void) const;
		
		OpenViBE::boolean setComDelay(const OpenViBE::uint32 sComDelay);
		OpenViBE::uint32 getComDelay(void) const;
		

	protected:
		OpenViBE::uint32& m_rUSBIndex;
		::GtkListStore* m_pListStore;
		::GtkEntry* l_pEntryComInit;
		OpenViBE::CString m_sComInit;
		OpenViBE::uint32 m_sComDelay;
	};
};

#endif // __OpenViBE_AcquisitionServer_CConfigurationOpenBCI_H__
