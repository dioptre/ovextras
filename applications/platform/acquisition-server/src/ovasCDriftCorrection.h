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

		// The main functions to estimate and correct drift
		virtual OpenViBE::boolean estimateDrift(const OpenViBE::uint64 ui64NewSamples);
		// i64correction, ui64TotalSamples, vPendingBuffers and  oPendingStimulationSet may be modified by the correction.
		virtual OpenViBE::boolean correctDrift(OpenViBE::int64 i64Correction, OpenViBE::uint64& ui64TotalSamples, 
					std::vector < std::vector < OpenViBE::float32 > >& vPendingBuffers, OpenViBE::CStimulationSet& oPendingStimulationSet,
					const std::vector < OpenViBE::float32 >& vPaddingBuffer );
		
		// Status functions
		virtual OpenViBE::boolean isActive(void) const { return m_bIsActive; };
		virtual void printStats(void) const;

		// Result getters
		virtual OpenViBE::float64 getDrift(void) const;		                		// current drift, in ms.
		virtual OpenViBE::float64 getDriftTooFastMax(void) const;                   // maximum drift observed, in ms. (driver gave too many samples)
		virtual OpenViBE::float64 getDriftTooSlowMax(void) const;		            // maximum drift observed, in ms. (driver gave too few samples)
		virtual OpenViBE::int64 getDriftSampleCount(void) const;	                // current drift, in samples
		virtual OpenViBE::int64 getSuggestedDriftCorrectionSampleCount(void) const; // number of samples to correct drift with
		virtual OpenViBE::int64 getDriftToleranceSampleCount(void) const { return m_i64DriftToleranceSampleCount; }

		// Parameter getters and setters
		virtual OpenViBE::int64 getInnerLatencySampleCount(void) const;
		virtual OpenViBE::boolean setInnerLatencySampleCount(OpenViBE::int64 i64SampleCount);

		OpenViBEAcquisitionServer::EDriftCorrectionPolicy getDriftCorrectionPolicy(void) const;		
		OpenViBE::CString getDriftCorrectionPolicyStr(void) const;
		OpenViBE::boolean setDriftCorrectionPolicy(OpenViBEAcquisitionServer::EDriftCorrectionPolicy eDriftCorrectionPolicy);

		OpenViBE::uint64 getDriftToleranceDuration(void) const;
		OpenViBE::boolean setDriftToleranceDuration(OpenViBE::uint64 ui64DriftToleranceDuration);

		OpenViBE::uint64 getJitterEstimationCountForDrift(void) const;
		OpenViBE::boolean setJitterEstimationCountForDrift(OpenViBE::uint64 ui64JitterEstimationCountForDrift);

	protected:

		void reset(void);

		const OpenViBE::Kernel::IKernelContext& m_rKernelContext;

		// State of the drift correction
		OpenViBE::boolean m_bInitialized;
		OpenViBE::boolean m_bStarted;
		OpenViBE::boolean m_bIsActive;

		// Parameters
		OpenViBEAcquisitionServer::EDriftCorrectionPolicy m_eDriftCorrectionPolicy;
		OpenViBE::uint64 m_ui64DriftToleranceDuration;
		OpenViBE::int64 m_i64DriftToleranceSampleCount;
		OpenViBE::int64 m_i64InnerLatencySampleCount;
		OpenViBE::uint64 m_ui64JitterEstimationCountForDrift;
		OpenViBE::uint32 m_ui32SamplingFrequency;

		// Results
		OpenViBE::uint64 m_ui64ReceivedSampleCount;
		OpenViBE::uint64 m_ui64CorrectedSampleCount;
		OpenViBE::float64 m_f64DriftEstimate;				// In subsample accuracy, e.g. 1.2 samples.
		OpenViBE::float64 m_f64DriftEstimateTooFastMax;		// maximum over time
		OpenViBE::float64 m_f64DriftEstimateTooSlowMax;		// minumum over time

		// Stats
		OpenViBE::int64 m_i64DriftCorrectionSampleCountAdded;
		OpenViBE::int64 m_i64DriftCorrectionSampleCountRemoved;

		// Timekeeping
		OpenViBE::uint64 m_ui64StartTime;
		OpenViBE::uint64 m_ui64LastEstimationTime;

		// Jitter estimation buffer. Each entry is the number of samples deviated from the expectation
		// between the current time and the last measurement.
		// The average of the buffer items is the current aggregated drift estimate (in samples), convertable to ms by getDrift().
		std::list < OpenViBE::int64 > m_vJitterSampleCount;
	};
};

#endif // __OpenViBE_AcquisitionServer_CDriftCorrection_H__
