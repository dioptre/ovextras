
// @note in this code, ITimeArithmetics::secondsToTime() and ITimeArithmetics::timeToSeconds() 
// are sometimes used simply to convert between floating and fixed point even if the units are not seconds.
//
#include "ovasCDriftCorrection.h"

#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>

#include <ovp_global_defines.h>

#include <system/ovCTime.h>
#include <cmath> // std::abs
#include <sstream>
#include <iomanip>
#include <algorithm> // std::min, std::max

#define boolean OpenViBE::boolean

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;

//___________________________________________________________________//
//                                                                   //

CDriftCorrection::CDriftCorrection(const IKernelContext& rKernelContext)
	: m_rKernelContext(rKernelContext)
	,m_bStarted(false)
	,m_eDriftCorrectionPolicy(DriftCorrectionPolicy_DriverChoice)
{

	CString l_sDriftCorrectionPolicy=m_rKernelContext.getConfigurationManager().expand("${AcquisitionServer_DriftCorrectionPolicy}");
	if(l_sDriftCorrectionPolicy==CString("Forced"))
	{
		this->setDriftCorrectionPolicy(DriftCorrectionPolicy_Forced);
	}
	else if(l_sDriftCorrectionPolicy==CString("Disabled"))
	{
		this->setDriftCorrectionPolicy(DriftCorrectionPolicy_Disabled);
	}
	else
	{
		this->setDriftCorrectionPolicy(DriftCorrectionPolicy_DriverChoice);
	}

	this->setDriftToleranceDurationMs(m_rKernelContext.getConfigurationManager().expandAsUInteger("${AcquisitionServer_DriftToleranceDuration}", 5));
	this->setJitterEstimationCountForDrift(m_rKernelContext.getConfigurationManager().expandAsUInteger("${AcquisitionServer_JitterEstimationCountForDrift}", 128));
	
	m_ui64InitialSkipPeriod = m_rKernelContext.getConfigurationManager().expandAsUInteger("${AcquisitionServer_DriftInitialSkipPeriodMs}", 0);
	m_ui64InitialSkipPeriod = (m_ui64InitialSkipPeriod << 32) / 1000; // ms to fixed point sec

	reset();
}

CDriftCorrection::~CDriftCorrection(void)
{

}

boolean CDriftCorrection::start(uint32 ui32SamplingFrequency, uint64 ui64StartTime)
{
	if(ui32SamplingFrequency==0)
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Drift correction doesn't support sampling rate of 0.\n";
		return false;
	}

	reset();

	m_ui32SamplingFrequency = ui32SamplingFrequency;
	m_i64DriftToleranceSampleCount=(m_ui64DriftToleranceDurationMs * m_ui32SamplingFrequency) / 1000;

	m_ui64StartTime = ui64StartTime + m_ui64InitialSkipPeriod;

	m_ui64LastEstimationTime = ui64StartTime;

	m_rKernelContext.getLogManager() << LogLevel_Trace << "Drift correction is set to ";
	switch(m_eDriftCorrectionPolicy)
	{
		default:
		case DriftCorrectionPolicy_DriverChoice: m_rKernelContext.getLogManager() << CString("DriverChoice") << "\n"; break;
		case DriftCorrectionPolicy_Forced:       m_rKernelContext.getLogManager() << CString("Forced") << "\n"; break;
		case DriftCorrectionPolicy_Disabled:     m_rKernelContext.getLogManager() << CString("Disabled") << "\n"; break;
	};

	m_rKernelContext.getLogManager() << LogLevel_Trace << "Driver monitoring drift estimation on " << m_ui64JitterEstimationCountForDrift << " jitter measures\n";
	m_rKernelContext.getLogManager() << LogLevel_Trace << "Driver monitoring drift tolerance set to " << m_ui64DriftToleranceDurationMs << " milliseconds - eq " << m_i64DriftToleranceSampleCount << " samples\n";

	m_bStarted = true;

	return true;
}

void CDriftCorrection::stop(void)
{
	m_bStarted = false;
	m_bIsActive = false;
}

