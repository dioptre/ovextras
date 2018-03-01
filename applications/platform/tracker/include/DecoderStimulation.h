
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "Source.h"
#include "DecoderBase.h"
#include "Stream.h"
#include "TypeStimulation.h"

class DecoderStimulation : public DecoderBase { 
public:
	DecoderStimulation(OpenViBE::Kernel::IKernelContext& ctx, Stream<TypeStimulation>* target) :  m_Target(target), DecoderBase(ctx), m_decoder(m_Box,0) {} ;

	virtual bool decode(const MemoryBufferWithType& chunk) {
		m_Box.dummy.m_InBuffer.setSize(0,true);
		m_Box.dummy.m_InBuffer.append(chunk.buffer);
		m_decoder.decode(0);

		if(m_decoder.isBufferReceived())
		{
			OpenViBE::IStimulationSet* decoded = m_decoder.getOutputStimulationSet();

			TypeStimulation::Buffer* tmp = new TypeStimulation::Buffer();
			for(size_t i=0;i<decoded->getStimulationCount();i++)
			{
				tmp->m_buffer.appendStimulation(decoded->getStimulationIdentifier(i),
					decoded->getStimulationDate(i),
					decoded->getStimulationDuration(i)
					);
			}
			tmp->m_bufferStart = chunk.bufferStart;
			tmp->m_bufferEnd = chunk.bufferEnd;

			m_Target->push(tmp);
		}
		return true;

	};

private:

	OpenViBEToolkit::TStimulationDecoder<BoxAlgorithmProxy> m_decoder;

	Stream<TypeStimulation>* m_Target = nullptr;

};

