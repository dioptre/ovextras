
#include <iostream>
#include <string>

#include "ntpclient.h"

using namespace std;

#define NTP_MODE_CLIENT 3
#define NTP_VERSION 3
#define NTP_TRANSMIT 40

// \fixme std::cout/cerr not safe from a thread
NTPClient::NTPClient(string hostName, uint32_t ntpIntervalMs, uint64_t (*localClock)(void))
		: m_socket(m_service), m_ntpIntervalMs(ntpIntervalMs), m_ui64BufferedTime(0), m_ui64NumCalls(0), m_pLocalClock(localClock)
{
	if(m_ntpIntervalMs == 0) 
	{
		std::cout << "NTP client disabled\n";
		return;
	}

	std::cout << "NTP set to poll [" << hostName << "] every " << m_ntpIntervalMs << "ms\n";

	boost::asio::ip::udp::resolver resolver(m_service);
	boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), hostName, "ntp");

	try { 
		m_receiver = *resolver.resolve(query);
		m_socket.open(boost::asio::ip::udp::v4());
	} catch (std::exception& e){
		std::cerr << "NTP Client Error: " << e.what() << std::endl;
	}

	m_pThread.reset(new boost::thread(boost::bind(&NTPClient::loop, this )));
}

NTPClient::~NTPClient()
{
	if(m_socket.is_open())
	{
		m_socket.close();
	}
}

void NTPClient::loop(void)
{
	while(1)
	{
		pollTime();

		boost::this_thread::sleep(boost::posix_time::millisec(m_ntpIntervalMs));
	}
}

uint64_t NTPClient::getTime(void) {
	uint64_t returnValue = 0;

	{
		boost::mutex::scoped_lock lock(m_oMutex);	
		returnValue = m_ui64BufferedTime;
//		std::cout << "NTP returning " << (m_bufferedTime >> 32) << "sec\n";
	}

	return returnValue;
}

void NTPClient::pollTime(void) {
				
	boost::array<unsigned char, 48> sendBuf;
	memset(&sendBuf[0], 0, 48);

	sendBuf[0]= NTP_MODE_CLIENT | (NTP_VERSION << 3);

	const uint64_t l_ui64ClientSendTime = (m_pLocalClock ? m_pLocalClock() : 0);

	// *(uint64_t *)(&sendBuf[0] + NTP_TRANSMIT) = (m_ui64NumCalls << 32);
	//m_ui64NumCalls++;

	m_socket.send_to(boost::asio::buffer(sendBuf), m_receiver);

	boost::array<unsigned long, 1024> recvBuf;
	boost::asio::ip::udp::endpoint sender;
		
	try
	{
		const size_t len = m_socket.receive_from( boost::asio::buffer(recvBuf), sender);
		if(len==0) {
			std::cerr << "NTP Client error: Received 0 bytes from the NTP server\n";
			return;
		}

		const uint64_t l_ui64ClientReceiveTime = (m_pLocalClock ? m_pLocalClock() : 0);

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

		const uint64_t l_ui64ServerReceiveTimeSeconds = ntohl(recvBuf[8]);
		const uint64_t l_ui64ServerReceiveTimeFraction = ntohl(recvBuf[9]);
		const uint64_t l_ui64ServerReceiveTime = (l_ui64ServerReceiveTimeSeconds << 32) + l_ui64ServerReceiveTimeFraction;

		const uint64_t l_ui64ServerSendTimeSeconds = ntohl(recvBuf[10]);
		const uint64_t l_ui64ServerSendTimeFraction = ntohl(recvBuf[11]);
		const uint64_t l_ui64ServerSendTime = (l_ui64ServerSendTimeSeconds << 32) + l_ui64ServerSendTimeFraction;

		// const uint64_t l_tmpSeconds = (l_ui64ServerReceiveTime>>32) -2208988800U;
		// std::cout << "NTP received at " << ctime((time_t*)&l_tmpSeconds);

		{
			boost::mutex::scoped_lock lock(m_oMutex);	

			// super ad-hoc correction to get server time-at-call
			const uint64_t l_ui64ServerDelay = l_ui64ServerSendTime - l_ui64ServerReceiveTime;
			const uint64_t l_ui64TransmissionDelay = l_ui64ClientReceiveTime - l_ui64ClientSendTime;
			const uint64_t l_ui64TransmissionTook = (l_ui64TransmissionDelay - l_ui64ServerDelay) / 2;

			m_ui64BufferedTime = l_ui64ServerReceiveTime - l_ui64TransmissionTook;

//			std::cout << "NTP returning " << (m_ui64BufferedTime >> 32) << "sec, trd = " << (l_ui64TransmissionTook>>32) << "\n";
		}

	} catch (std::exception& e){

		std::cerr << "NTP Client error: " << e.what() << std::endl;

	}
}