void CDriftCorrection::reset(void)
{
	m_vJitterEstimate.clear();

	m_f64ReceivedSampleCount = 0;
	m_f64CorrectedSampleCount = 0;
	m_i64InnerLatencySampleCount = 0;

	m_f64DriftEstimate = 0;
	m_f64DriftEstimateTooSlowMax = 0;
	m_f64DriftEstimateTooFastMax = 0;
	m_i64DriftCorrectionSampleCountAdded=0;
	m_i64DriftCorrectionSampleCountRemoved=0;

	m_ui64DriftCorrectionCount = 0;

	// Don't know the sampling rate yet, so cannot put a good value. Put something.
	m_i64DriftToleranceSampleCount = 1;

	m_bIsActive = false;
	m_bInitialSkipPeriodPassed = (m_ui64InitialSkipPeriod<=0 ? true : false);
}


//___________________________________________________________________//
//                                                                   //


void CDriftCorrection::printStats(void) const
{
	if(!m_bStarted)
	{
		m_rKernelContext.getLogManager() << LogLevel_Info << "Drift correction is stopped, no statistics were collected.\n";
		return;
	}

	const uint64 l_ui64ElapsedTime = m_ui64LastEstimationTime-m_ui64StartTime;
	const float64 l_f64ElapsedTime = ITimeArithmetics::timeToSeconds(l_ui64ElapsedTime);

	const uint64 l_ui64TheoreticalSampleCountFixedPoint = m_ui32SamplingFrequency * l_ui64ElapsedTime;
	const float64 l_f64TheoreticalSampleCount = ITimeArithmetics::timeToSeconds(l_ui64TheoreticalSampleCountFixedPoint);

	const float64 l_f64AddedRatio  = (l_f64TheoreticalSampleCount ? (m_i64DriftCorrectionSampleCountAdded   / static_cast<float64>(l_f64TheoreticalSampleCount)) : 0);
	const float64 l_f64RemovedRatio= (l_f64TheoreticalSampleCount ? (m_i64DriftCorrectionSampleCountRemoved / static_cast<float64>(l_f64TheoreticalSampleCount)) : 0);

	const uint64 l_ui64DriftToleranceDurationMs = getDriftToleranceDurationMs();
	const float64 l_f64DriftRatio=getDriftMs()/static_cast<float64>(l_ui64DriftToleranceDurationMs);
	const float64 l_f64DriftRatioTooFastMax = getDriftTooFastMax()/static_cast<float64>(l_ui64DriftToleranceDurationMs);
	const float64 l_f64DriftRatioTooSlowMax = getDriftTooSlowMax()/static_cast<float64>(l_ui64DriftToleranceDurationMs);

	const float64 l_f64EstimatedSamplingRate = m_f64ReceivedSampleCount / l_f64ElapsedTime;
	const float64 l_f64DeviationPercent =  100.0*(l_f64EstimatedSamplingRate / static_cast<float64>(m_ui32SamplingFrequency));

	
	if( l_f64DriftRatioTooFastMax > 1.0 || l_f64DriftRatioTooSlowMax > 1.0 || std::abs(l_f64DriftRatio) > 1.0 )
	{
		// Drift tolerance was exceeded, print some stats

		m_rKernelContext.getLogManager() << LogLevel_Info << "Stats after " << l_f64ElapsedTime << " second session of " << m_ui32SamplingFrequency << "hz sampling (declared rate),\n";

		m_rKernelContext.getLogManager() << LogLevel_Info << "  Estimate : Driver samples at " << std::round(l_f64EstimatedSamplingRate*10.0)/10.0
			<< "hz (" << std::round(l_f64DeviationPercent*10.0)/10.0 << "% of declared)\n";

		m_rKernelContext.getLogManager() << LogLevel_Info << "  Received : " << m_f64ReceivedSampleCount << " samples\n";
		m_rKernelContext.getLogManager() << LogLevel_Info << "  Expected : " << l_f64TheoreticalSampleCount << " samples\n";
		m_rKernelContext.getLogManager() << LogLevel_Info << "  Returned : " << m_f64CorrectedSampleCount << " samples " 
			<< (m_eDriftCorrectionPolicy == DriftCorrectionPolicy_Disabled ? "(drift correction disabled)" : "(after drift correction)") << "\n";

		m_rKernelContext.getLogManager() << LogLevel_Info << "  Added    : " << m_i64DriftCorrectionSampleCountAdded << " samples (" << l_f64AddedRatio << "%)\n";
		m_rKernelContext.getLogManager() << LogLevel_Info << "  Removed  : " << m_i64DriftCorrectionSampleCountRemoved << " samples (" << l_f64RemovedRatio << "%)\n";
		m_rKernelContext.getLogManager() << LogLevel_Info << "  Operated : " << m_ui64DriftCorrectionCount << " times (interventions)\n";

		m_rKernelContext.getLogManager() << LogLevel_Info << "Estimated drift (tolerance = " << l_ui64DriftToleranceDurationMs << "ms),\n";

		m_rKernelContext.getLogManager() << (l_f64DriftRatioTooSlowMax > 1.0 ? LogLevel_Warning : LogLevel_Info)
			<< "  Slow peak  : " << -m_f64DriftEstimateTooSlowMax << " samples (" << getDriftTooSlowMax() << "ms late, " << 100*l_f64DriftRatioTooSlowMax << "% of tol.)\n";
	
		m_rKernelContext.getLogManager() << (l_f64DriftRatioTooFastMax > 1.0 ? LogLevel_Warning : LogLevel_Info)
			<< "  Fast peak  : " << m_f64DriftEstimateTooFastMax << " samples (" << getDriftTooFastMax() << "ms early, " << 100*l_f64DriftRatioTooFastMax << "% of tol.)\n";

		m_rKernelContext.getLogManager() << (std::abs(l_f64DriftRatio) > 1.0 ? LogLevel_Warning : LogLevel_Info) 
			<< "  Last estim : " << m_f64DriftEstimate << " samples (" << getDriftMs() << "ms, " << 100*l_f64DriftRatio << "% of tol., "
			<< std::round(100.0* (getDriftMs()/1000.0) / l_f64ElapsedTime * 10.0) / 10.0 << "% of session length)"
			<< (m_eDriftCorrectionPolicy == DriftCorrectionPolicy_Disabled ? "" : ", after corr.")
			<< "\n";

		const float64 l_f64RemainingDriftCount = m_f64CorrectedSampleCount - l_f64TheoreticalSampleCount;
		const float64 l_f64RemainingDriftMs = 1000.0 * l_f64RemainingDriftCount / static_cast<float64>(m_ui32SamplingFrequency);
		m_rKernelContext.getLogManager() << (std::abs(l_f64RemainingDriftMs) > l_ui64DriftToleranceDurationMs ? LogLevel_Warning : LogLevel_Info)
			<< "  Remaining  : " << l_f64RemainingDriftCount << " samples (" << l_f64RemainingDriftMs << "ms, " << 100*l_f64RemainingDriftMs/l_ui64DriftToleranceDurationMs << "% of tol., "
			<< std::round(100.0* (l_f64RemainingDriftMs/1000.0) / l_f64ElapsedTime * 10.0) / 10.0 << "% of session length)"
			<< (m_eDriftCorrectionPolicy == DriftCorrectionPolicy_Disabled ? "" : ", after corr.")
			<< "\n";

		if( m_eDriftCorrectionPolicy == DriftCorrectionPolicy_DriverChoice && m_i64DriftCorrectionSampleCountAdded==0 && m_i64DriftCorrectionSampleCountRemoved==0)
		{
			m_rKernelContext.getLogManager() << LogLevel_ImportantWarning << "  The driver did not try to correct the drift. This may be a feature of the driver.\n";
		}
	}

}



