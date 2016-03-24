#include "ovCTime.h"

#include <cmath>

// \note On Windows, unless timeBeginPeriod(1) is set, the non-QPC functions here may have only 15ms accuracy.
// \warning On Windows, avoid "using namespace System;" here as it may cause confusion with stuff coming from windows/boost
// 
// To find code based on boost::posix_time, look into earlier versions of this file. That clock may not have a monotonicy guarantee.
//

#if defined(TARGET_HAS_Boost_Chrono)
#define OV_BOOST_CHRONO_TIME               // Use timing routines based on boost::chrono.
#else
#define OV_CLASSIC_TIME                    // Use the 'classic' openvibe timing routines. deprecated, should be removed after a transition period.
#endif

#if defined(OV_BOOST_CHRONO_TIME)
 #define BOOST_CHRONO_HEADER_ONLY
 #include <boost/chrono.hpp>
 #include <boost/thread.hpp>
#endif

#if defined(OV_CLASSIC_TIME)
 #if defined TARGET_OS_Linux
  #include <unistd.h>
  #include <ctime>
  #include <sys/time.h>
 #elif defined TARGET_OS_Windows
  #include <windows.h>
 #else
  //
 #endif
#endif

#define boolean System::boolean
#define uint32 System::uint32
#define uint64 System::uint64

// From ITimeArithmetics
static uint64 subsecondsToTime(const uint64 ui64Seconds, const uint64 ui64Subseconds, const uint64 ui64SubsInSecond) 
{
	return (ui64Seconds << 32) + (ui64Subseconds << 32) / ui64SubsInSecond;
}

#if defined(OV_CLASSIC_TIME)

boolean System::Time::sleep(const uint32 ui32MilliSeconds)
{
	return zsleep(((((uint64)ui32MilliSeconds)<<22)/1000)<<10);
}

#if defined TARGET_OS_Windows

namespace
{
	static uint64 zgetTime1(void)
	{
		// high definition time estimation
		// counts CPU cycles and uses nominal frequency
		//  - very accurate on short duration
		//  - very unaccurate on long duration

		uint64 l_ui64Result=0;

		static boolean l_bInitialized=false;
		static uint64 l_ui64Frequency;
		static uint64 l_ui64CounterStart;
		uint64 l_ui64Counter;

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

		l_ui64Result=subsecondsToTime(l_ui64Counter/l_ui64Frequency, l_ui64Counter%l_ui64Frequency, l_ui64Frequency);

		return l_ui64Result;
	}

	static uint64 zgetTime2(void)
	{
		// low definition time estimation
		// uses windows multimedia timers
		//  - very unaccurate on short duration (refreshed every 15ms)
		//  - very accurate on long duration

		uint64 l_ui64Result=0;

		static boolean l_bInitialized=false;
		static uint64 l_ui64CounterStart;
		uint64 l_ui64Counter;

		l_ui64Counter=uint64(timeGetTime());
		l_ui64Counter=subsecondsToTime(l_ui64Counter/1000, l_ui64Counter%1000, 1000);

		if(!l_bInitialized)
		{
			l_ui64CounterStart=l_ui64Counter;
			l_bInitialized=true;
		}

		l_ui64Result=l_ui64Counter-l_ui64CounterStart;

		return l_ui64Result;
	}

