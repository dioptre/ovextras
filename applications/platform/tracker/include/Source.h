
#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <stack>
#include <map>
#include <vector>

#include <openvibe/ov_all.h>
#include <openvibe/ovCMatrix.h>

#include <socket/IConnectionServer.h>

#include <fs/Files.h>

#include <ebml/CReader.h>
#include <ebml/CReaderHelper.h>

class CClientHandler;
class OutputEncoder;

class MemoryBufferWithType
{
public:
	OpenViBE::CMemoryBuffer buffer;
	uint64_t streamType;
	uint64_t streamIndex;
	uint64_t bufferStart;
	uint64_t bufferEnd;
};

class Source : public EBML::IReaderCallback {
public:

	Source(void)
		: m_SamplingRate(0), m_ChunkSize(0), m_pFile(nullptr)
		,m_oReader(*this) {};

	bool initialize(const char *signalFile);
	bool uninitialize(void);

	bool pullChunk(MemoryBufferWithType& output);
	bool stop(void);

public:
	bool playingStarted;

	EBML::CReader m_oReader;
	EBML::CReaderHelper m_oReaderHelper;
	OpenViBE::CMemoryBuffer m_oSwap;
	OpenViBE::CMemoryBuffer m_oPendingChunk;

	uint32_t m_SamplingRate;
	uint32_t m_ChunkSize;
	uint32_t m_ChunksSent;

	OpenViBE::boolean m_bPending;
	OpenViBE::boolean m_bHasEBMLHeader;

	std::stack < EBML::CIdentifier > m_vNodes;
	std::map < OpenViBE::uint32, OpenViBE::uint32 > m_vStreamIndexToOutputIndex;
	std::map < OpenViBE::uint32, OpenViBE::CIdentifier > m_vStreamIndexToTypeIdentifier;

	std::vector<OpenViBE::uint64> m_ChunkStreamType;
	OpenViBE::uint32 m_ChunkStreamIndex;
	OpenViBE::uint64 m_ui64StartTime;
	OpenViBE::uint64 m_ui64EndTime;

private:

	virtual EBML::boolean isMasterChild(const EBML::CIdentifier& rIdentifier);
	virtual void openChild(const EBML::CIdentifier& rIdentifier);
	virtual void processChildData(const void* pBuffer, const EBML::uint64 ui64BufferSize);
	virtual void closeChild(void);

//	OpenViBE::Kernel::IKernelContext& m_KernelContext;

	FILE *m_pFile;
};

