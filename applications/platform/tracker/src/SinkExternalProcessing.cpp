
#include "SinkExternalProcessing.h"

#include <iostream>
#include <thread>
#include <deque>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

#include <algorithm> // std::max

using namespace OpenViBE;
using namespace OpenViBE::Kernel;

// thread::
// wait for connection;
// while(connected ||!quit)
// {
//    waitForChunk();
//    pushChunk();
// }
// if(quit) {
//	  pushEndStim(); // XML have player controller that will quit
// }
// exit;

// #include "StreamChunk.h"
// #include "StreamSignalChunk.h"
// #include "StreamStimulationChunk.h"

#include "Stream.h"
#include "TypeSignal.h"
#include "TypeStimulation.h"

using namespace Communication;

void playerLaunch(const char *xmlFile, bool playFast); // In sink.cpp

bool SinkExternalProcessing::configureSink(void)
{
	if(m_xmlFilename.length()==0) 
	{
		std::cout << "Error: Please set sink filename first\n";
		return false;
	}

	std::string Designer = std::string(OpenViBE::Directories::getBinDir().toASCIIString()) + "/openvibe-designer --no-session-management --open ";
	std::string OutputDump = std::string(OpenViBE::Directories::getDistRootDir().toASCIIString()) + "/tracker-sink-configure-dump.txt";

	auto cmd = Designer + std::string(m_xmlFilename) + " >" + OutputDump;

	if(system(cmd.c_str())!=0)
	{
		std::cout << "Launch of [" << cmd.c_str() << "] failed\n";
		return false;
	}
	return true;
}


bool SinkExternalProcessing::initialize(const char *xmlFile) 
{ 
	if(!xmlFile || !xmlFile[0])
	{
		std::cout << "Error: Cannot initialize sink without a file\n";
		return false;
	}

	std::cout << "Sink: Initializing with "	<< xmlFile << "\n";
	
	m_pConnectionServer = NULL;
	m_ConnectionPort = 1024;
	m_ChunksSent = 0;
	m_xmlFilename = xmlFile;

	m_ClientHandlerThread = nullptr;
	m_pPlayerThread = nullptr;
	m_headerSent = false;


	return true; 
}

bool SinkExternalProcessing::uninitialize(void) 
{ 
	std::cout << "Sink: Uninitializing\n";

	stop();

	return true;
}


bool SinkExternalProcessing::pull(Track& source)
{
	auto streams = source.getAllStreams();

	if(!m_headerSent)
	{
		// Push headers
		for(size_t i=0;i<streams.size();i++)
		{
			if(streams[i])
			{
				streams[i]->initializeEBMLExport();

				std::vector<uint8_t> data;
				streams[i]->getEBMLHeader(data);

				if (!m_client.pushEBML(i, 0, 0, std::make_shared<const std::vector<uint8_t>>(data)))
				{
					std::cerr << "Failed to push EBML Header.\n";
					std::cerr << "Error " << m_client.getLastError() << "\n";
					return false;
				}
			}
		}

		m_client.pushSync();

		while (!m_client.isSyncMessageReceived())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		m_headerSent = true;
	}

	int nextStream;
	if(!source.getNextStreamIndex(nextStream))
	{
		// Push end: Declare all streams as ended
		for(size_t i=0;i<streams.size();i++)
		{
			if(streams[i])
			{
				std::vector<uint8_t> data;
				uint64_t start,end;
				streams[i]->getEBMLEnd(data, start, end);

				if (!m_client.pushEBML(i, start, end, std::make_shared<const std::vector<uint8_t>>(data)))
				{
					std::cerr << "Failed to push EBML End.\n";
					std::cerr << "Error " << m_client.getLastError() << "\n";
					return false;
				}
			}
		}

		m_client.pushSync();

		while (!m_client.isSyncMessageReceived())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		return false;
	}

	// Push data chunk
	std::vector<uint8_t> data;
	uint64_t start, end;
	streams[nextStream]->getEBMLChunk(data, start, end);

	// Wait until its time
	while(!m_PlayFast && System::Time::zgetTime()-m_startTime < start)
	{
		// std::this_thread::sleep_for(std::chrono::milliseconds(1));	
		std::this_thread::yield();
	}

	if (!m_client.pushEBML(nextStream, start, end, std::make_shared<const std::vector<uint8_t>>(data)))
	{
		std::cerr << "Failed to push EBML data chunk.\n";
		std::cerr << "Error " << m_client.getLastError() << "\n";
		return false;
	}

	m_client.pushSync();

	m_ChunksSent++;

	if(!streams[nextStream]->step())
	{
		std::cout << "Can not step further\n";
		return false;
	}

	return true;
}

bool SinkExternalProcessing::pull(StreamBase* stream)
{
	std::cout << "Not supported";
	return false;
}

bool SinkExternalProcessing::play(bool playFast)
{
	if(m_xmlFilename.length()==0)
	{
		std::cout << "Error: No sink initialized\n";
		return false;
	}

	m_ChunksSent = 0;
	m_PreviousChunkEnd = 0;
	m_PlayFast = playFast;	// @todo To work neatly it'd be better to be able to pass in the chunk times to the designer side
	m_headerSent = false;

	m_pPlayerThread = new std::thread(std::bind(&playerLaunch, m_xmlFilename.c_str(), m_PlayFast));

	std::string connectionID;
	m_client.setConnectionID(connectionID);
	int errorCount = 0;
	while(!m_client.connect("127.0.0.1", 49152))
	{
		MessagingClient::ELibraryError error = m_client.getLastError();
		
		if (error == MessagingClient::ELibraryError::Socket_FailedToConnect)
		{
			printf("Server not responding\n");
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			if(errorCount++>5) 
			{
				return false; 
			}
		}
		else
		{
			printf("Error %d\n", error);
			return false;
		}

		/*
		if (s_DidRequestForcedQuit)
		{
			exit(EXIT_SUCCESS);
		}
		*/
	}

	m_startTime = System::Time::zgetTime();

	// Announce to server that the box has finished initializing and wait for acknowledgement
	while (!m_client.isSyncMessageReceived())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	m_client.pushLog(Communication::ELogLevel::LogLevel_Info, "Received Ping");

	m_client.pushSync();
	m_client.pushLog(Communication::ELogLevel::LogLevel_Info, "Sent Pong");

	return true;
}

bool SinkExternalProcessing::stop(void)
{
	if(m_client.isConnected())
	{
		m_client.close();
	}

	// tear down the player object
	if(m_pPlayerThread)
	{
		std::cout << "Joining player thread\n";
		m_pPlayerThread->join();
		delete m_pPlayerThread;
		m_pPlayerThread = nullptr;
	}


	return true;
}


uint64_t SinkExternalProcessing::getCurrentTime(void) const
{
	if(m_SamplingRate==0)
	{
		return 0;
	}
	return ITimeArithmetics::sampleCountToTime(m_SamplingRate, m_ChunksSent*m_ChunkSize);
}

