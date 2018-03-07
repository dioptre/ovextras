
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "StreamBase.h"

#include <iostream>
#include <algorithm>

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

protected:

	typename T::Header* m_Header = nullptr;

 	std::vector<typename T::Buffer*> m_Chunks;

// 	typename T::End m_End;


};

