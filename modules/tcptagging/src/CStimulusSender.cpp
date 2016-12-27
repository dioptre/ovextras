
#include "CStimulusSender.h"

#include <iostream>
#include <boost/array.hpp>
#include <sys/timeb.h>

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

	return true;
}

boolean CStimulusSender::sendStimulation(uint64 ui64Stimulation, uint64 ui64Timestamp /* = 0 */) 
{
	if(!m_bConnectedOnce) {
		return false;
	}	

	if(!m_oStimulusSocket.is_open())
	{
		std::cout << "Error: Cannot send stimulation, socket is not open\n";
		return false;
	}

	uint64 l_ui64Zero = 0;
	try
	{
		boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&l_ui64Zero, sizeof(uint64)));
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
