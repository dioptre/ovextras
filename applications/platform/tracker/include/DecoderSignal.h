
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "Source.h"
#include "DecoderMatrix.h"
#include "Stream.h"
#include "TypeSignal.h"

class DecoderSignal : public DecoderMatrix { 
public:
	DecoderSignal(OpenViBE::Kernel::IKernelContext& ctx, Stream<TypeSignal>* target) : m_Target(target), DecoderMatrix(ctx, nullptr), m_decoder(m_Box,0) {} ;

	virtual bool decode(const MemoryBufferWithType& chunk) {
		m_Box.dummy.m_InBuffer.setSize(0,true);
		m_Box.dummy.m_InBuffer.append(chunk.buffer);
		m_decoder.decode(0);

		if(m_decoder.isHeaderReceived())
		{
			TypeSignal::Header* header = new TypeSignal::Header();
			header->m_samplingFrequency = m_decoder.getOutputSamplingRate();

			OpenViBE::IMatrix* decoded = m_decoder.getOutputMatrix();
			OpenViBEToolkit::Tools::Matrix::copy(header->m_header, *decoded);

//			std::cout << "H " << decoded->getBufferElementCount() << " " << header->m_header.getBufferElementCount()<< "\n";

			m_Target->setHeader(header);
		}

		if(m_decoder.isBufferReceived())
		{
			OpenViBE::IMatrix* decoded = m_decoder.getOutputMatrix();

			TypeSignal::Buffer* tmp = new TypeSignal::Buffer(); 
			OpenViBEToolkit::Tools::Matrix::copy(tmp->m_buffer, *decoded);
			tmp->m_bufferStart = chunk.bufferStart;
			tmp->m_bufferEnd = chunk.bufferEnd;

// 			std::cout << "B " << decoded->getBufferElementCount() << " " << tmp->m_buffer.getBufferElementCount()<< "\n";

			m_Target->push(tmp);
		}

		return true;
	};

private:

	OpenViBEToolkit::TSignalDecoder<BoxAlgorithmProxy> m_decoder;

	Stream<TypeSignal>* m_Target = nullptr;

};

