//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include "Workspace.h"

int main(int argc, char *argv[])
{
	Workspace wp;

	wp.setSink("test-processor.xml");
	wp.setTrack("test-eeg.ov");
	
	// Push some chunks to selection
	Selection& selection = wp.m_track.m_Selection;
	selection.addRange(Range(3,5));
	selection.addRange(Range(9,11));

	wp.play();

	return 0;
}

