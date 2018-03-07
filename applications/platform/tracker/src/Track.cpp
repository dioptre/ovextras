
#include <iostream>

#include "Track.h"
#include "Source.h"

#include "Stream.h"
#include "TypeSignal.h"
#include "TypeStimulation.h"

#include "DecoderSignal.h"
#include "DecoderStimulation.h"

bool Track::initialize(const char *filename) 
{ 
	//reset first
	uninitialize();

	Source loader;

	loader.initialize(filename);
	
//	uint32 cnt = 0; 
	MemoryBufferWithType buf;
	while(loader.pullChunk(buf))	
	{	
		if(buf.streamIndex>=m_Streams.size())
		{
			// New stream; could use factory here
			m_Streams.resize(buf.streamIndex+1, nullptr);
			m_Decoders.resize(buf.streamIndex+1, nullptr);

			if(buf.streamType==OV_TypeId_Signal)
			{
				Stream<TypeSignal>* tmp = new Stream<TypeSignal>();
				m_Streams[buf.streamIndex] = tmp;
				m_Decoders[buf.streamIndex] = new DecoderSignal(m_KernelContext, tmp);
			}
			else if (buf.streamType==OV_TypeId_Stimulations)
			{
				Stream<TypeStimulation>* tmp = new Stream<TypeStimulation>();
				m_Streams[buf.streamIndex] = tmp;
				m_Decoders[buf.streamIndex] = new DecoderStimulation(m_KernelContext, tmp);
			}
			else
			{
				std::cout << "No constructor for " << m_KernelContext.getTypeManager().getTypeName(buf.streamType) << "\n";
				continue;
			}
		}

		if(m_Decoders[buf.streamIndex])
		{
			m_Decoders[buf.streamIndex]->decode(buf);
		}
	}
	
	std::cout << "Streams initialized ok\n";

	return true;
};

bool Track::uninitialize(void)
{
	for(size_t i=0;i<m_Decoders.size();i++)
	{
		delete m_Decoders[i];
	}
	m_Decoders.clear();
	for(size_t i=0;i<m_Streams.size();i++)
	{
		delete m_Streams[i];
	}
	m_Streams.clear();

	return true;
}

bool Track::rewind(void)
{
	bool returnValue = true;

	std::for_each(m_Streams.begin(), m_Streams.end(), [&returnValue](StreamBase* entry) { returnValue &= entry->rewind(); } );

	return returnValue;
}

uint64_t Track::getSamplingRate(void) const
{
	// @fixme picks rate of first signal stream
	for(auto it = m_Streams.begin();it!=m_Streams.end();it++)
	{
		if( (*it)->getTypeIdentifier()==OV_TypeId_Signal)
		{
			Stream<TypeSignal>* tmp = reinterpret_cast< Stream<TypeSignal>* >(*it);
			const TypeSignal::Header& head = tmp->getHeader();
			return head.m_samplingFrequency;
		}
	}
	return 0;
}

const StreamBase* Track::getStream(uint64_t idx) const 
{
	if(idx<m_Streams.size())
	{
		return m_Streams[idx];
	}
	else
	{
		return nullptr;
	}
}

bool Track::getNextStream(StreamBase** output)
{
	if(m_Streams.size()==0)
	{
		return false;
	}

	// @todo: check selection here

	// Find the stream with the earliest chunk, return the stream
	StreamBase* earliestPtr = nullptr;
	uint64_t earliest = std::numeric_limits<uint64_t>::max();

	for(auto it = m_Streams.begin();it!=m_Streams.end();it++)
	{
		StreamBase* ptr = *it;

		const TypeBase::Buffer* nextChunk;
		if(ptr->peek(&nextChunk) && nextChunk->m_bufferStart < earliest)
		{
			earliest = nextChunk->m_bufferStart;
			earliestPtr = *it;
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
