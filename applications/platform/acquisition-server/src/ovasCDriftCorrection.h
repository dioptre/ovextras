#ifndef __OpenViBE_AcquisitionServer_CDriftCorrection_H__
#define __OpenViBE_AcquisitionServer_CDriftCorrection_H__

#include "ovas_base.h"

#include <vector>
#include <list>

namespace OpenViBEAcquisitionServer
{
	typedef enum
	{
		DriftCorrectionPolicy_DriverChoice=0,
		DriftCorrectionPolicy_Forced,
		DriftCorrectionPolicy_Disabled,
	} EDriftCorrectionPolicy;

	/*
	 * \class CDriftCorrection
	 * \author Jussi T. Lindgren / Inria
	 * 
	 * \brief A refactoring of the old drift correction code that was originally mixed into the acquisition server main code.
	 *
	 */
	class CDriftCorrection
	{
	public:

		CDriftCorrection(const OpenViBE::Kernel::IKernelContext& rKernelContext);
		virtual ~CDriftCorrection(void);

		// To start a new drift estimation and stop it.
		virtual OpenViBE::boolean start(OpenViBE::uint32 ui32SamplingFrequency, OpenViBE::uint64 ui64StartTime);
		virtual void stop(void);

		// Estimate the drift
		// 
		// \param ui64NewSamples [in] : How many samples the driver returned
		virtual OpenViBE::boolean estimateDrift(const OpenViBE::uint64 ui64NewSamples);

		// Request a drift correction
		//
		// \param i64correction [in] : the number of samples to correct
		// \param ui64TotalSamples [out] : Number of total samples after correction for drift
		// \param vPendingBuffers [in/out] : The sample buffer to be corrected
		// \param oPendingStimulationSet [in/out] : The stimulation set to be realigned
		// \param vPaddingBuffer[in] : The sample to repeatedly add if i64Correction > 0
		virtual OpenViBE::boolean correctDrift(const OpenViBE::int64 i64Correction, OpenViBE::uint64& ui64TotalSamples, 
					std::vector < std::vector < OpenViBE::float32 > >& vPendingBuffers, OpenViBE::CStimulationSet& oPendingStimulationSet,
					const std::vector < OpenViBE::float32 >& vPaddingBuffer );
		
		// Status functions
		virtual OpenViBE::boolean isActive(void) const { return m_bIsActive; };
		// Prints various statistics but only if drift tolerance was exceeded
		virtual void printStats(void) const;

		// Result getters
		virtual OpenViBE::float64 getDriftMs(void) const;		              		// current drift, in ms.
		virtual OpenViBE::float64 getDriftTooFastMax(void) const;                   // maximum positive drift observed, in ms. (driver gave too many samples)
		virtual OpenViBE::float64 getDriftTooSlowMax(void) const;		            // maximum negative drift observed, in ms. (driver gave too few samples)
		virtual OpenViBE::int64 getDriftSampleCount(void) const;	                // current drift, in samples
		virtual OpenViBE::int64 getSuggestedDriftCorrectionSampleCount(void) const; // number of samples to correct drift with
		virtual OpenViBE::int64 getDriftToleranceSampleCount(void) const { return m_i64DriftToleranceSampleCount; }

		// Parameter getters and setters
		virtual OpenViBE::int64 getInnerLatencySampleCount(void) const;
		virtual OpenViBE::boolean setInnerLatencySampleCount(OpenViBE::int64 i64SampleCount);

		OpenViBEAcquisitionServer::EDriftCorrectionPolicy getDriftCorrectionPolicy(void) const;		
		OpenViBE::CString getDriftCorrectionPolicyStr(void) const;
		OpenViBE::boolean setDriftCorrectionPolicy(OpenViBEAcquisitionServer::EDriftCorrectionPolicy eDriftCorrectionPolicy);

		OpenViBE::uint64 getDriftToleranceDurationMs(void) const;					// In milliseconds
		OpenViBE::boolean setDriftToleranceDurationMs(OpenViBE::uint64 ui64DriftToleranceDuration);

		OpenViBE::uint64 getJitterEstimationCountForDrift(void) const;
		OpenViBE::boolean setJitterEstimationCountForDrift(OpenViBE::uint64 ui64JitterEstimationCountForDrift);

	protected:

		void reset(void);

		// Computes jitter in fractional samples
		OpenViBE::float64 computeJitter(const OpenViBE::uint64 ui64CurrentTime);

		const OpenViBE::Kernel::IKernelContext& m_rKernelContext;

		// State of the drift correction
		OpenViBE::boolean m_bInitialized;
		OpenViBE::boolean m_bStarted;
		OpenViBE::boolean m_bIsActive;

		// Parameters
		OpenViBEAcquisitionServer::EDriftCorrectionPolicy m_eDriftCorrectionPolicy;
		OpenViBE::uint64 m_ui64DriftToleranceDurationMs;
		OpenViBE::int64 m_i64DriftToleranceSampleCount;
		OpenViBE::int64 m_i64InnerLatencySampleCount;
		OpenViBE::uint64 m_ui64JitterEstimationCountForDrift;
		OpenViBE::uint32 m_ui32SamplingFrequency;

		// Results
		OpenViBE::uint64 m_ui64ReceivedSampleCount;
		OpenViBE::uint64 m_ui64CorrectedSampleCount;
		OpenViBE::float64 m_f64DriftEstimate;				// In subsample accuracy, e.g. 1.2 samples.
		OpenViBE::float64 m_f64DriftEstimateTooFastMax;		// maximum over time
		OpenViBE::float64 m_f64DriftEstimateTooSlowMax;		// minimum over time

		// Stats
		OpenViBE::int64 m_i64DriftCorrectionSampleCountAdded;
		OpenViBE::int64 m_i64DriftCorrectionSampleCountRemoved;

		// Timekeeping
		OpenViBE::uint64 m_ui64StartTime;
		OpenViBE::uint64 m_ui64LastEstimationTime;

		// Jitter estimation buffer. Each entry is the difference between the expected number of 
		// samples and the received number of samples per each call to estimateDrift(). The buffer has subsample accuracy to avoid rounding errors.
		// -> The average of the buffer items is the current aggregated drift estimate (in samples), convertable to ms by getDrift().
		std::list < OpenViBE::float64 > m_vJitterEstimate;

	};
};

#endif // __OpenViBE_AcquisitionServer_CDriftCorrection_H__
