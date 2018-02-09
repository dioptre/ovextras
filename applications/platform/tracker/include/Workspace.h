
#pragma once

#include "Track.h"
#include "Sink.h"

class Workspace {
public:

	Workspace(void) { };
	~Workspace(void);

	// Set an EEG file to the workspace (@todo change to 'addTrack' for multiple files)
	bool setTrack(const char *filename);

	// Set output for chunks
	bool setSink(const char *scenarioXml);

	// Push the selected part of the tracks to sink
	bool play(void);

	// Save and load workspace
	bool save(const char *filename) { m_Filename = filename; return false; };
	bool load(const char *filename) { return false; };

	Track m_track;
	Sink m_output;

	std::string m_Filename;

};