//___________________________________________________________________//
//                                                                   //

OpenViBE::float64 CDriftCorrection::computeJitter(const OpenViBE::uint64 ui64CurrentTime)
{
	// Compute the jitter. To get a bit cleaner code, instead of basing jitter estimate
	// on difference of estimated sample counts, we compute the diffs in the elapsed 
	// and the expected time based on the amount of samples received.
	const uint64 l_ui64ExpectedTime = m_ui64StartTime
		+ ITimeArithmetics::secondsToTime(m_f64CorrectedSampleCount) / static_cast<uint64>(m_ui32SamplingFrequency);

	float64 l_f64TimeDiff; // time in seconds that our expectation differs from the measured clock
	if(l_ui64ExpectedTime>=ui64CurrentTime)
	{
		// The driver is early
		l_f64TimeDiff = ITimeArithmetics::timeToSeconds(l_ui64ExpectedTime-ui64CurrentTime);
	}
	else
	{
		// The driver is late
		l_f64TimeDiff = -ITimeArithmetics::timeToSeconds(ui64CurrentTime-l_ui64ExpectedTime);
	}

	// Jitter in fractional samples
	const float64 l_f64Jitter = l_f64TimeDiff*m_ui32SamplingFrequency + static_cast<float64>(m_i64InnerLatencySampleCount);

	return l_f64Jitter;
}

