
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "Source.h"
#include "BoxAlgorithmProxy.h"
#include "TypeBase.h"

class DecoderBase { 
public:
	DecoderBase(OpenViBE::Kernel::IKernelContext& ctx) : m_KernelContext(ctx), m_Box(ctx) {} ;

	virtual bool decode(const MemoryBufferWithType& chunk) = 0;

protected:
	OpenViBE::Kernel::IKernelContext& m_KernelContext;

	BoxAlgorithmProxy m_Box;

};

