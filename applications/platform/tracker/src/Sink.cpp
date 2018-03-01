
#include "Sink.h"

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

class OutputEncoder
{
public:
	OutputEncoder(OpenViBE::Kernel::IKernelContext& ctx) : m_KernelContext(ctx), m_pStreamEncoder(nullptr) { };
	~OutputEncoder() 
	{
		if(m_pStreamEncoder) 
		{
			m_pStreamEncoder->uninitialize();
			m_KernelContext.getAlgorithmManager().releaseAlgorithm(*m_pStreamEncoder);

			ip_ui64SubjectIdentifier.uninitialize();
			ip_ui64SubjectAge.uninitialize();
			ip_ui64SubjectGender.uninitialize();
			ip_pSignalMatrix.uninitialize();
			ip_ui64SignalSamplingRate.uninitialize();
			ip_pStimulationSet.uninitialize();
			ip_ui64BufferDuration.uninitialize();
			op_pEncodedMemoryBuffer.uninitialize();
			ip_pChannelUnits.uninitialize();

			ip_bEncodeChannelLocalisationData.uninitialize();
			ip_bEncodeChannelUnitData.uninitialize();
		}
	}

	bool initialize(void)
	{
		m_pStreamEncoder=&m_KernelContext.getAlgorithmManager().getAlgorithm(m_KernelContext.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_MasterAcquisitionStreamEncoder));
		if(!m_pStreamEncoder->initialize())
		{
			return false;
		}

		ip_ui64SubjectIdentifier.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_SubjectIdentifier));
		ip_ui64SubjectAge.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_SubjectAge));
		ip_ui64SubjectGender.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_SubjectGender));
		ip_pSignalMatrix.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_SignalMatrix));
		ip_ui64SignalSamplingRate.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_SignalSamplingRate));
		ip_pStimulationSet.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_StimulationSet));
		ip_ui64BufferDuration.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_BufferDuration));
		ip_pChannelUnits.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_ChannelUnits));
		op_pEncodedMemoryBuffer.initialize(m_pStreamEncoder->getOutputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_OutputParameterId_EncodedMemoryBuffer));	

		ip_bEncodeChannelLocalisationData.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_EncodeChannelLocalisationData));
		ip_bEncodeChannelUnitData.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_EncodeChannelUnitData));

		return true;
	}

	IMemoryBuffer* encodeHeader(void)
	{
		op_pEncodedMemoryBuffer->setSize(0, true);
		m_pStreamEncoder->process(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputTriggerId_EncodeHeader);
		return op_pEncodedMemoryBuffer;
	}

	IMemoryBuffer* encodeBuffer(void)
	{
		op_pEncodedMemoryBuffer->setSize(0, true);
		m_pStreamEncoder->process(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputTriggerId_EncodeBuffer);	
		return op_pEncodedMemoryBuffer;
	}

	IMemoryBuffer* encodeEnd(void)
	{
		op_pEncodedMemoryBuffer->setSize(0, true);
		m_pStreamEncoder->process(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputTriggerId_EncodeEnd);	
		return op_pEncodedMemoryBuffer;
	}

public:
	OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64SubjectIdentifier;
	OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64SubjectAge;
	OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64SubjectGender;
	OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > ip_pSignalMatrix;
	OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > ip_pChannelUnits;
	OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64SignalSamplingRate;
	OpenViBE::Kernel::TParameterHandler < OpenViBE::IStimulationSet* > ip_pStimulationSet;
	OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64BufferDuration;
	OpenViBE::Kernel::TParameterHandler < OpenViBE::IMemoryBuffer* > op_pEncodedMemoryBuffer;

	OpenViBE::Kernel::TParameterHandler < OpenViBE::boolean > ip_bEncodeChannelLocalisationData;
	OpenViBE::Kernel::TParameterHandler < OpenViBE::boolean > ip_bEncodeChannelUnitData;

private:
	OpenViBE::Kernel::IKernelContext& m_KernelContext;
	OpenViBE::Kernel::IAlgorithmProxy* m_pStreamEncoder;

};

