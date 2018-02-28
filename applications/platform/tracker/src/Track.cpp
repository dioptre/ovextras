
#include "Track.h"
#include "Source.h"

#include "StreamSignal.h"
#include "StreamStimulation.h"

#include <iostream>

bool Track::initialize(const char *filename) 
{ 
	Source loader;

	loader.initialize(filename);
		
//	uint32 cnt = 0; 
	MemoryBufferWithType buf;
	while(loader.pullChunk(buf))
	{	
		auto it = m_Streams.find(buf.streamIndex);
		if(it == m_Streams.end())
		{
			// New stream; could use factory here
			if(buf.streamType==OV_TypeId_Signal)
			{
				m_Streams[buf.streamIndex] = new StreamSignal(m_KernelContext);
				m_StreamPosition[buf.streamIndex] = 0;
			}
			else if (buf.streamType==OV_TypeId_Stimulations)
			{
				m_Streams[buf.streamIndex] = new StreamStimulation(m_KernelContext);
				m_StreamPosition[buf.streamIndex] = 0;
			}
			else
			{
				std::cout << "No constructor for " << m_KernelContext.getTypeManager().getTypeName(buf.streamType) << "\n";
				continue;
			}
		}
		m_Streams[buf.streamIndex]->push(buf);
	}

	return true;
};

bool Track::uninitialize(void)
{
	return true;
//	return m_Dataset.uninitialize();
}

uint32_t Track::getSamplingRate(void) 
{
	// @fixme picks rate of first signal stream
	for(auto it = m_Streams.begin();it!=m_Streams.end();it++)
	{
		Stream* ptr = it->second;
		if( ptr->getTypeIdentifier()==OV_TypeId_Signal)
		{
			StreamSignal* tmp = reinterpret_cast<StreamSignal*>(ptr);
			return tmp->getSamplingRate();
		}
	}
	return 0;
}

bool Track::getNextStream(Stream** output)
{
	if(m_Streams.size()==0)
	{
		return false;
	}

	// @todo: check selection here

	// Find the stream with the earliest chunk, return the stream
	Stream* earliestPtr = nullptr;
	uint64_t earliest = std::numeric_limits<uint64_t>::max();

	for(auto it = m_Streams.begin();it!=m_Streams.end();it++)
	{
		Stream* ptr = it->second;

		const StreamChunk* nextChunk;
		if(ptr->peek(&nextChunk) && nextChunk->m_bufferStart < earliest)
		{
			earliest = nextChunk->m_bufferStart;
			earliestPtr = it->second;
		}
	}

	if(earliestPtr==nullptr)
	{
		std::cout << "All streams exhausted\n";
		return false;
	}

	*output = earliestPtr;

	return true;

}
