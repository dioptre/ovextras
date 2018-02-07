//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include "Workspace.h"

int main(int argc, char *argv[])
{
	Workspace wp;

	OpenViBE::CMatrix empty;
	
	wp.m_track.ImportTrack("testiraita.ov");

	wp.m_track.m_Dataset.m_vEEG.push_back(empty);
	wp.m_track.m_Dataset.m_samplingRate = 512;
	
	// Push some chunks to selection
	Selection& selection = wp.m_track.m_Selection;
	selection.addRange(Range(3,5));
	selection.addRange(Range(9,11));

	// Get blocks
	bool go = true;uint64_t cnt = 0;
	while(go)
	{
		OpenViBE::CMatrix chunk;

		go = wp.m_track.getNextChunk(cnt, chunk);

		// Push block to openvibe

		cnt++;
	}

	return 0;
}