class CClientHandler
{
public:
	CClientHandler(Sink& rSink) 
		:m_rSink(rSink)
		,m_bPleaseQuit(false)
		,m_BuffersSent(0)
		,m_con(nullptr)
	{
		m_BufferDuration = ITimeArithmetics::sampleCountToTime(m_rSink.m_SamplingRate, m_rSink.m_ChunkSize);
	}

	Sink& m_rSink;
	bool m_bPleaseQuit;
	std::deque< OpenViBE::IMemoryBuffer* > m_vClientPendingBuffer;
	std::mutex m_oClientThreadMutex;
	std::condition_variable m_oPendingBufferCondition;
	uint64_t m_BufferDuration;
	uint64_t m_BuffersSent;
	Socket::IConnection* m_con;

	static void start_thread(CClientHandler* pHandler)
	{
		(*pHandler)();
	}

	void pushChunk(const OpenViBE::IMemoryBuffer &memoryBuffer)
	{
		{
			std::lock_guard<std::mutex> oLock(m_oClientThreadMutex);
			if(!m_bPleaseQuit)
			{
				CMemoryBuffer* l_pMemoryBuffer=new CMemoryBuffer(memoryBuffer);
				m_vClientPendingBuffer.push_back(l_pMemoryBuffer);
			}
		}

		// No big harm notifying in any case, though if in 'quit' state, the quit request has already notified
		m_oPendingBufferCondition.notify_one();	
	}

	void operator()(void)
	{
		// wait until connection
		m_con = m_rSink.m_pConnectionServer->accept();
		if(!m_con)
		{
			// @fixme buffers might be unflushed in this case
			std::cout << "Error:No connection\n";
			return;
		}

		std::cout << "Got connection, sending...\n";

		// add mutex here that gets chunks from the list and encodes them to the connection
		std::unique_lock<std::mutex> oLock(m_oClientThreadMutex, std::defer_lock);

		const uint64_t startTime = System::Time::zgetTime();

		while(true)
		{
			oLock.lock();

			// Wait until something interesting happens...
			m_oPendingBufferCondition.wait(oLock,
				[this]() { 
					return (m_bPleaseQuit ||  m_con->isConnected() || m_vClientPendingBuffer.size()>0);
				}
			);

			// Exit the loop if we're told to quit or if we've lost the connection
			if(m_bPleaseQuit || !m_con->isConnected())
			{
				oLock.unlock();
				break;
			}

			// At this point, we should have a buffer
			if(!m_vClientPendingBuffer.size())
			{
				// n.b. Shouldn't happen, but we don't have an error reporting mechanism in the thread...
				oLock.unlock();
				continue;
			}

			// Simulate a realtime sending device. @fixme spins thread and only needed for visus
			while(!m_rSink.m_PlayFast) {
				uint64_t elapsed = System::Time::zgetTime() - startTime;
				if(elapsed >= m_BufferDuration*m_BuffersSent) {
					break;
				}
			}

//			std::cout << "Sending out chunk " << m_BuffersSent << " at " << ITimeArithmetics::timeToSeconds(System::Time::zgetTime()) << "\n";

			auto pBuffer=m_vClientPendingBuffer.front();
			m_vClientPendingBuffer.pop_front();

			const uint64 l_ui64MemoryBufferSize=pBuffer->getSize();
			m_con->sendBufferBlocking(&l_ui64MemoryBufferSize, sizeof(l_ui64MemoryBufferSize));
			m_con->sendBufferBlocking(pBuffer->getDirectPointer(), (uint32)pBuffer->getSize());

			delete pBuffer;

			m_BuffersSent++;

			// Don't go into blocking send while holding the lock; ok to unlock as l_pMemoryBuffer ptr+mem is now owned by this thread
			oLock.unlock();
		}

		// client should close the connection on exit

		std::cout << "Flushing remaining " << m_vClientPendingBuffer.size() << " buffers\n";

		oLock.lock();
		for(size_t i=0;i<m_vClientPendingBuffer.size();i++)
		{
			delete m_vClientPendingBuffer[i];
		}
		m_vClientPendingBuffer.clear();
		oLock.unlock();
	}
};

