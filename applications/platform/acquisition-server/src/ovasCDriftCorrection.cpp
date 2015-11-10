#include "ovasCDriftCorrection.h"

#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>

#include <ovp_global_defines.h>

#include <system/ovCTime.h>
#include <cmath> // std::abs

#define boolean OpenViBE::boolean

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;

// #define OV_DEBUG_DRIFT

// #define OV_ESTIMATE_BASIC
// #define OV_ESTIMATE_LOCAL
#define OV_ESTIMATE_TIMEDIFF_BASED
// #define OV_ESTIMATE_BOOST

#if defined(OV_ESTIMATE_BOOST)
#include "boost/date_time/posix_time/posix_time.hpp"
#endif

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

	this->setDriftToleranceDuration(m_rKernelContext.getConfigurationManager().expandAsUInteger("${AcquisitionServer_DriftToleranceDuration}", 5));
	this->setJitterEstimationCountForDrift(m_rKernelContext.getConfigurationManager().expandAsUInteger("${AcquisitionServer_JitterEstimationCountForDrift}", 128));

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
	m_i64DriftToleranceSampleCount=(m_ui64DriftToleranceDuration * m_ui32SamplingFrequency) / 1000;

	m_ui64StartTime = ui64StartTime;

#if defined(OV_ESTIMATE_BOOST)
	m_oStartTime = boost::posix_time::microsec_clock::local_time();
#endif

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
	m_rKernelContext.getLogManager() << LogLevel_Trace << "Driver monitoring drift tolerance set to " << m_ui64DriftToleranceDuration << " milliseconds - eq " << m_i64DriftToleranceSampleCount << " samples\n";

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

	m_ui64ReceivedSampleCount = 0;
	m_ui64CorrectedSampleCount = 0;
	m_i64InnerLatencySampleCount = 0;

	m_f64DriftEstimate = 0;
	m_f64DriftEstimateTooSlowMax = 0;
	m_f64DriftEstimateTooFastMax = 0;
	m_i64DriftCorrectionSampleCountAdded=0;
	m_i64DriftCorrectionSampleCountRemoved=0;

	// Don't know the sampling rate yet, so cannot put a good value. Put something.
	m_i64DriftToleranceSampleCount = 1;

	m_bIsActive = false;

}



//___________________________________________________________________//
//                                                                   //



//___________________________________________________________________//
//                                                                   //

float64 truncateDecimals(float64 value, uint32 nDecimals)
{
	uint32 tmp=1;
	for(uint32 i=0;i<nDecimals;i++) {
		tmp*=10;
	}
	const int64 trunc = static_cast<int64>(value*tmp);
	const float64 retVal = trunc / static_cast<float64>(tmp);

	return retVal;
}

