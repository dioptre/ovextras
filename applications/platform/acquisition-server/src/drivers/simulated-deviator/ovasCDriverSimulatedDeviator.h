#ifndef __OpenViBE_AcquisitionServer_CDriverSimulatedDeviator_H__
#define __OpenViBE_AcquisitionServer_CDriverSimulatedDeviator_H__

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <random>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDriverSimulatedDeviator
	 * \brief Simulates a drifting acquisition device by changing the sample rate by a random process.
	 * \author Jussi T. Lindgren (Inria)
	 *
	 * The driver simulates a square wave attempting to model a steady, analog process. The square wave changes
	 * sign every 1 sec. This process is then sampled using a sampling rate that is changing during the 
	 * recording according to the parameters given to the driver. These parameters are
	 *
	 * Offset   - The center sampling frequency deviation from the declared driver sampling rate. Can be negative.
	 * Spread   - How big jumps the random walk takes; related to the sigma of the normal distribution
	 * MaxDev   - The maximum allowed deviation in Hz from the true sampling rate + Offset
	 * Pullback - How strongly the random walk is pulled towards the true sampling rate + Offset
	 * Update   - How often the sampling rate is changed (in seconds)
	 *
	 * The MaxDev and Pullback are used to keep the stochastic process from diverging.
	 *
	 */
	class CDriverSimulatedDeviator : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverSimulatedDeviator(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
		virtual void release(void);
		virtual const char* getName(void);

		virtual OpenViBE::boolean initialize(
			const OpenViBE::uint32 ui32SampleCountPerSentBlock,
			OpenViBEAcquisitionServer::IDriverCallback& rCallback);
		virtual OpenViBE::boolean uninitialize(void);

		virtual OpenViBE::boolean start(void);
		virtual OpenViBE::boolean stop(void);
		virtual OpenViBE::boolean loop(void);

		virtual OpenViBE::boolean isConfigurable(void);
		virtual OpenViBE::boolean configure(void);
		virtual const OpenViBEAcquisitionServer::IHeader* getHeader(void) { return &m_oHeader; }

	protected:

		SettingsHelper m_oSettings;

		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;
		OpenViBEAcquisitionServer::CHeader m_oHeader;

		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;
		std::vector<OpenViBE::float32> m_vSample;

		OpenViBE::uint64 m_ui64TotalSampleCount;		// Number of samples sent by the drifting sampling process
		OpenViBE::uint64 m_ui64TotalSampleCountReal;    // Number of samples of some imaginary, steady 'real' process, here a square wave

		OpenViBE::uint64 m_ui64StartTime;
		OpenViBE::uint64 m_ui64LastAdjustment;
		OpenViBE::float64 m_usedSamplingFrequency;

	private:
		OpenViBE::boolean m_bSendPeriodicStimulations;
		OpenViBE::float64 m_Offset;
		OpenViBE::float64 m_Spread;
		OpenViBE::float64 m_MaxDev;
		OpenViBE::float64 m_Pullback;
		OpenViBE::float64 m_Update;
		OpenViBE::uint64 m_Wavetype;

		OpenViBE::float64 m_FreezeFrequency;
		OpenViBE::float64 m_FreezeDuration;
		OpenViBE::uint64 m_NextFreezeTime;

		std::random_device m_rd;
		std::mt19937 m_gen;
	};
};

#endif // __OpenViBE_AcquisitionServer_CDriverSimulatedDeviator_H__
