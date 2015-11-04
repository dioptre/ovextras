/*
 * \author Nathanael Foy, Jozef Legeny, Jussi T. Lindgren
 * \date 2015
 * \brief Some tests for timing
 */


//#include "stdafx.h"
#include <iostream>
#include <stdio.h>
// #include <stdint.h>

#include <string>
#include <iostream>

//Components of the Boost Library
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/config.hpp>

#if defined TARGET_OS_Linux
 #include <unistd.h>
 #include <ctime>
 #include <sys/time.h>

// typedef signed long long int64_t;

#elif defined(TARGET_OS_Windows)
 #include <Windows.h>
 #include <MMSystem.h> 
#endif

using namespace std;

#define NTP_MODE_CLIENT 3
#define NTP_VERSION 3

namespace OpenViBE {
	class ITimeArithmetics {
	public:
		static double timeToSeconds(const uint64_t ui64Time)
		{
			return (ui64Time>>m_ui32Shift)/double(m_ui32Multiplier);
		}

		static const uint32_t m_ui32DecimalPrecision = 10;		// In bits

		static const uint32_t m_ui32Shift = 32-m_ui32DecimalPrecision;			// 22
		static const uint32_t m_ui32Multiplier = 1L << m_ui32DecimalPrecision;	// 1024
	};
}

class NTPClient 
{
	private:
		boost::asio::io_service m_service;
		boost::asio::ip::udp::endpoint m_receiver;
		boost::asio::ip::udp::socket m_socket;

	public:

	NTPClient(string hostName)
		: m_socket(m_service)
	{
		std::cout << "NTP set to poll [" << hostName << "]\n";

		boost::asio::ip::udp::resolver resolver(m_service);
		boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), hostName, "ntp");

		try { 
			m_receiver = *resolver.resolve(query);
			m_socket.open(boost::asio::ip::udp::v4());
		} catch (std::exception& e){
			std::cerr << "NTP Client Error: " << e.what() << std::endl;
		}
	}

	uint64_t getTime(void) {
				
		uint64_t returnValue = 0;

		boost::array<unsigned char, 48> sendBuf;
		memset(&sendBuf[0], 0, 48);

		sendBuf[0]= NTP_MODE_CLIENT | (NTP_VERSION << 3);

		m_socket.send_to(boost::asio::buffer(sendBuf), m_receiver);

		boost::array<unsigned long, 1024> recvBuf;
		boost::asio::ip::udp::endpoint sender;
		
		try
		{
			size_t len = m_socket.receive_from( boost::asio::buffer(recvBuf), sender);
			if(len==0) {
				std::cerr << "Received 0 bytes from the NTP server\n";
				return 0;
			}
#if 0
			// NTP debug
			char fn[512];
			static int i = 0;

			sprintf(fn, "dump%d.dat", i++);
			FILE* fp=fopen(fn, "w");
			if(fp) {
				fwrite(&recvBuf[0], len, 1,fp);
				fclose(fp);
			}
#endif

			const uint64_t seconds = ntohl(recvBuf[4]);  
			const uint64_t fraction = ntohl(recvBuf[5]); 
			const uint64_t microSecs = (fraction * 1000000) >> 32;

//			const uint64_t ctimeSeconds = seconds - 2208988800U;  // subtract unix time
//			std::cout << "NTP received " << seconds << ", " << fraction << ", " << ctime((time_t*)&ctimeSeconds);

			static const uint64_t l_ui64MicrosPerSecond = 1000*1000;

			returnValue = (seconds << 32) + microSecs*(0xFFFFFFFF/(l_ui64MicrosPerSecond));

		} catch (std::exception& e){

			std::cerr << "NTP Client error: " << e.what() << std::endl;

		}

		return returnValue;
	}
};

#if defined(TARGET_OS_Windows)

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

	l_ui64Result=((l_ui64Counter/l_ui64Frequency)<<32)+(((l_ui64Counter%l_ui64Frequency)<<32)/l_ui64Frequency);

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
	l_ui64Counter=((l_ui64Counter/1000)<<32)+(((l_ui64Counter%1000)<<32)/1000);

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