void CDriftCorrection::printStats(void) const
{
	if(!m_bStarted)
	{
		m_rKernelContext.getLogManager() << LogLevel_Info << "Drift correction is stopped, no statistics collected.\n";
		return;
	}

	const uint64 l_ui64ElapsedTime = m_ui64LastEstimationTime-m_ui64StartTime;
	const float64 l_f64ElapsedTime = ITimeArithmetics::timeToSeconds(l_ui64ElapsedTime);

	const uint64 l_ui64TheoreticalSampleCount= (m_ui32SamplingFrequency * l_ui64ElapsedTime) >> 32;

	const float64 l_f64AddedRatio  = (l_ui64TheoreticalSampleCount ? (m_i64DriftCorrectionSampleCountAdded   / static_cast<float64>(l_ui64TheoreticalSampleCount)) : 0);
	const float64 l_f64RemovedRatio= (l_ui64TheoreticalSampleCount ? (m_i64DriftCorrectionSampleCountRemoved / static_cast<float64>(l_ui64TheoreticalSampleCount)) : 0);

	const uint64 l_ui64DriftToleranceDuration = getDriftToleranceDuration();
	const float64 l_f64DriftRatio=getDrift()/static_cast<float64>(l_ui64DriftToleranceDuration);
	const float64 l_f64DriftRatioTooFastMax = getDriftTooFastMax()/static_cast<float64>(l_ui64DriftToleranceDuration);
	const float64 l_f64DriftRatioTooSlowMax = getDriftTooSlowMax()/static_cast<float64>(l_ui64DriftToleranceDuration);

	m_rKernelContext.getLogManager() << LogLevel_Info << "Stats after " << l_f64ElapsedTime << " seconds of " << m_ui32SamplingFrequency << "hz sampling,\n";
	m_rKernelContext.getLogManager() << LogLevel_Info << "  Received : " << m_ui64ReceivedSampleCount << " samples\n";
	m_rKernelContext.getLogManager() << LogLevel_Info << "  Expected : " << l_ui64TheoreticalSampleCount << " samples\n";
	m_rKernelContext.getLogManager() << LogLevel_Info << "  Returned : " << m_ui64CorrectedSampleCount << " samples " 
		<< (m_eDriftCorrectionPolicy == DriftCorrectionPolicy_Disabled ? "(drift correction disabled)" : "(after drift correction)") << "\n";
	m_rKernelContext.getLogManager() << LogLevel_Info << "  Added    : " << m_i64DriftCorrectionSampleCountAdded << " samples (" << truncateDecimals(l_f64AddedRatio,1) << "%)\n";
	m_rKernelContext.getLogManager() << LogLevel_Info << "  Removed  : " << m_i64DriftCorrectionSampleCountRemoved << " samples (" << truncateDecimals(l_f64RemovedRatio,1) << "%)\n";

	m_rKernelContext.getLogManager() << LogLevel_Info << "Estimated drift (tolerance = " << l_ui64DriftToleranceDuration << "ms),\n";

	m_rKernelContext.getLogManager() << (l_f64DriftRatioTooSlowMax > 1.0 ? LogLevel_Warning : LogLevel_Info)
		<< "  Slow peak  : " << -truncateDecimals(m_f64DriftEstimateTooSlowMax,1) << " samples (" << truncateDecimals(getDriftTooSlowMax(),1) << "ms late, " << truncateDecimals(100*l_f64DriftRatioTooSlowMax,1) << "% of tol.)\n";
	m_rKernelContext.getLogManager() << (l_f64DriftRatioTooFastMax > 1.0 ? LogLevel_Warning : LogLevel_Info)
		<< "  Fast peak  : " << truncateDecimals(m_f64DriftEstimateTooFastMax,1) << " samples (" << truncateDecimals(getDriftTooFastMax(),1) << "ms early, " << truncateDecimals(100*l_f64DriftRatioTooFastMax,1) << "% of tol.)\n";

	m_rKernelContext.getLogManager() << (std::abs(l_f64DriftRatio) > 1.0 ? LogLevel_Warning : LogLevel_Info)
		<< "  Last estim : " << truncateDecimals(m_f64DriftEstimate,1) << " samples (" << truncateDecimals(getDrift(),1) << "ms, " << truncateDecimals(100*l_f64DriftRatio,1) << "% of tol.)"
		<< (m_eDriftCorrectionPolicy == DriftCorrectionPolicy_Disabled ? "" : ", after corr.")
		<< "\n";

	const int64 l_i64RemainingDriftCount = (static_cast<int64>(m_ui64CorrectedSampleCount) - static_cast<int64>(l_ui64TheoreticalSampleCount));
	const float64 l_f64RemainingDriftMs = l_i64RemainingDriftCount / static_cast<float64>(m_ui32SamplingFrequency);
	m_rKernelContext.getLogManager() << (std::abs(l_f64RemainingDriftMs) > l_ui64DriftToleranceDuration ? LogLevel_ImportantWarning : LogLevel_Info)
		<< "  Remaining  : " << l_i64RemainingDriftCount << " samples (" << truncateDecimals(l_f64RemainingDriftMs,1) << "ms, " << truncateDecimals(100*l_f64RemainingDriftMs/l_ui64DriftToleranceDuration,1) << "% of tol.)"
		<< (m_eDriftCorrectionPolicy == DriftCorrectionPolicy_Disabled ? "" : ", after corr.")
		<< "\n";

	if(l_f64DriftRatioTooFastMax > 1.0 || l_f64DriftRatioTooSlowMax > 1.0 || std::abs(l_f64DriftRatio) > 1.0)
	{
		if( m_eDriftCorrectionPolicy == DriftCorrectionPolicy_DriverChoice && m_i64DriftCorrectionSampleCountAdded==0 && m_i64DriftCorrectionSampleCountRemoved==0)
		{
			m_rKernelContext.getLogManager() << LogLevel_ImportantWarning << "  The driver did not try to correct the drift. This may be a feature of the driver.\n";
		}
	}
}



//___________________________________________________________________//
//                                                                   //


