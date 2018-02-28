
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

// #include "Codecs.h"

// using namespace OpenViBE;
// using namespace OpenViBE::Kernel;

#include "StreamMatrix.h"

#include "BoxAlgorithmProxy.h"

class StreamSignal : public StreamMatrix {
public:
	StreamSignal(OpenViBE::Kernel::IKernelContext& ctx) : StreamMatrix(ctx), m_Box(ctx), decoder(m_Box, 0) {} ;

	virtual ~StreamSignal(void) { };

	virtual OpenViBE::CIdentifier getTypeIdentifier(void) const override { return OVTK_TypeId_Signal; };
	
	virtual bool push(const MemoryBufferWithType& buf) { 
		m_Box.dummy.m_InBuffer.setSize(0,true);
		m_Box.dummy.m_InBuffer.append(buf.buffer);
		decoder.decode(0);

		if(decoder.isHeaderReceived())
		{
			m_SamplingRate = decoder.getOutputSamplingRate();
		}

		return StreamMatrix::push(buf);
	};

	virtual uint32_t getSamplingRate(void) const { return m_SamplingRate; };

private:

	uint32_t m_SamplingRate;

	BoxAlgorithmProxy m_Box;
	OpenViBEToolkit::TSignalDecoder<BoxAlgorithmProxy> decoder;

};

