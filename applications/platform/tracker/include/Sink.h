
#pragma once

#include <openvibe/ov_all.h>
#include <openvibe/ovCMatrix.h>

#include <socket/IConnectionServer.h>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "StreamBase.h"

class CClientHandler;
class OutputEncoder;

class Sink {
public:

	Sink(OpenViBE::Kernel::IKernelContext& KernelContext) : m_KernelContext(KernelContext) {};

	bool initialize(const char *xmlFile, uint32_t samplingRate);
	bool uninitialize(void);
	bool pull(StreamBase* stream);
	bool stop(void);

public:

	Socket::IConnectionServer* m_pConnectionServer = nullptr;

	std::thread* m_ClientHandlerThread = nullptr;
	std::thread* m_pPlayerThread = nullptr;

	CClientHandler* m_ClientHandler = nullptr;
	OutputEncoder* m_OutputEncoder = nullptr;

	uint32_t m_SamplingRate = 0;
	uint32_t m_ChunkSize = 0;

	uint32_t m_ChunksSent = 0;

	uint64_t m_PreviousChunkEnd = 0;

	bool m_SignalHeaderSent = false;
	bool m_StimulationHeaderSent = false;
	bool m_PlayFast = false;

private:

	OpenViBE::Kernel::IKernelContext& m_KernelContext;

};