boolean CDriftCorrection::estimateDrift(const uint64 ui64NewSamples)
{
	if(!m_bStarted) {
		m_rKernelContext.getLogManager() << LogLevel_Error << "Drift correction: estimateDrift() called before start()\n";
		return false;
	}

	m_ui64ReceivedSampleCount += ui64NewSamples;		// Just kept for statistics
	m_ui64CorrectedSampleCount += ui64NewSamples;		// The "corrected" amount of samples

	const uint64 l_ui64CurrentTime = System::Time::zgetTime();

#if defined(OV_ESTIMATE_BASIC)
	// Global estimate (the correction has memory over the whole play time)
	const uint64 l_ui64ElapsedTime = l_ui64CurrentTime - m_ui64StartTime;
	const uint64 l_ui64ExpectedSampleCountFixedPoint = (m_ui32SamplingFrequency * l_ui64ElapsedTime); // fractional
	const uint64 l_ui64CorrectedSampleCountFixedPoint = (m_ui64CorrectedSampleCount << 32);

	// Do some kludge arithmetic to get the signed jitter to a float (32:32 fixedpoint is unsigned). We try to
	// operate on the fixed point arithmetic as long as possible and only cast the diff to float in the end.
	float64 l_f64Jitter = static_cast<float64>(m_i64InnerLatencySampleCount);
	if(l_ui64ExpectedSampleCountFixedPoint >= l_ui64CorrectedSampleCountFixedPoint)
	{
		const uint64 l_ui64SampleCountDiff = l_ui64ExpectedSampleCountFixedPoint - l_ui64CorrectedSampleCountFixedPoint;
		l_f64Jitter += -ITimeArithmetics::timeToSeconds(l_ui64SampleCountDiff);
	}
	else
	{
		const uint64 l_ui64SampleCountDiff = l_ui64CorrectedSampleCountFixedPoint - l_ui64ExpectedSampleCountFixedPoint;
		l_f64Jitter += ITimeArithmetics::timeToSeconds(l_ui64SampleCountDiff);
	}

#if defined(OV_DEBUG_DRIFT)
	static int calls=0;
	if(calls++ % 100 == 0) {
		m_rKernelContext.getLogManager() << LogLevel_Info << "At " << ITimeArithmetics::timeToSeconds(l_ui64ElapsedTime)*1000 
			<< "ms r=" << m_ui64ReceivedSampleCount 
			<< " e=" << ITimeArithmetics::timeToSeconds(l_ui64ExpectedSampleCountFixedPoint) 
			// << ", i=" << m_i64InnerLatencySampleCount 
			<< " j=" << l_f64Jitter << " d=" << m_f64DriftEstimate 
			<< " sk=" << m_ui64ReceivedSampleCount / ITimeArithmetics::timeToSeconds(l_ui64ExpectedSampleCountFixedPoint) // skew between the clocks
			// << " m=" << (m_f64DriftEstimateTooFastMax > m_f64DriftEstimateTooSlowMax ? m_f64DriftEstimateTooFastMax : -m_f64DriftEstimateTooSlowMax)
			<< "\n";
	}
#endif

#elif defined(OV_ESTIMATE_LOCAL)
	// Just estimate from this chunk, the correction has no memory of events earlier than whats in the jitter buffer.
	const uint64 l_ui64ElapsedTime = l_ui64CurrentTime - m_ui64LastEstimationTime;
	const float64 l_f64ExpectedSampleCount = ITimeArithmetics::timeToSeconds(m_ui32SamplingFrequency * l_ui64ElapsedTime);
	const float64 l_f64NewSampleCount = static_cast<float64>(ui64NewSamples);

	const float64 l_f64Jitter = l_f64NewSampleCount - l_f64ExpectedSampleCount + m_i64InnerLatencySampleCount;

#if defined(OV_DEBUG_DRIFT)
	static int calls=0;
	if(calls++ % 4 == 0) {
		m_rKernelContext.getLogManager() << LogLevel_Info << "At " << ITimeArithmetics::timeToSeconds(l_ui64ElapsedTime)*1000 
			<< "ms r=" << m_ui64ReceivedSampleCount 
			<< " e=" << l_f64ExpectedSampleCount
			// << ", i=" << m_i64InnerLatencySampleCount 
			<< " j=" << l_f64Jitter << " d=" << m_f64DriftEstimate 
			<< " m=" << (m_f64DriftEstimateTooFastMax > m_f64DriftEstimateTooSlowMax ? m_f64DriftEstimateTooFastMax : -m_f64DriftEstimateTooSlowMax)
			<< "\n";
	}
#endif

#elif defined(OV_ESTIMATE_BOOST)
	// this global estimate uses posix::time for the arithmetic and the clock. Mainly to study if it makes a difference.
	boost::posix_time::ptime l_oTimeNow = boost::posix_time::microsec_clock::local_time();
	boost::posix_time::time_duration l_oTd = (l_oTimeNow - m_oStartTime);

	const float64 l_f64Expected = (l_oTd.total_microseconds() / 1000000.0) * m_ui32SamplingFrequency;
	const float64 l_f64Received = static_cast<float64>(m_ui64CorrectedSampleCount);

	const float64 l_f64Jitter = l_f64Received - l_f64Expected + m_i64InnerLatencySampleCount;

#if defined(OV_DEBUG_DRIFT)
	static int calls=0;
	if(calls++ % 100 == 0) {
		m_rKernelContext.getLogManager() << LogLevel_Info << "At " << l_oTd.total_seconds()
			<< "ms, +" << ui64NewSamples << " smp, rec=" << l_f64Received
			<< ", exp=" << l_f64Expected+l_f64ExpectedFraction << ", " << l_f64Jitter << " jit " << m_f64DriftEstimate << " dft\n";
	}
#endif

#elif defined(OV_ESTIMATE_TIMEDIFF_BASED)
	// An alternative way to compute the jitter, by computing the diff in the times instead of diff in the sample counts.
	const uint64 l_ui64ExpectedTime = m_ui64StartTime + (m_ui64CorrectedSampleCount<<32) / static_cast<uint64>(m_ui32SamplingFrequency);

	float64 l_f64TimeDiff; // time in seconds that our expectation differs from the measured clock
	if(l_ui64ExpectedTime>=l_ui64CurrentTime)
	{
		l_f64TimeDiff = ITimeArithmetics::timeToSeconds(l_ui64ExpectedTime-l_ui64CurrentTime);
	}
	else
	{
		l_f64TimeDiff = -ITimeArithmetics::timeToSeconds(l_ui64CurrentTime-l_ui64ExpectedTime);
	}

	// Jitter in fractional samples
	const float64 l_f64Jitter = l_f64TimeDiff*m_ui32SamplingFrequency + static_cast<float64>(m_i64InnerLatencySampleCount);

#if defined(OV_DEBUG_DRIFT)
	static int calls=0;
	if(calls++ % 100 == 0) {
		m_rKernelContext.getLogManager() << LogLevel_Info << "At " 
			<< ITimeArithmetics::timeToSeconds(l_ui64CurrentTime)*1000 << "ms."
			<< " r=" <<m_ui64ReceivedSampleCount
			<< " td=" << l_f64Diff*1000 << "ms, j=" << l_f64Jitter << "\n";
	}
#endif

#endif

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

#ifdef OV_DEBUG_DRIFT
		if(m_f64DriftEstimate!=0)
		{	
			/*
			m_rKernelContext.getLogManager() << LogLevel_Info << 
				"At " << ITimeArithmetics::timeToSeconds(l_ui64ElapsedTime)*1000 << "ms, "
				<< "Acq mon [drift:" << getDrift() << "][jitter:" << l_i64JitterSampleCount << "] samples "
				<< "dsc " << getDriftSampleCount() << " "
				<< "(rsc " << m_ui64ReceivedSampleCount << " tsc " << l_ui64TheoreticalSampleCount << " ilsc " << m_i64InnerLatencySampleCount 
				<< "jsc " << m_vJitterEstimate.size() 
				<< ").\n";
			*/

			/*
			for(list<float64>::iterator j=m_vJitterEstimate.begin(); j!=m_vJitterEstimate.end(); j++)
			{
				std::cout << *j << " ";
			}
			std::cout << "\n";
			*/
		}
#endif
	}

	m_ui64LastEstimationTime = l_ui64CurrentTime;

	return true;
}


