#ifndef __OpenViBE_AcquisitionServer_CConfigurationDriverSimulatedDeviator_H__
#define __OpenViBE_AcquisitionServer_CConfigurationDriverSimulatedDeviator_H__

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CConfigurationDriverSimulatedDeviator
	 * \author Jussi T. Lindgren (Inria)
	 * \brief The CConfigurationDriverSimulatedDeviator handles the configuration dialog specific to the Simulated Deviator driver
	 *
	 * \sa CDriverSimulatedDeviator
	 */

	class CConfigurationDriverSimulatedDeviator : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
		public:
			CConfigurationDriverSimulatedDeviator(OpenViBEAcquisitionServer::IDriverContext& rDriverContext, const char* sGtkBuilderFileName
				, OpenViBE::boolean& rSendPeriodicStimulations
				, OpenViBE::float64& rOffset
				, OpenViBE::float64& rSpread
				, OpenViBE::float64& rMaxDev
				, OpenViBE::float64& rPullback
				, OpenViBE::float64& rUpdate	
				, OpenViBE::uint64& rWavetype
				, OpenViBE::float64& rFreezeFrequency
				, OpenViBE::float64& rFreezeDuration
				);

			virtual OpenViBE::boolean preConfigure(void);
			virtual OpenViBE::boolean postConfigure(void);

		protected:

		OpenViBEAcquisitionServer::IDriverContext& m_rDriverContext;

		OpenViBE::boolean& m_rSendPeriodicStimulations;
		OpenViBE::float64& m_Offset;
		OpenViBE::float64& m_Spread;
		OpenViBE::float64& m_MaxDev;
		OpenViBE::float64& m_Pullback;
		OpenViBE::float64& m_Update;
		OpenViBE::uint64& m_Wavetype;
		OpenViBE::float64& m_FreezeFrequency;
		OpenViBE::float64& m_FreezeDuration;
	};
};

#endif // __OpenViBE_AcquisitionServer_CConfigurationDriverSimulatedDeviator_H__

