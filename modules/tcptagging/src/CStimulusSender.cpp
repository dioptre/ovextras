
#include "CStimulusSender.h"

#include <iostream>
#include <boost/array.hpp>
#include <sys/timeb.h>

#include <system/ovCTime.h>

using namespace TCPTagging;

using boost::asio::ip::tcp;

// @fixme Should use some logging facility instead of std::cout

CStimulusSender::~CStimulusSender()
{
	if(m_oStimulusSocket.is_open())
	{
		m_oStimulusSocket.close();
	}
}
	
boolean CStimulusSender::connect(const char* sAddress, const char* sStimulusPort)
{
	tcp::resolver resolver(m_ioService);
			
	// Stimulus port
	std::cout << "Connecting to Acquisition Server's TCP Tagging [" << sAddress << " , port " << sStimulusPort << "]\n";
	try
	{
		boost::system::error_code error;

		tcp::resolver::query query = tcp::resolver::query(tcp::v4(), sAddress, sStimulusPort, boost::asio::ip::resolver_query_base::numeric_service);
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		m_oStimulusSocket.connect(*endpoint_iterator, error);
		if(error)
		{
			std::cout << "-- Boost ASIO connection error: " << error << "\n";
			return false;
		}
	} 
	catch (boost::system::system_error l_oError) 
	{
		std::cout << "-- Issue '" << l_oError.code().message().c_str() << "' with opening connection to server\n";
		return false;
	}
		
	m_bConnectedOnce = true;
	m_lastTimestamp = 0;

	return true;
}

boolean CStimulusSender::sendStimulation(uint64_t ui64Stimulation, uint64_t ui64Timestamp /* = 0 */, uint64_t ui64Flags /* = FPTIME|CLIENTSIDE */) 
{
	if(!m_bConnectedOnce) {
		return false;
	}	

	if(!m_oStimulusSocket.is_open())
	{
		std::cout << "Error: Cannot send stimulation, socket is not open\n";
		return false;
	}

	if(ui64Flags & IStimulusSender::TCP_Tagging_Flags::FLAG_AUTOSTAMP_CLIENTSIDE)
	{
		ui64Timestamp = System::Time::zgetTimeRaw(false);
		ui64Flags |= IStimulusSender::TCP_Tagging_Flags::FLAG_FPTIME;
	}

	if(ui64Timestamp < m_lastTimestamp)
	{
		std::cout << "Error: Stimulations must be inserted in increasing time order (now: "
			<< ui64Timestamp << ", prev: " << m_lastTimestamp << ", stim=" << ui64Stimulation << ")\n";
		return false;
	}
	m_lastTimestamp = ui64Timestamp;

	try
	{
		boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&ui64Flags, sizeof(uint64)));
		boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&ui64Stimulation, sizeof(uint64)));
		boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&ui64Timestamp, sizeof(uint64)));
	} 
	catch (boost::system::system_error l_oError) 
	{
		std::cout << "Issue '" << l_oError.code().message().c_str() << "' with writing stimulus to server\n";
		return false;
	}

	return true;
}

