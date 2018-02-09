//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include "Workspace.h"
#include <iostream>

Workspace::~Workspace(void)
{
	m_output.uninitialize();
	m_track.uninitialize();
}

bool Workspace::play(void)
{
	// Get blocks
	bool go = true;uint64_t cnt = 0;
	while(go)
	{
		const OpenViBE::CMatrix* chunk;

		go = m_track.getNextChunk(cnt, &chunk);

		if(go)
		{
			go = m_output.pushChunk(*chunk);
		}

		cnt++;
	}

	return 0;
}

bool Workspace::setTrack(const char *filename) 
{ 
	return m_track.initialize(filename); 
};

bool Workspace::setSink(const char *scenarioXml) 
{ 
	return m_output.initialize(scenarioXml); 

};