boolean CDriftCorrection::correctDrift(int64 i64Correction, uint64& ui64TotalSamples, std::vector < std::vector < OpenViBE::float32 > >& vPendingBuffer, OpenViBE::CStimulationSet& oPendingStimulationSet, 
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

	if(ui64TotalSamples != m_ui64CorrectedSampleCount) {
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

		const uint64 l_ui64TimeOfIncorrect     = ITimeArithmetics::sampleCountToTime(m_ui32SamplingFrequency, m_ui64CorrectedSampleCount-1);
		const uint64 l_ui64DurationOfIncorrect = ITimeArithmetics::sampleCountToTime(m_ui32SamplingFrequency, i64Correction);
		const uint64 l_ui64TimeOfCorrect       = ITimeArithmetics::sampleCountToTime(m_ui32SamplingFrequency, m_ui64CorrectedSampleCount-1+i64Correction);
		oPendingStimulationSet.appendStimulation(OVTK_GDF_Incorrect, l_ui64TimeOfIncorrect, l_ui64DurationOfIncorrect);
		oPendingStimulationSet.appendStimulation(OVTK_GDF_Correct,   l_ui64TimeOfCorrect, 0);

		m_f64DriftEstimate+=i64Correction;

		m_ui64CorrectedSampleCount+=i64Correction;
		m_i64DriftCorrectionSampleCountAdded+=i64Correction;
	}
	else if(i64Correction < 0)
	{
		const uint64 l_ui64SamplesToRemove=std::min<uint64>( uint64(-i64Correction), uint64(vPendingBuffer.size()) );

		vPendingBuffer.erase(vPendingBuffer.begin()+vPendingBuffer.size()-(int)l_ui64SamplesToRemove, vPendingBuffer.begin()+vPendingBuffer.size());

		const uint64 l_ui64LastSampleDate = ITimeArithmetics::sampleCountToTime(m_ui32SamplingFrequency, m_ui64CorrectedSampleCount-l_ui64SamplesToRemove);
		for(uint32 i=0; i<oPendingStimulationSet.getStimulationCount(); i++)
		{
			if(oPendingStimulationSet.getStimulationDate(i) > l_ui64LastSampleDate)
			{
				oPendingStimulationSet.setStimulationDate(i, l_ui64LastSampleDate);
			}
		}

		m_f64DriftEstimate-=l_ui64SamplesToRemove;

		m_ui64CorrectedSampleCount-=l_ui64SamplesToRemove;
		m_i64DriftCorrectionSampleCountRemoved+=l_ui64SamplesToRemove;
	}

	// correct the jitter estimate to match the correctioon we made. For example, if we had
	// a jitter estimate of [-1,-1,-1,...] and correct the drift by adding 1 sample, the new
	// estimate should become [0,0,0,0,...] wrt the adjustment. This changes jitter estimates
	// in the past. The other alternative would be to reset the estimate. Adjusting 
	// might keep some detail the reset would lose.
	for(list<float64>::iterator j=m_vJitterEstimate.begin(); j!=m_vJitterEstimate.end(); j++)
	{
		(*j)+=i64Correction;
	}

	ui64TotalSamples = m_ui64CorrectedSampleCount;

	return true;
}

