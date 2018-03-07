
#pragma once

#include "Track.h"

#include "Sink.h"

#include <openvibe/ov_all.h>

class Workspace {
public:

	Workspace(OpenViBE::Kernel::IKernelContext& rContext);
	~Workspace(void);

	// Set an EEG file to the workspace (@todo change to 'addTrack' for multiple files)
	bool setTrack(const char *filename);
	const char *getTrackFile(void) { return m_Trackfile.c_str();}

	const Track& getTrack(void) const { return m_track; }

	// Set output for chunks
	bool setSink(const char *scenarioXml);
	const char *getSinkFile(void) { return m_Sinkfile.c_str();}
	bool configureSink(void) { return m_sink.configureSink(); }

	uint64_t getCurrentTime(void) const { return m_sink.getCurrentTime(); }

	// Push the selected part of the tracks to sink
	bool step(void);

	bool play(bool playFast);
	bool stop(void);

	bool rewind(void) { return m_track.rewind(); }

	// Save and load workspace
	bool save(const char *filename);
	bool load(const char *filename);

protected:

	OpenViBE::Kernel::IKernelContext& m_KernelContext;

	Track m_track;
	Sink m_sink;

	std::string m_Trackfile;
	std::string m_Sinkfile;

};
