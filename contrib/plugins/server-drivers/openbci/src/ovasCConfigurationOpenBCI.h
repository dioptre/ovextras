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

	protected:
		OpenViBE::uint32& m_rUSBIndex;
		::GtkListStore* m_pListStore;
	};
};

#endif // __OpenViBE_AcquisitionServer_CConfigurationOpenBCI_H__
