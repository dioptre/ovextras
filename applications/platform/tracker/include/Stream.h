
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "StreamBase.h"

#include <iostream>

template<class T> class Stream : public StreamBase {
public:
	Stream(void) : StreamBase() { };
	// (OpenViBE::Kernel::IKernelContext& ctx) : Stream(ctx) {} ;

	virtual ~Stream(void) { };

	virtual OpenViBE::CIdentifier getTypeIdentifier(void) const override { return T::getTypeIdentifier(); };

	virtual const typename T::Header& getHeader(void) const { return m_Header; };
	virtual bool setHeader(typename T::Header& header) { m_Header = header; return true; }

	virtual bool peek(const typename T::Buffer** ptr) const { if(StreamBase::m_position<m_Chunks.size()){ *ptr = m_Chunks[StreamBase::m_position]; return true; } else { return false; } };
	virtual bool push(const typename T::Buffer* chunk) { m_Chunks.push_back(chunk); return true; };

	virtual bool peek(const TypeBase::Buffer** ptr) const override { 
		// std::cout << "pos " << StreamBase::m_position << "\n"; 
		if(StreamBase::m_position<m_Chunks.size()) { *ptr = m_Chunks[StreamBase::m_position]; return true; } else { return false; } 
	};

	virtual uint64_t getChunkCount(void) const override { return m_Chunks.size(); };

	virtual bool clear(void) { m_Chunks.clear(); return true; };

protected:

	typename T::Header m_Header;

 	std::vector<const typename T::Buffer*> m_Chunks;

// 	typename T::End m_End;


};

