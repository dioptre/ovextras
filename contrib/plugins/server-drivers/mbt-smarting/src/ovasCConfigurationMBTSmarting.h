#ifndef __OpenViBE_AcquisitionServer_CConfigurationMBTSmarting_H__
#define __OpenViBE_AcquisitionServer_CConfigurationMBTSmarting_H__

#if defined(TARGET_OS_Windows)

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>

namespace OpenViBEAcquisitionServer
{
	
	class CConfigurationMBTSmarting : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
	public:

		CConfigurationMBTSmarting(const char* sGtkBuilderFileName, OpenViBE::uint32& rUSBIndex);
		virtual OpenViBE::boolean preConfigure(void);
		virtual OpenViBE::boolean postConfigure(void);
		virtual ~CConfigurationMBTSmarting(void);
	
	protected:

		OpenViBE::uint32& m_rUSBIndex;
		
		::GtkListStore* m_pListStore;

	private:

	};
};

#endif

#endif // __OpenViBE_AcquisitionServer_CConfigurationMBTSmarting_H__