void playerLaunch(const char *xmlFile, bool playFast)
{
	std::string Designer = std::string(OpenViBE::Directories::getBinDir().toASCIIString()) + "/openvibe-designer --no-gui " 
		+ (playFast ? "--play-fast " : "--play ");
	std::string OutputDump = std::string(OpenViBE::Directories::getDistRootDir().toASCIIString()) + "/tracker-dump.txt";

	auto cmd = Designer + std::string(xmlFile) + " >" + OutputDump;

	if(system(cmd.c_str())!=0)
	{
		std::cout << "Launch of [" << cmd.c_str() << "] failed\n";
	}
}

bool Sink::initialize(const char *xmlFile, uint32_t samplingRate) 
{ 
	std::cout << "Sink: Initializing with "	<< xmlFile << "\n";
	
	m_pConnectionServer = NULL;
	const uint32_t ui32ConnectionPort = 1024;
	m_SamplingRate = samplingRate;
	m_ChunkSize = 32;
	m_ChunksSent = 0;
	m_PlayFast = false;	// @todo To work neatly it'd be better to be able to pass in the chunk times to the designer side

	m_ClientHandlerThread = nullptr;
	m_pPlayerThread = nullptr;

	// Create a socket reading thread here
	m_pConnectionServer=Socket::createConnectionServer();
	if(!m_pConnectionServer->listen(ui32ConnectionPort))
	{
		std::cout << "Error listening to port " << ui32ConnectionPort << "\n";
		return false;
	}

	m_OutputEncoder = new OutputEncoder(m_KernelContext);
	if(!m_OutputEncoder->initialize())
	{
		delete m_OutputEncoder;
		return false;
	}

	m_ClientHandler = new CClientHandler(*this);

	// Create a player object with the xml
	m_ClientHandlerThread = new std::thread(std::bind(CClientHandler::start_thread, m_ClientHandler));
	m_pPlayerThread = new std::thread(std::bind(&playerLaunch, xmlFile, m_PlayFast));

	return true; 
}

bool Sink::uninitialize(void) 
{ 
	std::cout << "Sink: Uninitializing\n";

	// tear down the player object
	if(m_pPlayerThread)
	{
		std::cout << "Joining player thread\n";
		m_pPlayerThread->join();
		delete m_pPlayerThread;
		m_pPlayerThread = nullptr;
	}

	// tear down the server thread	
	if(m_ClientHandlerThread)
	{
		std::cout << "Joining client handler thread\n";

		// Use a scoped lock before toggling a variable owned by the thread
		{
			std::lock_guard<std::mutex> oLock(m_ClientHandler->m_oClientThreadMutex);
				
			// Tell the thread to quit
			m_ClientHandler->m_bPleaseQuit = true;
		}

		// Wake up the thread in case it happens to be waiting on the cond var
		m_ClientHandler->m_oPendingBufferCondition.notify_one();
		
		// @fixme potential ouch, but needed if thread is sitting in accept()
		m_pConnectionServer->close();

		m_ClientHandlerThread->join();
		delete m_ClientHandlerThread;
		m_ClientHandlerThread = nullptr;
	}

	if(m_ClientHandler)
	{
		delete m_ClientHandler;
		m_ClientHandler = nullptr;
	}

	if(m_pConnectionServer)
	{
		m_pConnectionServer->release();
		m_pConnectionServer = nullptr;
	}

	if(m_OutputEncoder)
	{
		delete m_OutputEncoder;
		m_OutputEncoder = nullptr;
	}

	return true;
}

#if 0
bool Sink::pushChunk(const StreamChunk* chunk)
{
//	std::cout << "Sink: Queueing chunk\n";

	if(!m_OutputEncoder)
	{
		return false;
	}

#if 0
	if(!playingStarted)
	{
		m_OutputEncoder->ip_ui64SignalSamplingRate = m_SamplingRate;
		m_OutputEncoder->ip_ui64BufferDuration = ITimeArithmetics::sampleCountToTime(m_SamplingRate, m_ChunkSize);
		OpenViBEToolkit::Tools::Matrix::copyDescription(*m_OutputEncoder->ip_pSignalMatrix, chunk);

		m_ClientHandler->pushChunk(*m_OutputEncoder->encodeHeader());

		IStimulationSet* stimSet = m_OutputEncoder->ip_pStimulationSet;
		stimSet->clear();
		stimSet->appendStimulation(OVTK_StimulationId_ExperimentStart, 0, 0);

		playingStarted = true;
	}
	
	for(size_t i=0;i<chunk.getBufferElementCount();i++)
	{
		float32 val = std::abs(chunk.getBuffer()[i]);
		if(val>100000000)
		{
			std::cout << "meh\n";
			break;
		}
	}

	IMatrix* target = m_OutputEncoder->ip_pSignalMatrix;
	OpenViBEToolkit::Tools::Matrix::copy(*target, chunk);

	m_OutputEncoder->ip_pStimulationSet->clear();

	m_ClientHandler->pushChunk(*m_OutputEncoder->encodeBuffer());
	m_ChunksSent++;

#endif

	return true;
}

