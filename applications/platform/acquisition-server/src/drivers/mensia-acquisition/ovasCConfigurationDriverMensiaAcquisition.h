#ifndef __OpenViBE_AcquisitionServer_CConfigurationDriverMensiaAcquisition_H__
#define __OpenViBE_AcquisitionServer_CConfigurationDriverMensiaAcquisition_H__

#ifdef TARGET_OS_Windows


#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CConfigurationDriverMensiaAcquisition
	 * \author Jozef Legeny (Mensia)
	 * \date 28 jan 2013
	 * \brief The CConfigurationDriverMensiaAcquisition loads the configuration using the mensia-acquisition dynamic library
	 *
	 * \sa CDriverGenericOscillator
	 */

	class CConfigurationDriverMensiaAcquisition : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
		public:
			CConfigurationDriverMensiaAcquisition(OpenViBEAcquisitionServer::IDriverContext& rDriverContext, const char* sGtkBuilderFileName);

			virtual OpenViBE::boolean preConfigure(void);
			virtual OpenViBE::boolean postConfigure(void);

		protected:

		OpenViBEAcquisitionServer::IDriverContext& m_rDriverContext;

	};
}

#endif // TARGET_OS_Windows

#endif // __OpenViBE_AcquisitionServer_CConfigurationDriverMensiaAcquisition_H__

