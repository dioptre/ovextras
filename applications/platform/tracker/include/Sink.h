
#pragma once

#include <openvibe/ov_all.h>
#include <openvibe/ovCMatrix.h>

#include <socket/IConnectionServer.h>

#include <thread>
#include <mutex>
#include <condition_variable>

class CClientHandler;
class OutputEncoder;

class Sink {
public:

	Sink(OpenViBE::Kernel::IKernelContext& KernelContext) : m_KernelContext(KernelContext), m_SamplingRate(0), m_ChunkSize(0) {};

	bool initialize(const char *xmlFile, uint32_t samplingRate, uint32_t chunkSize);
	bool uninitialize(void);
	bool pushChunk(const OpenViBE::CMatrix& chunk);
	bool stop(void);

public:
	bool playingStarted;

	Socket::IConnectionServer* m_pConnectionServer;

	std::thread* m_ClientHandlerThread;
	std::thread* m_pPlayerThread;

	CClientHandler* m_ClientHandler;
	OutputEncoder* m_OutputEncoder;

	uint32_t m_SamplingRate;
	uint32_t m_ChunkSize;

	uint32_t m_ChunksSent;

private:

	OpenViBE::Kernel::IKernelContext& m_KernelContext;

};