#endif

bool Sink::pull(StreamBase* stream)
{

	const TypeBase::Buffer* ptr;

	if(!stream->peek(&ptr))
	{
		return false;
	}

	// Flush previous buffers when the current buffer start is older than the previous ones
	if(ptr->m_bufferStart>= m_PreviousChunkEnd && m_PreviousChunkEnd>0)
	{
			m_ClientHandler->pushChunk(*m_OutputEncoder->encodeBuffer());

			m_OutputEncoder->ip_pStimulationSet->clear();
	}
	m_PreviousChunkEnd = std::max(m_PreviousChunkEnd, ptr->m_bufferEnd );

	if(stream->getTypeIdentifier() == OV_TypeId_Signal)
	{
		// Stream<TypeSignal>* sPtr = reinterpret_cast< Stream<TypeSignal>* >(ptr);
		const TypeSignal::Buffer* sPtr = reinterpret_cast<const TypeSignal::Buffer*>(ptr);

		if(!m_SignalHeaderSent)
		{
			m_OutputEncoder->ip_ui64SignalSamplingRate = m_SamplingRate;
			m_OutputEncoder->ip_ui64BufferDuration = ITimeArithmetics::sampleCountToTime(m_SamplingRate, m_ChunkSize);
			OpenViBEToolkit::Tools::Matrix::copyDescription(*m_OutputEncoder->ip_pSignalMatrix, sPtr->m_buffer);

			IStimulationSet* stimSet = m_OutputEncoder->ip_pStimulationSet;
			stimSet->clear();
			stimSet->appendStimulation(OVTK_StimulationId_ExperimentStart, 0, 0);

			m_ClientHandler->pushChunk(*m_OutputEncoder->encodeHeader());

			m_SignalHeaderSent = true;
		}

		IMatrix* target = m_OutputEncoder->ip_pSignalMatrix;
		OpenViBEToolkit::Tools::Matrix::copy(*target, sPtr->m_buffer);



		m_ChunksSent++;

	}
	// @fixme issue here that the acquisition stream apparently actually expects all headers to be sent at once, so we drop all other streams before first signal chunk...
	else if(stream->getTypeIdentifier() == OV_TypeId_Stimulations)
	{
			
//			std::cout << "Stimchunk\n";
		const TypeStimulation::Buffer* sPtr = reinterpret_cast<const TypeStimulation::Buffer*>(ptr);

		// This will be sent when the next signal chunk is sent
		for(size_t i=0;i<sPtr->m_buffer.getStimulationCount();i++)
		{
			m_OutputEncoder->ip_pStimulationSet->appendStimulation(
				sPtr->m_buffer.getStimulationIdentifier(i),
				sPtr->m_buffer.getStimulationDate(i),
				sPtr->m_buffer.getStimulationDuration(i)
				);
		}
	}

	if(!stream->step())
	{
		std::cout << "Can not step further\n";
		return false;
	}

	return true;
}


bool Sink::stop(void)
{
	if(m_SignalHeaderSent)
	{
		const uint64_t endTime = ITimeArithmetics::sampleCountToTime(m_SamplingRate, (m_ChunksSent+1)*m_ChunkSize);

		IStimulationSet* stimSet = m_OutputEncoder->ip_pStimulationSet;
		stimSet->clear();
		stimSet->appendStimulation(OVTK_StimulationId_ExperimentStop, endTime, 0);
		m_ClientHandler->pushChunk(*m_OutputEncoder->encodeBuffer());

		m_ClientHandler->pushChunk(*m_OutputEncoder->encodeEnd());

		m_SignalHeaderSent = false;
	}

	return true;
}