uint64_t getSystemTime(void)
{
	FILETIME ft;

	GetSystemTimeAsFileTime(&ft);

	// 100 nanosecond intervals
	uint64_t ll_now = (LONGLONG)ft.dwLowDateTime + ((LONGLONG)(ft.dwHighDateTime) << 32LL);

	static const uint64_t l_ui64IntervalsPerSecond = 10*1000*1000; // 100ns -> ms -> s 

	uint64_t seconds = ll_now / l_ui64IntervalsPerSecond;		
	uint64_t fraction = ll_now % l_ui64IntervalsPerSecond;

	// below in fraction part, scale [0,l_uiIntervalsPerSecond-1] to 32bit integer range
	return (seconds<<32) + fraction*(0xFFFFFFFF/l_ui64IntervalsPerSecond);
}

#endif

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

	l_ui64TimeMicroSecond+=l_ui64SecDiff*1000000;
	l_ui64TimeMicroSecond+=l_ui64USecDiff;

	uint64_t l_ui64Result=((l_ui64TimeMicroSecond/1000000)<<32)+(((l_ui64TimeMicroSecond%1000000)<<32)/1000000);

	return l_ui64Result;
#endif
}

uint64_t getBoostTime(void)
{
	boost::posix_time::ptime epoch(boost::posix_time::from_time_t(0));
	// boost::posix_time::ptime time_t_epoch(boost::local_time::date(1970,1,1)); 

	boost::posix_time::ptime mst1 = boost::posix_time::microsec_clock::local_time();

	boost::posix_time::time_duration td = mst1-epoch;
	
	const uint64_t micros = td.total_microseconds();

	static const uint64_t l_ui64MicrosPerSecond = 1000*1000;

	const uint64_t seconds = micros / l_ui64MicrosPerSecond;
	const uint64_t fraction = micros % l_ui64MicrosPerSecond;

	// below in fraction part, scale [0,l_ui64MicrosPerSecond-1] to 32bit integer range
	const uint64_t retVal =  (seconds<<32) + fraction*(0xFFFFFFFF/l_ui64MicrosPerSecond);

//	std::cout << "poo: " << OpenViBE::ITimeArithmetics::timeToSeconds(retVal) << "\n";

	return retVal;
}

class Measurement
{
public:
	Measurement(void) : start(0),now(0),last(0),errors(0) { };
	uint64_t start;
	uint64_t now;
	uint64_t last;
	uint64_t errors;
};

// #include <ntstatus.h>

#if 0
// hidden functions
#pragma comment(lib, "ntdll")

extern "C" {
ULONG NtSetTimerResolution(
		IN  ULONG   desiredResolution,
		IN  BOOLEAN setResolution,
		OUT PULONG  currentResolution);

ULONG NtQueryTimerResolution(
		OUT PULONG minimumResolution,
		OUT PULONG maximumResolution,
		OUT PULONG currentResolution);
}; /* extern "C" */
#endif


