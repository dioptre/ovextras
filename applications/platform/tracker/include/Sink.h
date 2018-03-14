
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

	virtual bool initialize(const char *xmlFile);
	virtual bool uninitialize(void);

	virtual bool configureSink(void);

	virtual bool pull(StreamBase* stream);

	virtual bool play(bool playFast);
	virtual bool stop(void);

	virtual uint64_t getCurrentTime(void) const;

public:

	Socket::IConnectionServer* m_pConnectionServer = nullptr;

	std::thread* m_ClientHandlerThread = nullptr;
	std::thread* m_pPlayerThread = nullptr;

	CClientHandler* m_ClientHandler = nullptr;
	OutputEncoder* m_OutputEncoder = nullptr;
	
	uint32_t m_ConnectionPort = 1024;
	uint32_t m_ChunksSent = 0;
	uint64_t m_PreviousChunkEnd = 0;

	bool m_SignalHeaderSent = false;
	bool m_StimulationHeaderSent = false;
	bool m_PlayFast = false;

	uint64_t m_SamplingRate = 0;	// these will get resolved after we have a header
	uint64_t m_ChunkSize = 0;

	std::string m_xmlFilename;

protected:

	OpenViBE::Kernel::IKernelContext& m_KernelContext;

};