	static uint64 win32_zgetTime(void)
	{
		// hopefully high definition time estimation
		// tries to mix both low & high definition chronos
		// and rescale the long-time unaccurate chrono in the long-time accurate (but locally unaccurate) chrono range

		uint64 l_ui64Result=0;
		static boolean l_bInitialized=false;
		static uint64 t1_start=0;
		static uint64 t2_start=0;
		static uint64 l_ui64Result1_last=0;
		static uint64 l_ui64Result2_last=0;
		static uint64 l_ui64Result_last=0;

		// intialize start times for both low & high def chronos
		if(!l_bInitialized)
		{
			t1_start=zgetTime1();
			t2_start=zgetTime2();
			l_bInitialized=true;
		}

		// update current time for both low & high def chronos
		uint64 l_ui64Result1=zgetTime1()-t1_start;
		uint64 l_ui64Result2=zgetTime2()-t2_start;

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
}

#endif

boolean System::Time::zsleep(const uint64 ui64Seconds)
{
#if defined TARGET_OS_Linux
	usleep((ui64Seconds*1000000)>>32);
#elif defined TARGET_OS_Windows
	Sleep((uint32)(((ui64Seconds>>10)*1000)>>22));
#else
#endif
	return true;
}

uint32 System::Time::getTime(void)
{
	return (uint32)(((zgetTime()>>22)*1000)>>10);
}

#if defined TARGET_OS_Linux

uint64 System::Time::zgetTime(void)
{
	uint64 l_ui64Result=0;

	static boolean l_bInitialized=false;
	static struct timeval l_oTimeValueStart;
	struct timeval l_oTimeValue;
	uint64 l_i64TimeMicroSecond=0;

	if(!l_bInitialized)
	{
		gettimeofday(&l_oTimeValueStart, NULL);

		l_bInitialized=true;
	}

	gettimeofday(&l_oTimeValue, NULL);
	int64 l_i64SecDiff=(int64)(l_oTimeValue.tv_sec-l_oTimeValueStart.tv_sec);
	int64 l_i64USecDiff=(int64)(l_oTimeValue.tv_usec-l_oTimeValueStart.tv_usec);

	l_i64TimeMicroSecond+=l_i64SecDiff*1000000;
	l_i64TimeMicroSecond+=l_i64USecDiff;

	const uint64 l_ui64MicrosInSecond = 1000*1000;

	l_ui64Result=subsecondsToTime(l_i64TimeMicroSecond/l_ui64MicrosInSecond, l_i64TimeMicroSecond % l_ui64MicrosInSecond, l_ui64MicrosInSecond);

	return l_ui64Result;
}

#else

uint64 System::Time::zgetTime(void)
{
	const uint64 l_ui64Result = win32_zgetTime();

	return l_ui64Result;
}

#endif

#endif // OV_CLASSIC_TIME

#if defined(OV_BOOST_CHRONO_TIME)

boolean System::Time::sleep(const uint32 ui32MilliSeconds)
{
	const boost::posix_time::time_duration l_oDuration = boost::posix_time::milliseconds(ui32MilliSeconds);

	boost::this_thread::sleep(l_oDuration);

	return true;
}

boolean System::Time::zsleep(const uint64 ui64Seconds)
{
	const uint32 l_ui32Seconds = static_cast<uint32>(ui64Seconds>>32);
	// zero the seconds with 0xFFFFFFFF, multiply to get the rest as fixed point microsec, then grab them (now in the 32 msbs)
	const uint64 l_ui64MicroSeconds = ((ui64Seconds & 0xFFFFFFFFLL) * 1000000LL) >> 32; 

	const boost::posix_time::time_duration l_oDuration = boost::posix_time::seconds(l_ui32Seconds) + boost::posix_time::microsec(l_ui64MicroSeconds);

	boost::this_thread::sleep(l_oDuration);

	return true;
}

uint32 System::Time::getTime(void)
{
	// turn the 32:32 fixed point seconds to milliseconds
	return static_cast<uint32>((zgetTime()*1000)>>32);
}



uint64 System::Time::zgetTime(void)
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

	const uint64_t l_ui64Seconds = l_oElapsedMs.count() / l_ui64MicrosPerSecond;
	const uint64_t l_ui64Fraction = l_oElapsedMs.count() % l_ui64MicrosPerSecond;

	// below in fraction part, scale [0,l_ui64MicrosPerSecond-1] to 32bit integer range
	const uint64_t l_ui64ReturnValue =  subsecondsToTime(l_ui64Seconds, l_ui64Fraction, l_ui64MicrosPerSecond);

	return l_ui64ReturnValue;
}

#endif // OV_BOOST_CHRONO_TIME


