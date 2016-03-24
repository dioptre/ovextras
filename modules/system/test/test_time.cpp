/*
 * \author Nathanael Foy, Jozef Legeny, Jussi T. Lindgren
 * \date 2015-2016
 * \brief Performs comparison of clock functions. 
 *
 * This compares the potential timing facilities (clocks) available on the platform in a few different ways. 
 * 
 * It is not meant for integration testing or testing of Openvibe per ce. However, bad results may indicate
 * a problem on the time accuracy of the system in general (esp. if from ovCTime clock)
 *
 * The test will poll the clocks repeatedly between sleeps and write a .csv file. Each column gives the time each clock gave.
 *
 * Note that as modules are not supposed to depend on openvibe or its
 * kernel, we currently reimplement some functions here.
 *
 */


#include <iostream>
#include <stdio.h>

#include <string>
#include <iostream>

#include <ovCTime.h>

//Components of the Boost Library
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/config.hpp>

// On Linuxes, needs libboost-chrono1.48dev; may require updating ALL boost pkgs to 1.48
#if defined(TARGET_HAS_Boost_Chrono)
#define BOOST_CHRONO_HEADER_ONLY
#include <boost/chrono.hpp>
#endif

#if defined TARGET_OS_Linux
 #include <unistd.h>
 #include <ctime>
 #include <sys/time.h>

#elif defined(TARGET_OS_Windows)
 #include <Windows.h>
 #include <MMSystem.h> 
#endif

#include "ntpclient.h"

using namespace std;

namespace OpenViBE {
	class ITimeArithmetics {
	public:
		// From ITimeArithmetics
		static uint64_t subsecondsToTime(const uint64_t ui64Seconds, const uint64_t ui64Subseconds, const uint64_t ui64SubsInSecond) 
		{
			return (ui64Seconds << 32) + (ui64Subseconds << 32) / ui64SubsInSecond;
		}

		static double timeToSeconds(const uint64_t ui64Time)
		{
			return (ui64Time>>m_ui32Shift)/double(m_ui32Multiplier);
		}

		static const uint32_t m_ui32DecimalPrecision = 10;		// In bits

		static const uint32_t m_ui32Shift = 32-m_ui32DecimalPrecision;			// 22
		static const uint32_t m_ui32Multiplier = 1L << m_ui32DecimalPrecision;	// 1024
	};
}
	

// A uint64_t wrapper for openvibe time getter for compilers
// that are not happy mixing uint64_t and System::uint64
uint64_t ovGetTime(void) {
	return static_cast<uint64_t>( System::Time::zgetTime() );
}

#if defined(TARGET_OS_Windows)

// Classic OV time functions, these are copies from ovCTime.h/ovCTime.cpp as we don't know which functions it has enabled (todo: refactor)

static uint64_t zgetTime1(void)
{
	// high definition time estimation
	// counts CPU cycles and uses nominal frequency
	//  - very accurate on short duration
	//  - very unaccurate on long duration

	uint64_t l_ui64Result=0;

	static bool l_bInitialized=false;
	static uint64_t l_ui64Frequency;
	static uint64_t l_ui64CounterStart;
	uint64_t l_ui64Counter;

	if(!l_bInitialized)
	{
		LARGE_INTEGER l_oPerformanceFrequency;
		QueryPerformanceFrequency(&l_oPerformanceFrequency);
		l_ui64Frequency=l_oPerformanceFrequency.QuadPart;

		LARGE_INTEGER l_oPerformanceCounterStart;
		QueryPerformanceCounter(&l_oPerformanceCounterStart);
		l_ui64CounterStart=l_oPerformanceCounterStart.QuadPart;

		l_bInitialized=true;
	}

	LARGE_INTEGER l_oPerformanceCounter;
	QueryPerformanceCounter(&l_oPerformanceCounter);
	l_ui64Counter=l_oPerformanceCounter.QuadPart-l_ui64CounterStart;
	
	l_ui64Result=OpenViBE::ITimeArithmetics::subsecondsToTime(l_ui64Counter/l_ui64Frequency, l_ui64Counter%l_ui64Frequency, l_ui64Frequency);

	return l_ui64Result;
}

