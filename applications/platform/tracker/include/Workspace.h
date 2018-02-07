
#pragma once

#include <Track.h>

class Workspace {
public:

	Workspace(void) : m_track("TestiRaita") { };

	Track m_track;

	std::string m_Filename;

};
