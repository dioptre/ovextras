#include "ovCTime.h"

#include <cmath>

#include <boost/thread.hpp>

using namespace System;

#define boolean System::boolean

boolean Time::sleep(const uint32 ui32MilliSeconds)
{
	const boost::posix_time::time_duration l_oDuration = boost::posix_time::milliseconds(ui32MilliSeconds);

	boost::this_thread::sleep(l_oDuration);

	return true;
}

boolean Time::zsleep(const uint64 ui64Seconds)
{
	const uint32 l_ui32Seconds = static_cast<uint32>(ui64Seconds>>32);
	// zero the seconds with 0xFFFFFFFF, multiply to get the rest as fixed point microsec, then grab them (now in the 32 msbs)
	const uint64 l_ui64MicroSeconds = ((ui64Seconds & 0xFFFFFFFFLL) * 1000000LL) >> 32; 

	const boost::posix_time::time_duration l_oDuration = boost::posix_time::seconds(l_ui32Seconds) + boost::posix_time::microsec(l_ui64MicroSeconds);

	boost::this_thread::sleep(l_oDuration);

	return true;
}

uint32 Time::getTime(void)
{
	// turn the 32:32 fixed point seconds to milliseconds
	return static_cast<uint32>((zgetTime()*1000)>>32);
}

uint64 Time::zgetTime(void)
{
	static boolean l_bInitialized = false;
	static boost::posix_time::ptime l_oStartTime;
	if(!l_bInitialized)
	{
		l_bInitialized = true;
		l_oStartTime =  boost::posix_time::microsec_clock::local_time();
	}

	const boost::posix_time::ptime l_oTimeNow = boost::posix_time::microsec_clock::local_time();

	const boost::posix_time::time_duration l_oElapsed = l_oTimeNow - l_oStartTime;

	const uint64 l_ui64Seconds = l_oElapsed.ticks() / l_oElapsed.ticks_per_second();
	const uint64 l_ui64Fraction = l_oElapsed.ticks() % l_oElapsed.ticks_per_second();

	// Scale the fraction from [0,td.ticks_per_second[ range to [0,2^32-1] (=0xFFFFFFFF)
	const uint64 l_ui64ReturnValue = (l_ui64Seconds<<32) + l_ui64Fraction*(0xFFFFFFFFLL/l_oElapsed.ticks_per_second());

	return l_ui64ReturnValue;
}
