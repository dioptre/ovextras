
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeBase.h"

class StreamBase {
public:
	StreamBase(void) : m_position(0) { } ;

	virtual ~StreamBase(void) { } ; 

	virtual OpenViBE::CIdentifier getTypeIdentifier(void) const = 0;

	virtual bool peek(const TypeBase::Buffer** ptr) const = 0;
	virtual bool peek(uint64_t timePoint, TypeBase::Buffer** ptr) const = 0;

	virtual bool step(void) { if(m_position <= getChunkCount() ) { m_position++; return true; } else { return false; } } ;
	virtual bool rewind(void) { m_position = 0; return true; };

// @note Implementing seek might be problematic until we can pass the chunk times to Designer (stim times need to belong inside chunk)
// 	virtual bool seek(uint64_t timePoint) = 0;

	virtual bool clear(void) = 0;

	virtual size_t getChunkCount(void) const = 0;
//	virtual bool getChunk(size_t idx, TypeBase::Buffer **ptr) const = 0;

	virtual bool initializeEBMLExport(void) =0;
	virtual bool getEBMLHeader(std::vector<uint8_t>& data) = 0;
	virtual bool getEBMLChunk(std::vector<uint8_t>& data, uint64_t& start, uint64_t& end) = 0;
	virtual bool getEBMLChunk(int idx, std::vector<uint8_t>& data, uint64_t& start, uint64_t& end) = 0;
	virtual bool getEBMLEnd(std::vector<uint8_t>& data, uint64_t& start, uint64_t& end) = 0;

protected:

	size_t m_position = 0;

};