static uint64_t zgetTime2(void)
{
	// low definition time estimation
	// uses windows multimedia timers
	//  - very unaccurate on short duration (refreshed every 15ms)
	//  - very accurate on long duration

	uint64_t l_ui64Result=0;

	static bool l_bInitialized=false;
	static uint64_t l_ui64CounterStart;
	uint64_t l_ui64Counter;

	l_ui64Counter=uint64_t(timeGetTime());
	l_ui64Counter=OpenViBE::ITimeArithmetics::subsecondsToTime(l_ui64Counter/1000, l_ui64Counter%1000, 1000);

	if(!l_bInitialized)
	{
		l_ui64CounterStart=l_ui64Counter;
		l_bInitialized=true;
	}

	l_ui64Result=l_ui64Counter-l_ui64CounterStart;

	return l_ui64Result;
}

static uint64_t win32_zgetTime(void)
{
	// hopefully high definition time estimation
	// tries to mix both low & high definition chronos
	// and rescale the long-time unaccurate chrono in the long-time accurate (but locally unaccurate) chrono range

	uint64_t l_ui64Result=0;
	static bool l_bInitialized=false;
	static uint64_t t1_start=0;
	static uint64_t t2_start=0;
	static uint64_t l_ui64Result1_last=0;
	static uint64_t l_ui64Result2_last=0;
	static uint64_t l_ui64Result_last=0;

	// intialize start times for both low & high def chronos
	if(!l_bInitialized)
	{
		t1_start=zgetTime1();
		t2_start=zgetTime2();
		l_bInitialized=true;
	}

	// update current time for both low & high def chronos
	uint64_t l_ui64Result1=zgetTime1()-t1_start;
	uint64_t l_ui64Result2=zgetTime2()-t2_start;

	// when low def chrono has update, recompute scale coefficients
	if(l_ui64Result1!=l_ui64Result1_last)
	{
		l_ui64Result1_last=l_ui64Result1;
		l_ui64Result2_last=l_ui64Result2;
	}

	// when denominator is 0, consider that elapsed time was not sufficient since the beginning
	if(l_ui64Result2_last==0)
	{
		return 0;
	}

	// scale high def chrono in low def chrono range
	l_ui64Result=(l_ui64Result2/l_ui64Result2_last)*l_ui64Result1_last;
	l_ui64Result+=((l_ui64Result2%l_ui64Result2_last)*l_ui64Result1_last)/l_ui64Result2_last;

	// ensure no return in past
	if(l_ui64Result<l_ui64Result_last)
	{
		return l_ui64Result_last;
	}
	l_ui64Result_last=l_ui64Result;
	return l_ui64Result;
}

#endif // TARGET_OS_Windows
	
uint64_t zgetTime(void)
{
#if defined(TARGET_OS_Windows)
	return win32_zgetTime();
#else
	static bool l_bInitialized=false;
	static struct timeval l_oTimeValueStart;
	struct timeval l_oTimeValue;
	uint64_t l_ui64TimeMicroSecond=0;

	if(!l_bInitialized)
	{
			gettimeofday(&l_oTimeValueStart, NULL);

			l_bInitialized=true;
	}

	gettimeofday(&l_oTimeValue, NULL);
	uint64_t l_ui64SecDiff=(uint64_t)(l_oTimeValue.tv_sec-l_oTimeValueStart.tv_sec);
	uint64_t l_ui64USecDiff=(uint64_t)(l_oTimeValue.tv_usec-l_oTimeValueStart.tv_usec);

	const uint64_t l_ui64MicrosInSecond = 1000*1000;

	l_ui64TimeMicroSecond+=l_ui64SecDiff*l_ui64MicrosInSecond;
	l_ui64TimeMicroSecond+=l_ui64USecDiff;

	uint64_t l_ui64Result=OpenViBE::ITimeArithmetics::subsecondsToTime(l_ui64TimeMicroSecond/l_ui64MicrosInSecond, l_ui64TimeMicroSecond % l_ui64MicrosInSecond, l_ui64MicrosInSecond);

	return l_ui64Result;
#endif
}

