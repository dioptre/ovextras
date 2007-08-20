#include "Time.h"

#if defined System_OS_Linux
 #include <unistd.h>
 #include <time.h>
 #include <sys/time.h>
#elif defined System_OS_Windows
 #include <windows.h>
#else
#endif

using namespace System;
#define boolean System::boolean

boolean Time::sleep(const uint32 ui32MilliSeconds)
{
	return zsleep((((uint64)ui32MilliSeconds)<<32)/1000);
}

boolean Time::zsleep(const uint64 ui64Seconds)
{
#if defined System_OS_Linux
	usleep((ui64Seconds*1000000)>>32);
#elif defined System_OS_Windows
	Sleep((uint32)((ui64Seconds*1000)>>32));
#else
#endif
	return true;
}

uint32 Time::getTime(void)
{
	return (uint32)((zgetTime()*1000)>>32);
}

uint64 Time::zgetTime(void)
{
	uint64 l_ui64Result=0;
#if defined System_OS_Linux
	struct timeval l_oTimeValue;
	gettimeofday(&l_oTimeValue, NULL);
	l_ui64Result+=(((uint64)l_oTimeValue.tv_sec)<<32);
	l_ui64Result+=(((uint64)l_oTimeValue.tv_usec)<<32)/1000000;
#elif defined System_OS_Windows
	LARGE_INTEGER l_oPerformanceCounter;
	LARGE_INTEGER l_oPerformanceFrequency;
	QueryPerformanceCounter(&l_oPerformanceCounter);
	QueryPerformanceFrequency(&l_oPerformanceFrequency);
	l_ui64Result=((l_oPerformanceCounter.QuadPart<<32)/l_oPerformanceFrequency.QuadPart);
#else
#endif
	return l_ui64Result;
}
