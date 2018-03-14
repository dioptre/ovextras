
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include <ebml/IWriterHelper.h>
#include <ebml/IWriter.h>
#include <ebml/TWriterCallbackProxy.h>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>



#include "StreamBase.h"

class EBMLWriterCallback {
public:
	void write(const void* buffer, const uint64_t bufferSize)
	{
		const uint8_t* data = static_cast<const uint8_t*>(buffer);
		m_Buffer.insert(m_Buffer.end(), data, data + bufferSize);
	}

	void clear()
	{
		m_Buffer.clear();
	}

	const std::vector<uint8_t>& data()
	{
		return m_Buffer;
	}

private:
	std::vector<uint8_t> m_Buffer;
};

template<class T> class Stream : public StreamBase {
public:
	Stream(void) : StreamBase() { };
	// (OpenViBE::Kernel::IKernelContext& ctx) : Stream(ctx) {} ;

	virtual ~Stream(void) { if(m_Header) { delete m_Header; } clear(); };

	virtual OpenViBE::CIdentifier getTypeIdentifier(void) const override { return T::getTypeIdentifier(); };

	virtual const typename T::Header& getHeader(void) const { return *m_Header; };
	virtual bool setHeader(typename T::Header* header) { m_Header = header; return true; }

	virtual bool push(typename T::Buffer* chunk) { m_Chunks.push_back(chunk); return true; };

	virtual bool peek(const typename T::Buffer** ptr) const { return Stream::peek(reinterpret_cast<const TypeBase::Buffer**>(ptr)); };
	virtual bool peek(const TypeBase::Buffer** ptr) const override { 
		// std::cout << "pos " << StreamBase::m_position << "\n"; 
		if(StreamBase::m_position<m_Chunks.size()) { *ptr = m_Chunks[StreamBase::m_position]; return true; } else { return false; } 
	};
	
	virtual bool peek(uint64_t timePoint, typename T::Buffer** ptr) { return Stream::peek(timePoint, reinterpret_cast<TypeBase::Buffer**>(ptr)); 
	}

	virtual bool peek(uint64_t timePoint, TypeBase::Buffer** ptr) const override { 
		// @fixme inefficient; could improve with binary search if needed
		auto it = std::find_if(m_Chunks.begin(), m_Chunks.end(), 
			[timePoint](const typename T::Buffer* b) { return (timePoint >= b->m_bufferStart && timePoint < b->m_bufferEnd); });
		if(it!=m_Chunks.end())
		{
			*ptr = *it; return true;
		}
		return false;
	}

	virtual size_t getChunkCount(void) const override { return m_Chunks.size(); };
	virtual bool getChunk(size_t idx, typename T::Buffer** ptr) const { 
		if(idx<m_Chunks.size()) 
		{ *ptr = m_Chunks[idx]; return true; } 
		else 
		{ *ptr = nullptr; return false; } };

	virtual bool clear(void) { 
		std::for_each(m_Chunks.begin(),m_Chunks.end(), [](typename T::Buffer* ptr) { delete ptr; } ); 
		m_Chunks.clear(); StreamBase::m_position = 0; return true; 
	};
	
	// Iterators and operators
	typename std::vector<typename T::Buffer*>::iterator begin() { return m_Chunks.begin(); };
	typename std::vector<typename T::Buffer*>::iterator end() { return m_Chunks.end(); };
	typename T::Buffer& operator[] (int idx) 
	{
          return m_Chunks[idx];  
	}

	// EBML serialization
	bool initializeEBMLExport(void) override
	{
		m_writerCallbackProxy = new EBML::TWriterCallbackProxy1<EBMLWriterCallback>(m_writerCallback, &EBMLWriterCallback::write);
		m_writer = EBML::createWriter(*m_writerCallbackProxy);
		m_writerHelper = EBML::createWriterHelper();
		m_writerHelper->connect(m_writer);	
		return true;
	};

	bool getEBMLHeader(std::vector<uint8_t>& data) override
	{
		m_writerCallback.clear();
		m_writerHelper->openChild(OVTK_NodeId_Header);
		{
			m_writerHelper->openChild(OVTK_NodeId_Header_StreamType);
			{
				m_writerHelper->setUIntegerAsChildData(0);
				m_writerHelper->closeChild();
			}
			m_writerHelper->openChild(OVTK_NodeId_Header_StreamVersion);
			{
				m_writerHelper->setUIntegerAsChildData(0);
				m_writerHelper->closeChild();
			}	
			if(m_Header)
			{
				m_Header->getEBML(*m_writerHelper);
			}
			m_writerHelper->closeChild();
		}
		data = m_writerCallback.data();
		return true;
	}

	bool getEBMLChunk(int idx, std::vector<uint8_t>& data, uint64_t& start, uint64_t& end) override
	{
		if(idx<m_Chunks.size())
		{
			m_writerCallback.clear();
			m_Chunks[idx]->getEBML(*m_writerHelper);
			data = m_writerCallback.data();

			start = m_Chunks[idx]->m_bufferStart;
			end = m_Chunks[idx]->m_bufferEnd;
			return true;
		} 
		else if(idx==m_Chunks.size())
		{
			return getEBMLEnd(data, start, end);
		}
		else
		{
			return false;
		}
	}

	bool getEBMLChunk(std::vector<uint8_t>& data, uint64_t& start, uint64_t& end) override
	{
		return getEBMLChunk(m_position, data, start, end);
	}

	bool getEBMLEnd(std::vector<uint8_t>& data, uint64_t& start, uint64_t& end) override
	{
		start  = (m_Chunks.size()>0 ? m_Chunks[m_Chunks.size()-1]->m_bufferEnd : 0);
		end = start;

		m_writerCallback.clear();
		m_writerHelper->openChild(OVTK_NodeId_End);
		{
			m_writerHelper->closeChild();
		}
		data = m_writerCallback.data();

		return true;
	}

protected:

	typename T::Header* m_Header = nullptr;

 	std::vector<typename T::Buffer*> m_Chunks;
	// ChunkVector m_Chunks;

	// EBML
	EBMLWriterCallback m_writerCallback;
	EBML::TWriterCallbackProxy1<EBMLWriterCallback>* m_writerCallbackProxy = nullptr;
	EBML::IWriter* m_writer = nullptr;            
	EBML::IWriterHelper* m_writerHelper = nullptr;   

// 	typename T::End m_End;


};

