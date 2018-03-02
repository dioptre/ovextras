//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include "Workspace.h"

class Tracker
{
public:
	Tracker(OpenViBE::Kernel::IKernelContext& ctx);

	enum TrackerState {
		State_Stopped = 0,
		State_Playing,
		State_Paused
	};

	bool play(bool playFast);
	bool stop(void);

	bool step(void);

	Workspace& getWorkspace(void) { return m_Workspace; } ;

protected:

	OpenViBE::Kernel::IKernelContext& m_KernelContext;
	Workspace m_Workspace;

	TrackerState m_CurrentState = State_Stopped;

};


