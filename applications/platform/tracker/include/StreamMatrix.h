
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

#include "Stream.h"
#include "StreamMatrixChunk.h"

#include "BoxAlgorithmProxy.h"

class StreamMatrix : public Stream {
public:
	StreamMatrix(OpenViBE::Kernel::IKernelContext& ctx) : Stream(ctx), m_Box(ctx), decoder(m_Box, 0) {} ;

	virtual ~StreamMatrix(void) { };

	virtual OpenViBE::CIdentifier getTypeIdentifier(void) const override { return OVTK_TypeId_StreamedMatrix; };
	
	virtual bool peek(const StreamChunk **ptr) const { if(m_position<m_Chunks.size()) { *ptr = m_Chunks[m_position]; return true; } else { return false; } };
	virtual bool step(void) { if(m_position<=m_Chunks.size()) { m_position++; return true; } else { return false; } } ;
	virtual bool rewind(void) { m_position = 0; return true; };

	virtual bool getChunk(uint64_t idx, StreamChunk** ptr) { 
		if(idx<m_Chunks.size()) 
		{
			*ptr = m_Chunks[idx]; return true;
		}
		else
		{
			return false;
		}
	}

	virtual uint64_t getChunkStart(uint64_t idx) const { if(idx<m_Chunks.size()) return m_Chunks[idx]->m_bufferStart; else return (uint64_t)(-1);}

	virtual bool push(const MemoryBufferWithType& buf) { 
		m_Box.dummy.m_InBuffer.setSize(0,true);
		m_Box.dummy.m_InBuffer.append(buf.buffer);
		decoder.decode(0);

		if(decoder.isBufferReceived())
		{
			OpenViBE::IMatrix* decoded = decoder.getOutputMatrix();

			StreamMatrixChunk* tmp = new StreamMatrixChunk();
			OpenViBEToolkit::Tools::Matrix::copy(tmp->data, *decoded);
			tmp->m_bufferStart = buf.bufferStart;
			tmp->m_bufferEnd = buf.bufferEnd;

			m_Chunks.push_back(tmp);
		}
		return true;
	};

	std::vector<StreamMatrixChunk*> m_Chunks;

private:

	BoxAlgorithmProxy m_Box;
	OpenViBEToolkit::TStreamedMatrixDecoder<BoxAlgorithmProxy> decoder;

	size_t m_position = 0;

};