#include <iostream>

boolean CDriftCorrection::estimateDrift(const uint64 ui64NewSamples)
{
	if(!m_bStarted) {
		m_rKernelContext.getLogManager() << LogLevel_Error << "Drift correction: estimateDrift() called before start()\n";
		return false;
	}

	const uint64 l_ui64CurrentTime = System::Time::zgetTime();

	if (l_ui64CurrentTime < m_ui64StartTime)
	{
		// We can ignore some sets of samples for the driver to stabilize. For example, 
		// a delay in delivering the first set would cause a permanent drift (offset) unless corrected. 
		// Yet this offset would be totally harmless to any client connecting to AS after first 
		// set of samples have been received, and if drift correction is disabled, it would 
		// stay there in the measure. 
		//
		// With conf token AcquisitionServer_DriftInitialSkipPeriodMs set to 0 no sets will be skipped.

		return true;
	}
	else
	{
		if (!m_bInitialSkipPeriodPassed)
		{
			// The next call will be the first estimation
			m_bInitialSkipPeriodPassed = true;
			m_ui64StartTime = l_ui64CurrentTime;
			m_ui64LastEstimationTime = l_ui64CurrentTime;
			return true;
		}
	}

	m_f64ReceivedSampleCount += ui64NewSamples;		// How many samples have arrived from the driver
	m_f64CorrectedSampleCount += ui64NewSamples;		// The "corrected" amount of samples

//	m_rKernelContext.getLogManager() << LogLevel_Info << "Drift measured at " << ITimeArithmetics::timeToSeconds(l_ui64CurrentTime - m_ui64StartTime) * 1000 << "ms.\n";

	const float64 l_f64Jitter = computeJitter(l_ui64CurrentTime);

	m_vJitterEstimate.push_back(l_f64Jitter);
	if(m_vJitterEstimate.size() > m_ui64JitterEstimationCountForDrift)
	{
		m_vJitterEstimate.pop_front();
	}

	// Estimate the drift after we have a full buffer of jitter estimates
	if(m_vJitterEstimate.size() == m_ui64JitterEstimationCountForDrift)
	{
		float64  l_f64NewDriftEstimate = 0;

		for(list<float64>::iterator j=m_vJitterEstimate.begin(); j!=m_vJitterEstimate.end(); j++)
		{
			l_f64NewDriftEstimate+=*j;
		}

		m_f64DriftEstimate = l_f64NewDriftEstimate / m_ui64JitterEstimationCountForDrift;

		if(m_f64DriftEstimate>0) {
			m_f64DriftEstimateTooFastMax = std::max<float64>(m_f64DriftEstimateTooFastMax, m_f64DriftEstimate);
		} else {
			m_f64DriftEstimateTooSlowMax = std::max<float64>(m_f64DriftEstimateTooSlowMax, -m_f64DriftEstimate);
		}

		if( std::abs(m_f64DriftEstimate) > this->getDriftToleranceSampleCount() )
		{
			m_rKernelContext.getLogManager() << LogLevel_Debug << 
				"At " << ITimeArithmetics::timeToSeconds(l_ui64CurrentTime)*1000 << "ms,"
				<< " Acq mon [drift:" << getDriftMs() << "][jitter:" << l_f64Jitter << "] samples,"
				<< " dsc " << getDriftSampleCount() 
				<< " (inner lat. samples " << m_i64InnerLatencySampleCount << ")\n";
		}
	}

	m_ui64LastEstimationTime = l_ui64CurrentTime;

	return true;
}


