//
// This proxy is needed in order to use the stream codecs from the toolkit
//

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

#include "Marker.h"

// using namespace OpenViBE;
// using namespace OpenViBE::Kernel;

namespace OpenViBE {
namespace Kernel {

class OV_API_Export IBoxIOProxy : public OpenViBE::Kernel::IBoxIO
{
public:
	virtual OpenViBE::uint32 getInputChunkCount(
		const OpenViBE::uint32 ui32InputIndex) const override  { return 0; }

	virtual OpenViBE::boolean getInputChunk(
		const OpenViBE::uint32 ui32InputIndex,
		const OpenViBE::uint32 ui32ChunkIndex,
		OpenViBE::uint64& rStartTime,
		OpenViBE::uint64& rEndTime,
		OpenViBE::uint64& rChunkSize,
		const OpenViBE::uint8*& rpChunkBuffer) const override { return true; }

	virtual const OpenViBE::IMemoryBuffer* getInputChunk(
		const OpenViBE::uint32 ui32InputIndex,
		const OpenViBE::uint32 ui32ChunkIndex)  const override { return &m_InBuffer; };

	virtual OpenViBE::uint64 getInputChunkStartTime(
		const OpenViBE::uint32 ui32InputIndex,
		const OpenViBE::uint32 ui32ChunkIndex)  const override  { return 0; };

	virtual OpenViBE::uint64 getInputChunkEndTime(
		const OpenViBE::uint32 ui32InputIndex,
		const OpenViBE::uint32 ui32ChunkIndex)  const override  { return 0; };

	virtual OpenViBE::boolean markInputAsDeprecated(
		const OpenViBE::uint32 ui32InputIndex,
		const OpenViBE::uint32 ui32ChunkIndex)  override  { return true; };

	virtual OpenViBE::uint64 getOutputChunkSize(
		const OpenViBE::uint32 ui32OutputIndex)  const override  { return 0; };

	virtual OpenViBE::boolean setOutputChunkSize(
		const OpenViBE::uint32 ui32OutputIndex,
		const OpenViBE::uint64 ui64Size,
		const OpenViBE::boolean bDiscard=true)  override  { return true; };

	virtual OpenViBE::uint8* getOutputChunkBuffer(
		const OpenViBE::uint32 ui32OutputIndex)  override { return nullptr; };

	virtual OpenViBE::boolean appendOutputChunkData(
		const OpenViBE::uint32 ui32OutputIndex,
		const OpenViBE::uint8* pBuffer,
		const OpenViBE::uint64 ui64BufferSize) override { return true; };

	virtual OpenViBE::IMemoryBuffer* getOutputChunk(
		const OpenViBE::uint32 ui32OutputIndex) override { return &m_OutBuffer; };

	virtual OpenViBE::boolean markOutputAsReadyToSend(
		const OpenViBE::uint32 ui32OutputIndex,
		const OpenViBE::uint64 ui64StartTime,
		const OpenViBE::uint64 ui64EndTime) override { return true; }

	virtual OpenViBE::CIdentifier getClassIdentifier(void) const override { return 0; };

	OpenViBE::CMemoryBuffer m_InBuffer;
	OpenViBE::CMemoryBuffer m_OutBuffer;

};
}
}

class BoxAlgorithmProxy {
public:
	BoxAlgorithmProxy(OpenViBE::Kernel::IKernelContext& ctx) : m_KernelContext(ctx) { };

	OpenViBE::Kernel::IBoxIO& getDynamicBoxContext(void) { return dummy; };
    OpenViBE::boolean markAlgorithmAsReadyToProcess(void) { return true;}
	
	OpenViBE::Kernel::IAlgorithmManager& getAlgorithmManager(void) const { return m_KernelContext.getAlgorithmManager(); };

	OpenViBE::Kernel::IBoxIOProxy dummy;
	
	OpenViBE::Kernel::IKernelContext& m_KernelContext;
};