#if defined(TARGET_OS_Windows)

// Alternative based on GetSystemTimeAsFileTime()
uint64_t getSystemTime(void)
{
	FILETIME ft;

	GetSystemTimeAsFileTime(&ft);

	// 100 nanosecond intervals
	uint64_t ll_now = (LONGLONG)ft.dwLowDateTime + ((LONGLONG)(ft.dwHighDateTime) << 32LL);

	static const uint64_t l_ui64IntervalsPerSecond = 10*1000*1000; // 100ns -> ms -> s 

	const uint64_t seconds = ll_now / l_ui64IntervalsPerSecond;		
	const uint64_t fraction = ll_now % l_ui64IntervalsPerSecond;

	// below in fraction part, scale [0,l_uiIntervalsPerSecond-1] to 32bit integer range
	return OpenViBE::ITimeArithmetics::subsecondsToTime(seconds, fraction, l_ui64IntervalsPerSecond);
}

#endif // TARGET_OS_Windows

// Alternative based on boost::posix_time

uint64_t getBoostTime(void)
{
	const boost::posix_time::ptime epoch(boost::posix_time::from_time_t(0));

	const boost::posix_time::ptime mst1 = boost::posix_time::microsec_clock::local_time();

	const boost::posix_time::time_duration td = mst1-epoch;
	
	const uint64_t micros = td.total_microseconds();

	static const uint64_t l_ui64MicrosPerSecond = 1000*1000;

	const uint64_t seconds = micros / l_ui64MicrosPerSecond;
	const uint64_t fraction = micros % l_ui64MicrosPerSecond;

	// below in fraction part, scale [0,l_ui64MicrosPerSecond-1] to 32bit integer range
	const uint64_t retVal = OpenViBE::ITimeArithmetics::subsecondsToTime(seconds, fraction, l_ui64MicrosPerSecond);

	return retVal;
}

#if defined(TARGET_HAS_Boost_Chrono)

// Alternative based on boost::chrono

uint64_t getBoostChronoTime(void)
{
	static bool l_bInitialized=false;
	static boost::chrono::steady_clock::time_point l_oTimeStart;

	if(!l_bInitialized)
	{
		l_oTimeStart = boost::chrono::steady_clock::now();
		l_bInitialized = true;
	}

	const boost::chrono::steady_clock::time_point l_oTimeNow = boost::chrono::steady_clock::now();

	const boost::chrono::steady_clock::duration l_oElapsed = l_oTimeNow - l_oTimeStart;

    const boost::chrono::microseconds l_oElapsedMs = boost::chrono::duration_cast<boost::chrono::microseconds>(l_oElapsed);

	const uint64_t l_ui64MicrosPerSecond = 1000*1000;

	const uint64_t seconds = l_oElapsedMs.count() / l_ui64MicrosPerSecond;
	const uint64_t fraction = l_oElapsedMs.count() % l_ui64MicrosPerSecond;

	// below in fraction part, scale [0,l_ui64MicrosPerSecond-1] to 32bit integer range
	const uint64_t retVal = OpenViBE::ITimeArithmetics::subsecondsToTime(seconds, fraction, l_ui64MicrosPerSecond);

	return retVal;
}
#endif

// Alternative from NTP time server

uint64_t getNTPTime(void)
{
	// Change to server you can access
	const char* NTPServer = "ntp.inria.fr";
	// How often to poll? Some servers may return identical values if polled too often. 2 minutes in milliseconds.
	const uint64_t ntpInterval = 2*60*1000;

#if defined(TARGET_HAS_Boost_Chrono)
	static NTPClient NTP(NTPServer, ntpInterval , getBoostChronoTime);
#else
	static NTPClient NTP(NTPServer, ntpInterval , zgetTime);
#endif

	return NTP.getTime();
}

#if defined(TARGET_OS_Windows)

// Alternative from ftime()

#include <sys/timeb.h>

