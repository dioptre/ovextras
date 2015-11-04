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

// #define TIMINGDEBUG

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

boolean CDriftCorrection::start(uint32 ui32SamplingFrequency)
{
	if(ui32SamplingFrequency==0)
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Drift correction doesn't support sampling rate of 0.\n";
		return false;
	}

	reset();

	m_ui32SamplingFrequency = ui32SamplingFrequency;
	m_i64DriftToleranceSampleCount=(m_ui64DriftToleranceDuration * m_ui32SamplingFrequency) / 1000;
	m_ui64StartTime = System::Time::zgetTime();

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
	m_vJitterSampleCount.clear();

	m_ui64ReceivedSampleCount = 0;
	m_ui64CorrectedSampleCount = 0;
	m_ui64LastEstimationTime = 0;
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
	m_rKernelContext.getLogManager() << LogLevel_Info << "  Added    : " << m_i64DriftCorrectionSampleCountAdded << " samples (" << l_f64AddedRatio << "%)\n";
	m_rKernelContext.getLogManager() << LogLevel_Info << "  Removed  : " << m_i64DriftCorrectionSampleCountRemoved << " samples (" << l_f64RemovedRatio << "%)\n";

	m_rKernelContext.getLogManager() << LogLevel_Info << "Estimated drift (tolerance = " << l_ui64DriftToleranceDuration << "ms),\n";

	m_rKernelContext.getLogManager() << (l_f64DriftRatioTooSlowMax > 1.0 ? LogLevel_Warning : LogLevel_Info)
		<< "  Peak slow : " << -m_f64DriftEstimateTooSlowMax << " samples (" << getDriftTooSlowMax() << "ms late, " << 100*l_f64DriftRatioTooSlowMax << "% of tol.)\n";
	m_rKernelContext.getLogManager() << (l_f64DriftRatioTooFastMax > 1.0 ? LogLevel_Warning : LogLevel_Info)
		<< "  Peak fast : " << m_f64DriftEstimateTooFastMax << " samples (" << getDriftTooFastMax() << "ms early, " << 100*l_f64DriftRatioTooFastMax << "% of tol.)\n";

	if(m_eDriftCorrectionPolicy != DriftCorrectionPolicy_Disabled)
	{
		const float64 l_f64RemainingDrift = (static_cast<int64>(m_ui64CorrectedSampleCount) - static_cast<int64>(l_ui64TheoreticalSampleCount)) / static_cast<float64>(m_ui32SamplingFrequency);
		m_rKernelContext.getLogManager() << (std::abs(l_f64RemainingDrift) > l_ui64DriftToleranceDuration ? LogLevel_ImportantWarning : LogLevel_Info)
			<< "  Residual : " << l_f64RemainingDrift << "ms, " << 100*l_f64RemainingDrift/l_ui64DriftToleranceDuration << "% of tol., after corr.\n";
	}

	m_rKernelContext.getLogManager() << (std::abs(l_f64DriftRatio) > 1.0 ? LogLevel_Warning : LogLevel_Info)
		<< "  Last estimate : " << m_f64DriftEstimate << " samples (" << getDrift() << "ms, " << 100*l_f64DriftRatio << "% of tol.)"
		<< (m_eDriftCorrectionPolicy == DriftCorrectionPolicy_Disabled ? ", NO corr." : ", after corr.")
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

	m_ui64ReceivedSampleCount += ui64NewSamples;
	m_ui64CorrectedSampleCount += ui64NewSamples;

	const uint64 l_ui64CurrentTime = System::Time::zgetTime();
	const uint64 l_ui64ElapsedTime = l_ui64CurrentTime - m_ui64StartTime;
	const uint64 l_ui64TheoreticalSampleCount= (m_ui32SamplingFrequency * l_ui64ElapsedTime) >> 32;
	const int64 l_i64JitterSampleCount
		= static_cast<int64>(m_ui64CorrectedSampleCount)  - static_cast<int64>(l_ui64TheoreticalSampleCount)
			+m_i64InnerLatencySampleCount;
			
	m_vJitterSampleCount.push_back(l_i64JitterSampleCount);
	if(m_vJitterSampleCount.size() > m_ui64JitterEstimationCountForDrift)
	{
		m_vJitterSampleCount.pop_front();
	}

	// Estimate the drift after we have a full buffer of jitter estimates
	if(m_vJitterSampleCount.size() == m_ui64JitterEstimationCountForDrift)
	{
		float64  l_f64NewDriftSampleEstimate = 0;

		for(list<int64>::iterator j=m_vJitterSampleCount.begin(); j!=m_vJitterSampleCount.end(); j++)
		{
			l_f64NewDriftSampleEstimate+=*j;
		}

		m_f64DriftEstimate = l_f64NewDriftSampleEstimate / m_ui64JitterEstimationCountForDrift;

		if(m_f64DriftEstimate>0) {
			m_f64DriftEstimateTooFastMax = std::max<float64>(m_f64DriftEstimateTooFastMax, m_f64DriftEstimate);
		} else {
			m_f64DriftEstimateTooSlowMax = std::max<float64>(m_f64DriftEstimateTooSlowMax, -m_f64DriftEstimate);
		}

#ifdef TIMINGDEBUG
		if(m_f64DriftEstimate!=0)
		{	
			/*
			m_rKernelContext.getLogManager() << LogLevel_Info << 
				"At " << ITimeArithmetics::timeToSeconds(l_ui64ElapsedTime)*1000 << "ms, "
				<< "Acq mon [drift:" << getDrift() << "][jitter:" << l_i64JitterSampleCount << "] samples "
				<< "dsc " << getDriftSampleCount() << " "
				<< "(rsc " << m_ui64ReceivedSampleCount << " tsc " << l_ui64TheoreticalSampleCount << " ilsc " << m_i64InnerLatencySampleCount 
				<< "jsc " << m_vJitterSampleCount.size() 
				<< ").\n";
			*/

			/*
			for(list<int64>::iterator j=m_vJitterSampleCount.begin(); j!=m_vJitterSampleCount.end(); j++)
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
	for(list<int64>::iterator j=m_vJitterSampleCount.begin(); j!=m_vJitterSampleCount.end(); j++)
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

#ifdef TIMINGDEBUG
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

int64 CDriftCorrection::getSuggestedDriftCorrectionSampleCount(void) const
{
	// If the drift is positive, we need to correct by removing samples
	if(this->getDriftSampleCount() >= this->getDriftToleranceSampleCount())
	{
		return -this->getDriftSampleCount();
	}

	// If the drift is negative, we need to add samples
	if(this->getDriftSampleCount() <= -this->getDriftToleranceSampleCount())
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
	m_ui64DriftToleranceDuration=ui64DriftToleranceDuration;
	return true;
}

boolean CDriftCorrection::setJitterEstimationCountForDrift(uint64 ui64JitterEstimationCountForDrift)
{
	m_ui64JitterEstimationCountForDrift=ui64JitterEstimationCountForDrift;
	return true;
}