int main(int argc, char** argv)
{
	int64_t runTime = 1;
	int64_t sleepTime = 0;

	if (argc > 1)
	{
		printf("Will run for %s seconds\n", argv[1]);
		runTime = atoi(argv[1]);
	}

	if (argc > 2)
	{
		printf("Will sleep for %s milliseconds between calls\n", argv[2]);
		sleepTime = atoi(argv[2]);
	}
	
//	unsigned long min, max, actual;
//	NtQueryTimerResolution(&min, &max, &actual);

	const uint64_t ntpInterval = (2LL*60LL) << 32;	// 2 minutes in seconds, ov time
	const bool ntpEnabled = false;
	const double errorThMs = 5.0;					// If clock delta is more than this, count an error

#if defined(TARGET_OS_Windows)
	const uint32_t preferredInterval = 1;			// set the preferred clock interval, in ms

	TIMECAPS caps;
	timeGetDevCaps(&caps, sizeof(TIMECAPS));

	printf("System supported timer period interval is [%d,%d] ms.\n", caps.wPeriodMin, caps.wPeriodMax);
	
	printf("Setting interval to %d ms\n", preferredInterval);
	MMRESULT result = timeBeginPeriod(preferredInterval);
	if(result!=0) {
		printf("WARNING: setting time interval returned error code %d\n", result);
	}
	if(sleepTime<preferredInterval) {
		printf("WARNING: sleep time %d ms is smaller than the preferred clock interval %d ms\n", sleepTime, preferredInterval);
	}
#endif

#if 0
	// boost time test
	boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
	boost::this_thread::sleep(boost::posix_time::millisec(500));
	boost::posix_time::ptime t2 = boost::posix_time::second_clock::local_time();
	boost::posix_time::time_duration diff = t2 - t1;
	std::cout << "boost1 : " << diff.total_milliseconds() << std::endl;

	boost::posix_time::ptime mst1 = boost::posix_time::microsec_clock::local_time();
	boost::this_thread::sleep(boost::posix_time::millisec(500));
	boost::posix_time::ptime mst2 = boost::posix_time::microsec_clock::local_time();
	boost::posix_time::time_duration msdiff = mst2 - mst1;
	std::cout << "boost2 : " << msdiff.total_milliseconds() << std::endl;
#endif

	printf("Run time: %lld (s), Sleep time: %lld (ms)\n", runTime, sleepTime);

	char filename[255];
	sprintf(filename, "timetest-%lld-%lld-elapsed.csv", runTime, sleepTime);
	FILE* f = fopen(filename, "w");
	sprintf(filename, "timetest-%lld-%lld-delta.csv", runTime, sleepTime);
	FILE* fd = fopen(filename, "w");
	sprintf(filename, "timetest-%lld-%lld-errors.csv", runTime, sleepTime);
	FILE* fe = fopen(filename, "w");

	// @warning Use a server at home that you can access.
	NTPClient NTP("ntp.inria.fr");

	Measurement timeSystem;
	Measurement timeBoost;
	Measurement timeZ;
	Measurement timeZ1;
	Measurement timeZ2;
	Measurement timeNTP;

	timeBoost.start = getBoostTime();
	timeBoost.last = timeBoost.start;

	timeZ.start= zgetTime();
	timeZ.last = timeZ.start;

#if defined(TARGET_OS_Windows)
	timeSystem.start = getSystemTime();
	timeSystem.last = timeSystem.start;

	timeZ1.start= zgetTime1();
	timeZ1.last = timeZ1.start;

	timeZ2.start= zgetTime2();
	timeZ2.last = timeZ2.start;
#endif

	timeNTP.start = (ntpEnabled ? NTP.getTime() : 0);
	timeNTP.last = timeNTP.start;
	uint64_t ntpPolled = timeSystem.start;

	fprintf(f, "Record,Expect,SystemTime,BoostTime,zGetTime,Z1,Z2,NTPTime\n");
	fprintf(fd, "Record,SystemTime,BoostTime,zGetTime,Z1,Z2,NTPTime\n");
	fprintf(fe, "SystemTime,BoostTime,zGetTime,Z1,Z2,NTPTime\n");

	int i = 0;
	while (timeZ.last < timeZ.start + (runTime << 32))
	{
		if (sleepTime > 0)
		{
			boost::this_thread::sleep(boost::posix_time::millisec(sleepTime));
			// Sleep(sleepTime);
		}

		timeBoost.now = getBoostTime();
		timeZ.now = zgetTime();

#if defined(TARGET_OS_Windows)
		timeSystem.now = getSystemTime();
		timeZ1.now = zgetTime1();
		timeZ2.now = zgetTime2();
#endif
		if(ntpEnabled && ntpPolled + ntpInterval <= timeSystem.now)
		{
			timeNTP.now = NTP.getTime();
			ntpPolled = timeSystem.now;
		}
		else
		{
			timeNTP.now = timeNTP.last;
		}

		// const uint64_t dSystem = timeSystem.now - timeSystem.last;
		// std::cout << i << " : " << (dSystem>>32) << ", " << (dSystem & 0x0000000FFFFFFFFLL) << "\n";
		// const uint64_t dBoost = timeBoost.now - timeBoost.last;
		// std::cout << i << " : " << (dBoost>>32) << ", " << (dBoost & 0x0000000FFFFFFFFLL) << " -> " << OpenViBE::ITimeArithmetics::timeToSeconds(dBoost) << "\n";
		// std::cout << i << " : " << timeSystem.start << " " << timeSystem.now << "\n";

		const double diffSystem = OpenViBE::ITimeArithmetics::timeToSeconds(timeSystem.now - timeSystem.last) * 1000.0;
		const double diffBoost = OpenViBE::ITimeArithmetics::timeToSeconds(timeBoost.now - timeBoost.last) * 1000.0;
		const double diffZ = OpenViBE::ITimeArithmetics::timeToSeconds(timeZ.now - timeZ.last) * 1000.0;
		const double diffZ1 = OpenViBE::ITimeArithmetics::timeToSeconds(timeZ1.now - timeZ1.last) * 1000.0;
		const double diffZ2 = OpenViBE::ITimeArithmetics::timeToSeconds(timeZ2.now - timeZ2.last) * 1000.0;
		const double diffNTP = OpenViBE::ITimeArithmetics::timeToSeconds(timeNTP.now - timeNTP.last) * 1000.0;

		if( fabs(diffSystem - sleepTime) > errorThMs ) timeSystem.errors++;
		if( fabs(diffBoost - sleepTime) > errorThMs ) timeBoost.errors++;
		if( fabs(diffZ - sleepTime) > errorThMs ) timeZ.errors++;
		if( fabs(diffZ1 - sleepTime) > errorThMs ) timeZ1.errors++;
		if( fabs(diffZ2 - sleepTime) > errorThMs ) timeZ2.errors++;
		// if( fabs(diffNTP - sleepTime) > 0.1*sleepTime ) timeNTP.errors++;

		const double elapsedSystem = OpenViBE::ITimeArithmetics::timeToSeconds(timeSystem.now - timeSystem.start) * 1000.0;
		const double elapsedBoost = OpenViBE::ITimeArithmetics::timeToSeconds(timeBoost.now - timeBoost.start) * 1000.0;
		const double elapsedZ = OpenViBE::ITimeArithmetics::timeToSeconds(timeZ.now - timeZ.start) * 1000.0;
		const double elapsedZ1 = OpenViBE::ITimeArithmetics::timeToSeconds(timeZ1.now - timeZ1.start) * 1000.0;
		const double elapsedZ2 = OpenViBE::ITimeArithmetics::timeToSeconds(timeZ2.now - timeZ2.start) * 1000.0;
		const double elapsedNTP = OpenViBE::ITimeArithmetics::timeToSeconds(timeNTP.now - timeNTP.start) * 1000.0;

		// fprintf(f, "Cycle %d\n", i);
		// fprintf(f, "SystemTime: [%lld]\n", Delta(start_st, st) / 10000);
		// fprintf(f, "zGetTime: %lf\n", (zt >> 22) / 1024.0 * 1000);
		//fprintf(f, "%d,%lf,%lf,%lf\n", i, st_d, zt_d, zt_d - st_d);
		// Times elapsed since start
		fprintf(f, "%d,%lld,%lf,%lf,%lf,%lf,%lf,%lf\n", i, (i+1)*sleepTime, elapsedSystem, elapsedBoost, elapsedZ, elapsedZ1, elapsedZ2, elapsedNTP);
		// Timediff since last measurement
		fprintf(fd, "%d,%lf,%lf,%lf,%lf,%lf,%lf\n", i, diffSystem, diffBoost, diffZ, diffZ1, diffZ2, diffNTP);
		i++;

		timeSystem.last = timeSystem.now;
		timeBoost.last = timeBoost.now;
		timeZ.last = timeZ.now;
		timeZ1.last = timeZ1.now;
		timeZ2.last = timeZ2.now;
		timeNTP.last = timeNTP.now;
	}

	fclose(f);
	fclose(fd);

	fprintf(fe, "%f %f %f %f %f\n", 
		timeSystem.errors/(float)i,
		timeBoost.errors/(float)i,
		timeZ.errors/(float)i,
		timeZ1.errors/(float)i,
		timeZ2.errors/(float)i
		);
	fclose(fe);

#if defined(TARGET_OS_Windows)
	timeEndPeriod(preferredInterval);
#endif

	return 0;
}