
// @todo put stuff to namespace OpenViBE::Tracker::

#pragma once

#include "Tracker.h"
#include "Workspace.h"

// Forward declare
struct _GtkBuilder;
class TrackRenderer;

class GUI {

public:

	GUI(int argc, char* argv[], Tracker& app) : m_rTracker(app)
	{
		initGUI(argc, argv);
	}
	~GUI();

	bool run();
	
	bool step();

protected: 

	bool initGUI(int argc, char* argv[]);
	bool openFileCB(void);
	bool openSinkCB(void);
	bool quitCB(void);
	bool stopCB(void);
	bool playCB(void);
	bool playFastCB(void);
	bool sinkPropertiesCB(void);

	bool redrawTrack(void);

	Tracker& m_rTracker;

	TrackRenderer* m_Renderer = nullptr;

	struct _GtkBuilder* m_pInterface = nullptr;

	uint64_t m_PreviousTime = 0;
};