boolean CDriftCorrection::correctDrift(int64 i64Correction, uint64& ui64TotalSamples, std::deque < std::vector < OpenViBE::float32 > >& vPendingBuffer, OpenViBE::CStimulationSet& oPendingStimulationSet, 
	const std::vector < OpenViBE::float32 >& vPaddingBuffer)
{
	if(!m_bStarted)
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Drift correction: correctDrift() called before start()\n";
		return false;
	}

	if(m_eDriftCorrectionPolicy == DriftCorrectionPolicy_Disabled)
	{
		// Not an error, we just don't correct
		return false;
	}

	m_bIsActive = true;

	if(i64Correction == 0)
	{
		return true;
	}

	if(ui64TotalSamples != static_cast<uint64>(m_f64CorrectedSampleCount)) {
		m_rKernelContext.getLogManager() << LogLevel_Warning << "Server and drift correction class disagree on the number of samples\n";
	}

	const uint64 l_ui64ElapsedTime=System::Time::zgetTime()-m_ui64StartTime;
	const float64 l_f64ElapsedTime=ITimeArithmetics::timeToSeconds(l_ui64ElapsedTime);

	m_rKernelContext.getLogManager() << LogLevel_Trace << "At time " << l_f64ElapsedTime << "s : Correcting drift by " << i64Correction << " samples\n";

	if(i64Correction > 0)
	{
		for(int64 i=0; i<i64Correction; i++)
		{
			vPendingBuffer.push_back(vPaddingBuffer);
		}

		const uint64 l_ui64TimeOfIncorrect = ITimeArithmetics::secondsToTime(m_f64CorrectedSampleCount - 1) / static_cast<uint64>(m_ui32SamplingFrequency);
		const uint64 l_ui64DurationOfIncorrect = ITimeArithmetics::sampleCountToTime(m_ui32SamplingFrequency, i64Correction);
		const uint64 l_ui64TimeOfCorrect = ITimeArithmetics::secondsToTime(m_f64CorrectedSampleCount - 1 + i64Correction) / static_cast<uint64>(m_ui32SamplingFrequency);
		oPendingStimulationSet.appendStimulation(OVTK_StimulationId_AddedSamplesBegin, l_ui64TimeOfIncorrect, l_ui64DurationOfIncorrect);
		oPendingStimulationSet.appendStimulation(OVTK_StimulationId_AddedSamplesEnd,   l_ui64TimeOfCorrect, 0);

		m_f64DriftEstimate+=i64Correction;

		m_f64CorrectedSampleCount+= i64Correction;
		m_i64DriftCorrectionSampleCountAdded+= i64Correction;
		m_ui64DriftCorrectionCount++;
	}
	else if(i64Correction < 0)
	{
		const uint64 l_ui64SamplesToRemove=std::min<uint64>( uint64(-i64Correction), uint64(vPendingBuffer.size()) );

		vPendingBuffer.erase(vPendingBuffer.begin()+vPendingBuffer.size()-(int)l_ui64SamplesToRemove, vPendingBuffer.begin()+vPendingBuffer.size());

		const uint64 l_ui64LastSampleDate = ITimeArithmetics::secondsToTime(m_f64CorrectedSampleCount-l_ui64SamplesToRemove) / static_cast<uint64>(m_ui32SamplingFrequency);
		for(uint32 i=0; i<oPendingStimulationSet.getStimulationCount(); i++)
		{
			if(oPendingStimulationSet.getStimulationDate(i) > l_ui64LastSampleDate)
			{
				oPendingStimulationSet.setStimulationDate(i, l_ui64LastSampleDate);
			}
		}

		oPendingStimulationSet.appendStimulation(OVTK_StimulationId_RemovedSamples, l_ui64LastSampleDate, 0);

		m_f64DriftEstimate-=l_ui64SamplesToRemove;

		m_f64CorrectedSampleCount-=l_ui64SamplesToRemove;
		m_i64DriftCorrectionSampleCountRemoved+=l_ui64SamplesToRemove;
		m_ui64DriftCorrectionCount++;
	}

	// correct the jitter estimate to match the correction we made. For example, if we had
	// a jitter estimate of [-1,-1,-1,...] and correct the drift by adding 1 sample, the new
	// estimate should become [0,0,0,0,...] with relation to the adjustment. This changes jitter estimates
	// in the past. The other alternative would be to reset the estimate. Adjusting 
	// might keep some detail the reset would lose.
	for(list<float64>::iterator j=m_vJitterEstimate.begin(); j!=m_vJitterEstimate.end(); j++)
	{
		(*j)+=i64Correction;
	}

	ui64TotalSamples = static_cast<uint64>(m_f64CorrectedSampleCount);

	return true;
}

