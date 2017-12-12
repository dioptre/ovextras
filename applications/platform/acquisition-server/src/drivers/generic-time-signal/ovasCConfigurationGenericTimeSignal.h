#ifndef __OpenViBE_AcquisitionServer_CConfigurationGenericTimeSignal_H__
#define __OpenViBE_AcquisitionServer_CConfigurationGenericTimeSignal_H__

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CConfigurationGenericTimeSignal
	 * \sa CDriverGenericTimeSignal
	 */
	class CConfigurationGenericTimeSignal : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
	public:

		CConfigurationGenericTimeSignal(OpenViBEAcquisitionServer::IDriverContext& rDriverContext, const char* sGtkBuilderFileName);

		virtual OpenViBE::boolean preConfigure(void);
		virtual OpenViBE::boolean postConfigure(void);

	protected:

		OpenViBEAcquisitionServer::IDriverContext& m_rDriverContext;

	private:

	};
};

#endif // __OpenViBE_AcquisitionServer_CConfigurationGenericTimeSignal_H__
