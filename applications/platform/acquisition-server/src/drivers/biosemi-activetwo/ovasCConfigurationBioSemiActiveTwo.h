/*
 * ovasCConfigurationBioSemiActiveTwo.h
 *
 * Copyright (c) 2012, Mensia Technologies SA. All rights reserved.
 * -- Rights transferred to Inria, contract signed 21.11.2014
 *
 */

#ifdef TARGET_HAS_ThirdPartyBioSemiAPI

#ifndef __OpenViBE_AcquisitionServer_CConfigurationBioSemiActiveTwo_H__
#define __OpenViBE_AcquisitionServer_CConfigurationBioSemiActiveTwo_H__

#include "../ovasCConfigurationBuilder.h"

#include <gtk/gtk.h>

#include <vector>

namespace OpenViBEAcquisitionServer
{
	class CConfigurationBioSemiActiveTwo : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
	public:

		CConfigurationBioSemiActiveTwo(const char* sGtkBuilderFileName);

		virtual OpenViBE::boolean preConfigure(void);
		virtual OpenViBE::boolean postConfigure(void);

	protected:
	};
};


#endif // __OpenViBE_AcquisitionServer_CConfigurationBioSemiActiveTwo_H__
#endif // TARGET_HAS_ThirdPartyBioSemiAPI
