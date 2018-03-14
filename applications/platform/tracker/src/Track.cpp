
#include <iostream>
#include <algorithm>

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
				Stream<TypeSignal>* targetStream = new Stream<TypeSignal>();
				m_Streams[buf.streamIndex] = targetStream;
				m_Decoders[buf.streamIndex] = new DecoderSignal(m_KernelContext, targetStream);
			}
			else if (buf.streamType==OV_TypeId_Stimulations)
			{
				Stream<TypeStimulation>* targetStream = new Stream<TypeStimulation>();
				m_Streams[buf.streamIndex] = targetStream;
				m_Decoders[buf.streamIndex] = new DecoderStimulation(m_KernelContext, targetStream);
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
	
	// Do global DC cuts ... @todo refactor elsewhere, this is horrible
	for(size_t i=0;i<m_Streams.size();i++)
	{
		if(m_Streams[i] && m_Streams[i]->getTypeIdentifier() == OV_TypeId_Signal)
		{
			// Compute DC
			Stream<TypeSignal>* stream = reinterpret_cast< Stream<TypeSignal>* >(m_Streams[i]);
			std::vector<OpenViBE::float64> streamMean;
			streamMean.resize(stream->getHeader().m_header.getDimensionSize(0));

			std::for_each(stream->begin(),stream->end(), 
				[&streamMean](TypeSignal::Buffer* buf) 
				{ 
					std::vector<OpenViBE::float64> chunkSum;
					chunkSum.resize(buf->m_buffer.getDimensionSize(0));
					for(size_t j=0;j<buf->m_buffer.getDimensionSize(0);j++)
					{
						for(size_t k=0;k<buf->m_buffer.getDimensionSize(1);k++)
						{
							chunkSum[j] += buf->m_buffer.getBuffer()[j*buf->m_buffer.getDimensionSize(1)+k];
						}
						streamMean[j] += (chunkSum[j]/buf->m_buffer.getDimensionSize(1));
					}			
				});

			for(size_t j=0;j<streamMean.size();j++)
			{
				streamMean[j] /= stream->getChunkCount();
			}
			std::for_each(stream->begin(),stream->end(),
				[&streamMean](TypeSignal::Buffer* buf)
				{
					OpenViBE::float64* bufPtr = buf->m_buffer.getBuffer();
					for(size_t j=0;j<buf->m_buffer.getDimensionSize(0);j++)
					{
						for(size_t k=0;k<buf->m_buffer.getDimensionSize(1);k++)
						{
							bufPtr[j*buf->m_buffer.getDimensionSize(1)+k] -= streamMean[j];
						}	
					}					
				});

			/*
			for(size_t c=0;c<stream->getChunkCount();c++)
			{
				TypeSignal::Buffer* buf;
				stream->getChunk(c, &buf);
				std::vector<OpenViBE::float64> chunkSum;
				for(size_t j=0;j<buf->m_buffer.getDimensionSize(0);j++)
				{
					for(size_t k=0;k<buf->m_buffer.getDimensionSize(1);k++)
					{
						chunkSum[j] += buf->m_buffer.getBuffer()[j*buf->m_buffer.getDimensionSize(1)+k];
					}
					streamMean[j] += (chunkSum[j]/buf->m_buffer.getDimensionSize(1));
				}
			}
			for(size_t j=0;j<streamMean.size();j++)
			{
				streamMean[j] /= stream->getChunkCount();
			}
			
			// Cut DC
			for(size_t c=0;c<stream->getChunkCount();c++)
			{
				TypeSignal::Buffer* buf;
				stream->getChunk(c, &buf);
				std::vector<OpenViBE::float64> chunkSum;
				OpenViBE::float64* bufPtr = buf->m_buffer.getBuffer();
				for(size_t j=0;j<buf->m_buffer.getDimensionSize(0);j++)
				{
					for(size_t k=0;k<buf->m_buffer.getDimensionSize(1);k++)
					{
						bufPtr[j*buf->m_buffer.getDimensionSize(1)+k] -= streamMean[j];
					}	
				}
			}
			*/
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

	std::for_each(m_Streams.begin(), m_Streams.end(), [&returnValue](StreamBase* entry) { if(entry) { returnValue &= entry->rewind(); } } );

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

bool Track::getNextStreamIndex(int& earliestIndex) const
{
	if(m_Streams.size()==0)
	{
		return false;
	}

	// @todo: check selection here

	// Find the stream with the earliest chunk, return the stream
	uint64_t earliestTime = std::numeric_limits<uint64_t>::max();
	bool foundSomething = false;

	for(size_t i=0;i<m_Streams.size();i++)
	{
		StreamBase* ptr = m_Streams[i];

		const TypeBase::Buffer* nextChunk;
		if(ptr && ptr->peek(&nextChunk) && nextChunk->m_bufferStart < earliestTime)
		{
			earliestTime = nextChunk->m_bufferStart;
			earliestIndex = (int)i;
			foundSomething = true;
		}
		/*
		if(ptr && ptr->isEof())
		{
			uint64_t lastTime = ptr->getLastTime();
			earliestTime = std::min<uint64_t>(earliestTime, lastTime);
			foundSomething = true;
		}
		*/
	}

	if(!foundSomething)
	{
		std::cout << "All streams exhausted\n";
		return false;
	}
	
	return true;
}

bool Track::getNextStream(StreamBase** output)
{
	int idx;
	if(getNextStreamIndex(idx))
	{
		*output = m_Streams[idx];
		return true;
	}
	return false;

}