uint64_t getFTime(void)
{
	static bool l_bInitialized=false;
	static uint64_t l_ui64TimeStartMs;

	if(!l_bInitialized)
	{
		timeb l_oTimeStart;
		ftime(&l_oTimeStart);
		l_ui64TimeStartMs = l_oTimeStart.time * 1000 + l_oTimeStart.millitm; 
		l_bInitialized = true;
	}

	timeb l_oTimeNow;
	ftime(&l_oTimeNow);

	const uint64_t l_ui64TimeNowMs = (l_oTimeNow.time * 1000 + l_oTimeNow.millitm);

	const uint64_t l_ui64ElapsedMs = l_ui64TimeNowMs - l_ui64TimeStartMs;

	static const uint64_t l_ui64MillisPerSecond = 1000;

	const uint64_t seconds = l_ui64ElapsedMs / l_ui64MillisPerSecond;
	const uint64_t fraction = l_ui64ElapsedMs % l_ui64MillisPerSecond;

	// below in fraction part, scale [0,l_ui64MicrosPerSecond-1] to 32bit integer range
	const uint64_t retVal = OpenViBE::ITimeArithmetics::subsecondsToTime(seconds, fraction, l_ui64MillisPerSecond);

	return retVal;

}

#endif

// This encapsulates a clock getter and its first poll time, last time, and current time - used to test different clocks in the same loop code.

class Clock
{
public:
	Clock( std::string name, uint64_t (*getTimeFunct)(void) ) : start(0),now(0),last(0),errors(0), m_sName(name), m_pGetTimeFunct(getTimeFunct) { };
	uint64_t start;
	uint64_t now;
	uint64_t last;
	uint64_t errors;
	std::string m_sName;
	uint64_t (*m_pGetTimeFunct)(void);
};

// Spin the clock, make an estimate of the clock granularity
void spinTest(const char *clockName, uint64_t (*getTimeFunct)(void) )
{
	const uint64_t l_ui64TestFor = 5LL << 32;

	printf("Spin test %s ... ", clockName);
	const uint64_t l_ui64StartTime = getTimeFunct();
	uint64_t l_ui64Now = l_ui64StartTime;
	uint64_t l_ui64Previous = l_ui64Now;
	uint64_t l_ui64CumulativeStep = 0;
	uint64_t l_ui64Estimates = 0;
	while( l_ui64Now - l_ui64StartTime < l_ui64TestFor)
	{
		l_ui64Now = getTimeFunct();
		if(l_ui64Now > l_ui64Previous) 
		{
			l_ui64CumulativeStep += (l_ui64Now - l_ui64Previous);
			l_ui64Estimates++;
		}
		if(l_ui64Now < l_ui64Previous)
		{
			printf("Error, clock is not monotonic\n");
			return;
		}
		l_ui64Previous = l_ui64Now;
	}
	const double l_f64Step = OpenViBE::ITimeArithmetics::timeToSeconds(l_ui64CumulativeStep) / l_ui64Estimates;
	printf("measured step is %f ms\n", l_f64Step*1000.0);
//	printf("  accu %lld after %lld iters.\n", l_ui64CumulativeStep, l_ui64Estimates);
}

#if defined(TARGET_OS_Windows)

#include <Winternl.h>

typedef ULONG (__stdcall* NtQueryTimerResolution)(
		OUT PULONG minimumResolution,
		OUT PULONG maximumResolution,
		OUT PULONG currentResolution);

// Returns 100ns units, needs a 'hidden' function from ntdll.dll
void getClockResolution(uint32_t& minimum, uint32_t& maximum, uint32_t& current)
{
    HMODULE hModule = LoadLibrary(TEXT("ntdll.dll"));
	if(!hModule) {
		std::cout << "Error loading ntdll.dll\n";
		return;
	}
    NtQueryTimerResolution func = (NtQueryTimerResolution)GetProcAddress(hModule, "NtQueryTimerResolution");
	if(!func)
	{
		std::cout << "Error getting NtQueryTimerResolution function handle\n";
		return;
	}

	ULONG min = 0, max = 0, cur = 0;
	func(&min, &max, &cur);

	minimum = min; maximum = max; current = cur;
}
#endif

