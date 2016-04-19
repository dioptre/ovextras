
#include <string>

//Components of the Boost Library
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/config.hpp>

using namespace std;

class NTPClient 
{
	private:
		boost::asio::io_service m_service;
		boost::asio::ip::udp::endpoint m_receiver;
		boost::asio::ip::udp::socket m_socket;
		boost::scoped_ptr<boost::thread> m_pThread;
		boost::mutex m_oMutex;
		uint32_t m_ntpIntervalMs;
		uint64_t m_ui64BufferedTime;
		uint64_t m_ui64NumCalls;
		uint64_t (*m_pLocalClock)(void);

	private:
		void pollTime(void);
		void loop(void);

	public:

		NTPClient(string hostName, uint32_t ntpIntervalMs, uint64_t (*localClock)(void));

		~NTPClient();

		uint64_t getTime(void);

};
