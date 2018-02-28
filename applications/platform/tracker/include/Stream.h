
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

// using namespace OpenViBE;
// using namespace OpenViBE::Kernel;

#include "Source.h"

#include "StreamChunk.h"

class Stream {
public:
	Stream(OpenViBE::Kernel::IKernelContext& ctx) : m_KernelContext(ctx) {} ;

	virtual ~Stream(void) { } ; 

	virtual OpenViBE::CIdentifier getTypeIdentifier(void) const = 0;

	virtual bool push(const MemoryBufferWithType& chunk) = 0;

	virtual bool peek(const StreamChunk **ptr) const = 0;
	virtual bool step(void) = 0;

	virtual bool rewind(void) = 0;
	
	OpenViBE::Kernel::IKernelContext& m_KernelContext;

private:

};

