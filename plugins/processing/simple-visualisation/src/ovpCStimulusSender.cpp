
#include "ovpCStimulusSender.h"

#include <iostream>
#include <boost/array.hpp>
#include <sys/timeb.h>

using namespace OpenViBE;
using namespace OpenViBEPlugins::SimpleVisualisation;

using boost::asio::ip::tcp;

// @fixme we're using std::cout here and there as this component may be run out of a box process() scope where the LogManager is available...

StimulusSender::~StimulusSender()
{
	if(m_oStimulusSocket.is_open())
	{
		m_oStimulusSocket.close();
	}
}
	
boolean StimulusSender::connect(const char* sAddress, const char* sStimulusPort)
{
	tcp::resolver resolver(m_ioService);
			
	// Stimulus port
	std::cout << "Connecting to stimulus port [" << sAddress << " : " << sStimulusPort << "]\n";
	try
	{
		boost::system::error_code error;

		tcp::resolver::query query = tcp::resolver::query(tcp::v4(), sAddress, sStimulusPort, boost::asio::ip::resolver_query_base::numeric_service);
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		m_oStimulusSocket.connect(*endpoint_iterator, error);
		if(error)
		{
			std::cout << "Connection error: " << error << "\n";
			return false;
		}
	} 
	catch (boost::system::system_error l_oError) 
	{
		std::cout << "Issue '" << l_oError.code().message().c_str() << "' with opening connection to server\n";
		return false;
	}
		
	m_bConnectedOnce = true;

	return true;
}

boolean StimulusSender::sendStimulation(uint64 ui64Stimulation) 
{
	if(!m_bConnectedOnce) {
		return false;
	}	

	timeb time_buffer;
	ftime(&time_buffer);
	// const uint64 posixTime = time_buffer.time*1000ULL + time_buffer.millitm;

	if(!m_oStimulusSocket.is_open())
	{
		std::cout << "Error: Cannot send stimulation, socket is not open\n";
		return false;
	}

	uint64 l_ui64tmp = 0;
	try
	{
		boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&l_ui64tmp, sizeof(uint64)));
		boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&ui64Stimulation, sizeof(uint64)));
		//boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&posixTime, sizeof(uint64)));
		boost::asio::write(m_oStimulusSocket, boost::asio::buffer((void *)&l_ui64tmp, sizeof(uint64)));
	} 
	catch (boost::system::system_error l_oError) 
	{
		std::cout << "Issue '" << l_oError.code().message().c_str() << "' with writing stimulus to server\n";
	}

	return true;
}
