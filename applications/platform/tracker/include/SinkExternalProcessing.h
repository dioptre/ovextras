
#pragma once

#include "Sink.h"
#include "Track.h"

#include <communication/ovCMessagingClient.h>

class SinkExternalProcessing : Sink {
public:

	SinkExternalProcessing(OpenViBE::Kernel::IKernelContext& KernelContext) : Sink(KernelContext) {};

	virtual bool initialize(const char *xmlFile) override;
	virtual bool uninitialize(void) override;

	virtual bool configureSink(void) override;

	virtual bool pull(StreamBase* source) override;
	virtual bool pull(Track& source);

	virtual bool play(bool playFast) override;
	virtual bool stop(void) override;

	virtual uint64_t getCurrentTime(void) const override;

protected:

	Communication::MessagingClient m_client;

	bool m_headerSent = false;

	uint64_t m_startTime = 0;

};

