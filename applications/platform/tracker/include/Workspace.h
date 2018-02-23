
#pragma once

#include <openvibe/ov_all.h>

#include "Track.h"
#include "Sink.h"

using namespace OpenViBE::Kernel;

class Workspace {
public:

	Workspace(IKernelContext& rContext) : m_KernelContext(rContext), m_output(rContext) { };
	~Workspace(void);

	// Set an EEG file to the workspace (@todo change to 'addTrack' for multiple files)
	bool setTrack(const char *filename);

	// Set output for chunks
	bool setSink(const char *scenarioXml, uint32_t samplingRate, uint32_t chunkSize);

	// Push the selected part of the tracks to sink
	bool play(void);

	// Save and load workspace
	bool save(const char *filename) { m_Filename = filename; return false; };
	bool load(const char *filename) { return false; };

	IKernelContext& m_KernelContext;

	Track m_track;
	Sink m_output;

	std::string m_Filename;

};
