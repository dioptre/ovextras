
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

	virtual bool step(void) { if(m_position <= getChunkCount() ) { m_position++; return true; } else { return false; } } ;
	virtual bool rewind(void) { m_position = 0; return true; };
	virtual bool seek(uint64_t timePoint) { return false; };
	virtual bool clear(void) = 0;

	virtual uint64_t getChunkCount(void) const = 0;

protected:

	size_t m_position = 0;

};