float64 CDriftCorrection::getDriftMs(void) const
{
	if(m_ui32SamplingFrequency==0) 
	{
		return 0;
	}

	return m_f64DriftEstimate*1000.0/m_ui32SamplingFrequency;
}

float64 CDriftCorrection::getDriftTooSlowMax(void) const
{
	if(m_ui32SamplingFrequency==0) 
	{
		return 0;
	}

	return m_f64DriftEstimateTooSlowMax*1000./m_ui32SamplingFrequency;
}

float64 CDriftCorrection::getDriftTooFastMax(void) const
{
	if(m_ui32SamplingFrequency==0) 
	{
		return 0;
	}

	return m_f64DriftEstimateTooFastMax*1000.0/m_ui32SamplingFrequency;
}

int64 CDriftCorrection::getDriftSampleCount(void) const
{
	return static_cast<int64>(m_f64DriftEstimate);
}

// Note that we cannot do actual correction with subsample accuracy, so here we truncate the drift estimate to integer in getDriftSampleCount().
int64 CDriftCorrection::getSuggestedDriftCorrectionSampleCount(void) const
{
	const float64 l_f64DriftToleranceMs = static_cast<float64>(this->getDriftToleranceDurationMs());
	const float64 l_f64CurrentDriftMs = this->getDriftMs();
	
	if(std::abs(l_f64CurrentDriftMs) > l_f64DriftToleranceMs)
	{
		// The correction is always to the opposite direction of the drift
		return -this->getDriftSampleCount();
	}

	return 0;
}

boolean CDriftCorrection::setInnerLatencySampleCount(OpenViBE::int64 i64SampleCount)
{
	m_i64InnerLatencySampleCount=i64SampleCount;
	return true;
}

int64 CDriftCorrection::getInnerLatencySampleCount(void) const
{
	return m_i64InnerLatencySampleCount;
}

// ____________________________________________________________________________
//




EDriftCorrectionPolicy CDriftCorrection::getDriftCorrectionPolicy(void) const
{
	return m_eDriftCorrectionPolicy;
}



CString CDriftCorrection::getDriftCorrectionPolicyStr(void) const
{
	switch (m_eDriftCorrectionPolicy)
	{
		case DriftCorrectionPolicy_Disabled:
			return CString("Disabled");
		case DriftCorrectionPolicy_DriverChoice:
			return CString("DriverChoice");
		case DriftCorrectionPolicy_Forced:
			return CString("Forced");
		default :
			return CString("N/A");
	}
}

uint64 CDriftCorrection::getDriftToleranceDurationMs(void) const
{
	return m_ui64DriftToleranceDurationMs;
}

uint64 CDriftCorrection::getJitterEstimationCountForDrift(void) const
{
	return m_ui64JitterEstimationCountForDrift;
}

boolean CDriftCorrection::setDriftCorrectionPolicy(EDriftCorrectionPolicy eDriftCorrectionPolicy)
{
	m_eDriftCorrectionPolicy=eDriftCorrectionPolicy;
	return true;
}



boolean CDriftCorrection::setDriftToleranceDurationMs(uint64 ui64DriftToleranceDurationMs)
{
	if(ui64DriftToleranceDurationMs==0)
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Minimum accepted drift tolerance limit is 1ms\n";
		m_ui64DriftToleranceDurationMs = 1;
		return true;
	}

	m_ui64DriftToleranceDurationMs=ui64DriftToleranceDurationMs;
	return true;
}

boolean CDriftCorrection::setJitterEstimationCountForDrift(uint64 ui64JitterEstimationCountForDrift)
{
	m_ui64JitterEstimationCountForDrift=ui64JitterEstimationCountForDrift;
	return true;
}