int main(int argc, char** argv)
{
	int64_t runTime = 120;
	int64_t sleepTime = 5;

	if (argc > 1)
	{
		runTime = atoi(argv[1]);
	}

	if (argc > 2)
	{
		sleepTime = atoi(argv[2]);
	}

	const double errorThMs = 5.0;					// If clock delta is more than this, count an error

#if defined(TARGET_OS_Windows)
	const uint32_t preferredResolution = 5;			// set the preferred clock resolution, in ms

	uint32_t minimumResolution, maximumResolution, currentResolution;
	getClockResolution(minimumResolution, maximumResolution, currentResolution);

	printf("System's timer resolution range is [%.2f, %.2f] ms\n", maximumResolution/10000.0, minimumResolution/10000.0);
	printf("Current timer resolution is %.2f ms\n", currentResolution/10000.0);

	if(currentResolution/10000.0 <= preferredResolution)
	{
		printf("WARNING: Current clock resolution is already <= the requested limit %d ms\n", preferredResolution);
	}

	printf("Requesting timer resolution of %d ms\n", preferredResolution);
	MMRESULT result = timeBeginPeriod(preferredResolution);
	if(result!=0) {
		printf("WARNING: setting time resolution returned error code %d\n", result);
	}
	if(sleepTime<preferredResolution) {
		printf("WARNING: sleep time %lld ms is smaller than the requested clock resolution %d ms\n", 
			static_cast<long long>(sleepTime), preferredResolution);
	}

	HANDLE l_oProcess = GetCurrentProcess();
	SetPriorityClass(l_oProcess, REALTIME_PRIORITY_CLASS);		// The highest priority class
	SetThreadPriority(l_oProcess, THREAD_PRIORITY_NORMAL);		// Even higher options: THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_TIME_CRITICAL

	// Ad-hoc sleep a bit to let clock stabilize after timeBeginPeriod()
	boost::this_thread::sleep(boost::posix_time::millisec(1000));
#endif

	// boost sleep accuracy test
	boost::posix_time::ptime mst1 = boost::posix_time::microsec_clock::local_time();
	boost::this_thread::sleep(boost::posix_time::millisec(500));
	boost::posix_time::ptime mst2 = boost::posix_time::microsec_clock::local_time();
	boost::posix_time::time_duration msdiff = mst2 - mst1;
	std::cout << "boost::this_thread::sleep accuracy : " << msdiff.total_milliseconds() << "ms slept (500ms asked, posix clock)" << std::endl;

#if defined(TARGET_HAS_Boost_Chrono)
	boost::chrono::steady_clock::time_point cmst1 = boost::chrono::steady_clock::now();
	boost::this_thread::sleep(boost::posix_time::millisec(500));
	boost::chrono::steady_clock::time_point cmst2 = boost::chrono::steady_clock::now();
	const boost::chrono::steady_clock::duration cmsdiff = cmst2 - cmst1;
	std::cout << "boost::this_thread::sleep accuracy : " <<  (boost::chrono::duration_cast<boost::chrono::milliseconds>(cmsdiff)).count() << "ms slept (500ms asked, chrono clock)" << std::endl;
#else
	std::cout << "Note: Boost chrono was not available during compilation, skipping\n";
#endif

	// Estimate clock granularity
	spinTest("ovCTime", ovGetTime);
#if defined(TARGET_HAS_Boost_Chrono)
	spinTest("boost::chrono", getBoostChronoTime);
#endif
	spinTest("zgetTime", zgetTime);
#if defined(TARGET_OS_Windows)
	spinTest("ftime", getFTime);
#endif
	spinTest("boost::posix_time", getBoostTime);
	
	printf("Starting sleep/poll test...\n");

	printf("Run time: %lld (s), Sleep time: %lld (ms)\n", static_cast<long long>(runTime), static_cast<long long>(sleepTime));

	char filename[255];
	sprintf(filename, "timetest-%lld-%lld-elapsed.csv", static_cast<long long>(runTime), static_cast<long long>(sleepTime));
	FILE* f = fopen(filename, "w");
	sprintf(filename, "timetest-%lld-%lld-delta.csv", static_cast<long long>(runTime), static_cast<long long>(sleepTime));
	FILE* fd = fopen(filename, "w");
	sprintf(filename, "timetest-%lld-%lld-errors.csv", static_cast<long long>(runTime), static_cast<long long>(sleepTime));
	FILE* fe = fopen(filename, "w");

	std::vector<Clock> l_vClocks;

	l_vClocks.push_back(Clock("ovCTime", ovGetTime));				// the first clock controls the duration of the test
#if defined(TARGET_HAS_Boost_Chrono)
	l_vClocks.push_back(Clock("BoostChronoTime", getBoostChronoTime));
#endif
	l_vClocks.push_back(Clock("zgetTime", zgetTime));					            
	l_vClocks.push_back(Clock("BoostTime", getBoostTime));
#if defined(TARGET_OS_Windows)
	l_vClocks.push_back(Clock("systemTime", getSystemTime));
	l_vClocks.push_back(Clock("zgetTime1", zgetTime1));
	l_vClocks.push_back(Clock("zgetTime2", zgetTime2));
	l_vClocks.push_back(Clock("ftime", getFTime));
#endif
// #define OV_TEST_USE_NTP
#if defined(OV_TEST_USE_NTP)
	l_vClocks.push_back(Clock("NTP", getNTPTime));
	// NTP clock needs a moment to complete the poll, call once to start
	getNTPTime(); 
	boost::this_thread::sleep(boost::posix_time::millisec(2000));
#endif

	// Times elapsed since start per clock
	fprintf(f, "Record,Expect");
	// Timediff since last measurement
	fprintf(fd, "Record");
	for(size_t i=0;i<l_vClocks.size();i++)
	{
		fprintf(f, ",%s", l_vClocks[i].m_sName.c_str());
		fprintf(fd, ",%s", l_vClocks[i].m_sName.c_str());
	}
	fprintf(f, "\n");
	fprintf(fd, "\n");

	// Start all the clocks
	for(size_t i=0;i<l_vClocks.size();i++)
	{
		l_vClocks[i].start = l_vClocks[i].m_pGetTimeFunct();
		l_vClocks[i].last = l_vClocks[i].start;
	}

	int pollCount = 0;
	while (l_vClocks[0].last < l_vClocks[0].start + (runTime << 32))
	{
		if (sleepTime > 0)
		{
			// @todo busy wait on a clock here to avoid confusion coming from sleep granularity (on Win, often resembles the clock resolution)
			// do a separate test for sleep accuracy if needed.
			boost::this_thread::sleep(boost::posix_time::millisec(sleepTime));
		}

		// First poll all the clocks
		for(size_t i=0;i<l_vClocks.size();i++)
		{	
			l_vClocks[i].now = l_vClocks[i].m_pGetTimeFunct();
		}

		// Then do the rest, so each clock gets approx the same (tiny) delay from this

		fprintf(f, "%d,%lld", pollCount, static_cast<long long>((pollCount+1)*sleepTime));
		fprintf(fd, "%d", pollCount);

		for(size_t i=0;i<l_vClocks.size();i++)
		{
			const double elapsed = OpenViBE::ITimeArithmetics::timeToSeconds(l_vClocks[i].now - l_vClocks[i].start) * 1000.0;
			const double diff = OpenViBE::ITimeArithmetics::timeToSeconds(l_vClocks[i].now - l_vClocks[i].last) * 1000.0;

			if( fabs(diff - sleepTime) > errorThMs ) l_vClocks[i].errors++;

			// Times elapsed since start
			fprintf(f, ",%lf", elapsed);

			// Timediff since last measurement
			fprintf(fd, ",%lf", diff);

			l_vClocks[i].last = l_vClocks[i].now;
		}

		fprintf(f, "\n");
		fprintf(fd, "\n");

		pollCount++;

	}

	fclose(f);
	fclose(fd);

	for(uint32_t i=0;i<l_vClocks.size();i++)
	{
		fprintf(fe, "%f ", l_vClocks[i].errors/(float)pollCount);
	}
	fprintf(fe, "\n");

	fclose(fe);

#if defined(TARGET_OS_Windows)
	timeEndPeriod(preferredResolution);
#endif

	return 0;
}
