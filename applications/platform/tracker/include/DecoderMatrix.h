
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "Source.h"
#include "DecoderBase.h"
#include "Stream.h"
#include "TypeMatrix.h"

class DecoderMatrix : public DecoderBase { 
public:
	DecoderMatrix(OpenViBE::Kernel::IKernelContext& ctx, Stream<TypeMatrix>* target) : DecoderBase(ctx), m_decoder(m_Box,0) {} ;

	virtual bool decode(const MemoryBufferWithType& chunk) {
		m_Box.dummy.m_InBuffer.setSize(0,true);
		m_Box.dummy.m_InBuffer.append(chunk.buffer);
		m_decoder.decode(0);

		if(m_decoder.isBufferReceived())
		{
			OpenViBE::IMatrix* decoded = m_decoder.getOutputMatrix();

			TypeMatrix::Buffer* tmp = new TypeMatrix::Buffer(); 
			OpenViBEToolkit::Tools::Matrix::copy(tmp->m_buffer, *decoded);
			tmp->m_bufferStart = chunk.bufferStart;
			tmp->m_bufferEnd = chunk.bufferEnd;

			m_Target->push(tmp);
		}
		return true;
	};

private:

	OpenViBEToolkit::TStreamedMatrixDecoder<BoxAlgorithmProxy> m_decoder;

	Stream<TypeMatrix>* m_Target = nullptr;

};