float64 CDriftCorrection::getDrift(void) const
{
	if(m_ui32SamplingFrequency==0) 
	{
		return 0;
	}

#ifdef OV_DEBUG_DRIFT
	// m_rKernelContext.getLogManager() << LogLevel_Info << "Req drift " << m_f64DriftEstimate << " freq " << m_ui32SamplingFrequency << "\n";
#endif
	return m_f64DriftEstimate*1000./m_ui32SamplingFrequency;
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

	return m_f64DriftEstimateTooFastMax*1000./m_ui32SamplingFrequency;
}

int64 CDriftCorrection::getDriftSampleCount(void) const
{
	return static_cast<int64>(m_f64DriftEstimate);
}

// Note that we cannot do actual correction with subsample accuracy, so here we truncate the drift estimate to integer in getDriftSampleCount().
int64 CDriftCorrection::getSuggestedDriftCorrectionSampleCount(void) const
{
	const float64 l_f64DriftTolerance = static_cast<float64>(this->getDriftToleranceDuration());
	const float64 l_f64CurrentDrift = this->getDrift();
	
	// If the drift is positive, we need to correct by removing samples. 
	if(l_f64CurrentDrift >= l_f64DriftTolerance)
	{
		return -this->getDriftSampleCount();
	}

	// If the drift is negative, we need to add samples
	if(l_f64CurrentDrift <= -l_f64DriftTolerance)
	{
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

uint64 CDriftCorrection::getDriftToleranceDuration(void) const
{
	return m_ui64DriftToleranceDuration;
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



boolean CDriftCorrection::setDriftToleranceDuration(uint64 ui64DriftToleranceDuration)
{
	if(ui64DriftToleranceDuration==0)
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Minimum accepted drift tolerance limit is 1ms\n";
		m_ui64DriftToleranceDuration = 1;
		return true;
	}

	m_ui64DriftToleranceDuration=ui64DriftToleranceDuration;
	return true;
}

boolean CDriftCorrection::setJitterEstimationCountForDrift(uint64 ui64JitterEstimationCountForDrift)
{
	m_ui64JitterEstimationCountForDrift=ui64JitterEstimationCountForDrift;
	return true;
}




