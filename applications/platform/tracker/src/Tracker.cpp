//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include <system/ovCTime.h>

#include "Tracker.h"

Tracker::Tracker(OpenViBE::Kernel::IKernelContext& ctx) : m_KernelContext(ctx), m_Workspace(ctx)
{
	
}

bool Tracker::play(bool playFast)
{
	switch(m_CurrentState)
	{
	case State_Stopped:
			m_Workspace.rewind();
			if(!m_Workspace.play(playFast))
			{
				std::cout << "Error: play failed\n";
				return false;
			}
			m_CurrentState = State_Playing;
			break;
	case State_Playing:
			m_CurrentState = State_Paused;
			break;
	case State_Paused:
			m_CurrentState = State_Playing;
			break;
	default:
			break;
	}

	return true;
}

bool Tracker::stop(void)
{
	m_CurrentState = State_Stopped; 

	return m_Workspace.stop();
}

bool Tracker::step(void)
{
	if(m_CurrentState==State_Playing)
	{
		if(!m_Workspace.step())
		{
			m_CurrentState = State_Stopped;
			return false;
		}
	}
	else
	{
		System::Time::sleep(1);
	}

	return true;
}

#if 0
void testCode(void)
{
	Workspace wp(*kernelWrapper.m_KernelContext);

// 	TestClass tmp(*kernelWrapper.m_KernelContext);

/*

	const CString eegFile = OpenViBE::Directories::getDataDir() + CString("/scenarios/signals/bci-motor-imagery.ov");
//	const CString eegFile = CString("E:/jl/noise-test.ov");
	const CString scenarioFile = OpenViBE::Directories::getDataDir() + CString("/applications/tracker/tracker-debug-display.xml");
	
	if(!wp.setTrack(eegFile.toASCIIString()))
	{
		return 2;
	}
	if(!wp.setSink(scenarioFile.toASCIIString()))
	{
		return 3;
	}

	// Push some chunks to selection
	Selection& selection = wp.m_track.m_Selection;
	selection.addRange(Range(3,5));
	selection.addRange(Range(9,11));

	if(!wp.play())
	{
		return 4;
	}
*/	
}
#endif

