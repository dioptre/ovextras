/*
* NeuroServo driver for OpenViBE
*
* \author (NeuroServo)
* \date Wed Nov 23 00:24:00 2016
*
*/

#ifndef __OpenViBE_AcquisitionServer_CConfigurationNeuroServoHid_H__
#define __OpenViBE_AcquisitionServer_CConfigurationNeuroServoHid_H__

#if defined TARGET_OS_Windows
#if defined TARGET_HAS_ThirdPartyNeuroServo

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CConfigurationNeuroServoHid
	 * \author  (NeuroServo)
	 * \date Wed Nov 23 00:24:00 2016
	 * \brief The CConfigurationNeuroServoHid handles the configuration dialog specific to the NeuroServo device.
	 *
	 * TODO: details
	 *
	 * \sa CDriverNeuroServoHid
	 */
	class CConfigurationNeuroServoHid : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
	public:

		CConfigurationNeuroServoHid(OpenViBEAcquisitionServer::IDriverContext& rDriverContext, const char* sGtkBuilderFileName);

		virtual OpenViBE::boolean preConfigure(void);
		virtual OpenViBE::boolean postConfigure(void);

		// Automatic Shutdown
		void checkRadioAutomaticShutdown(const OpenViBE::boolean bAutoShutdownStatus);
		OpenViBE::boolean getAutomaticShutdownStatus();
		void setRadioAutomaticShutdown(OpenViBE::boolean state);

		// Shutdown on driver disconnect
		void checkRadioShutdownOnDriverDisconnect(const OpenViBE::boolean bShutdownOnDriverDisconnectStatus);
		OpenViBE::boolean getShutdownOnDriverDisconnectStatus();
		void setRadioShutdownOnDriverDisconnect(OpenViBE::boolean state);

		// Device Light Enable
		void checkRadioDeviceLightEnable(const OpenViBE::boolean bDeviceLightEnableStatus);
		OpenViBE::boolean getDeviceLightEnableStatus();
		void setRadioDeviceLightEnable(OpenViBE::boolean state);

	protected:

		OpenViBEAcquisitionServer::IDriverContext& m_rDriverContext;

	private:

		OpenViBE::boolean m_bAutomaticShutdown;
		OpenViBE::boolean m_bShutdownOnDriverDisconnect;
		OpenViBE::boolean m_bDeviceLightEnable;
	};
};

#endif
#endif // TARGET_OS_Windows

#endif // __OpenViBE_AcquisitionServer_CConfigurationNeuroServoHid_H__
