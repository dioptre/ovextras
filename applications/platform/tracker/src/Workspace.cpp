//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include "Workspace.h"
#include <iostream>

Workspace::Workspace(OpenViBE::Kernel::IKernelContext& rContext)
	: m_KernelContext(rContext), m_sink(rContext), m_track(rContext) 
{ 
	/*
	if(m_KernelContext.getConfigurationManager().lookUpConfigurationTokenIdentifier("Tracker_Workspace_File") == OV_UndefinedIdentifier)
	{
		m_KernelContext.getConfigurationManager().addOrReplaceConfigurationToken("Tracker_Workspace_File", filename);		
	}
	*/

	// Restore the previous configuration
	load(nullptr);
};

Workspace::~Workspace(void)
{
	// Save current configuration
	save(nullptr);

	m_sink.uninitialize();
	m_track.uninitialize();
}

bool Workspace::step(void)
{
	// Get the stream which has the next pending chunk
	StreamBase* nextStream;

	bool nextStreamAvailable= m_track.getNextStream(&nextStream);
	if(!nextStreamAvailable)
	{
		std::cout << "Streams exhausted\n";
		m_sink.stop();
		return false;
	}

	return m_sink.pull(nextStream);	

}

bool Workspace::play(bool playFast) 
{
	return m_sink.play(playFast); 
}

bool Workspace::stop(void) 
{
	return m_sink.stop();
}

bool Workspace::setTrack(const char *filename) 
{ 
	m_KernelContext.getConfigurationManager().addOrReplaceConfigurationToken("Tracker_Workspace_File", filename);

	m_Trackfile = filename;

	return m_track.initialize(filename); 
};

bool Workspace::setSink(const char *scenarioXml) 
{ 
	m_KernelContext.getConfigurationManager().addOrReplaceConfigurationToken("Tracker_Workspace_Sink", scenarioXml);

	const OpenViBE::CString defaultFile = m_KernelContext.getConfigurationManager().expand("${Path_UserData}/openvibe-tracker.conf");
	if(!scenarioXml)
	{
		scenarioXml = defaultFile.toASCIIString();
	}

	m_Sinkfile = scenarioXml;

	return m_sink.initialize(scenarioXml);
};

// @fixme for multiple workspaces this solution needs to be reworked
bool Workspace::save(const char *filename)
{
	const OpenViBE::CString defaultFile = m_KernelContext.getConfigurationManager().expand("${Path_UserData}/openvibe-tracker.conf");
	if(!filename)
	{
		filename = defaultFile.toASCIIString();
	}

	FILE* l_pFile=::fopen(filename, "wt");
	if(l_pFile)
	{
		::fprintf(l_pFile, "# This file is generated\n");
		::fprintf(l_pFile, "# Do not modify\n");
		::fprintf(l_pFile, "\n");
		::fprintf(l_pFile, "# Last settings used in the Tracker\n");
		::fprintf(l_pFile, "Tracker_Workspace_File = %s\n", m_Trackfile.c_str() );
		::fprintf(l_pFile, "Tracker_Workspace_Sink = %s\n", m_Sinkfile.c_str() );
	
	} 
	else
	{
		return false;
	}

	fclose(l_pFile);

	return true;
}
	
bool Workspace::load(const char *filename)
{
	const OpenViBE::CString defaultFile = m_KernelContext.getConfigurationManager().expand("${Path_UserData}/openvibe-tracker.conf");
	if(!filename)
	{
		filename = defaultFile.toASCIIString();
	}

	if(!m_KernelContext.getConfigurationManager().addConfigurationFromFile(filename))
	{
		return false;
	}

	if(m_KernelContext.getConfigurationManager().lookUpConfigurationTokenIdentifier("Tracker_Workspace_File") != OV_UndefinedIdentifier)
	{
		setTrack(m_KernelContext.getConfigurationManager().lookUpConfigurationTokenValue("Tracker_Workspace_File"));
	}
	if(m_KernelContext.getConfigurationManager().lookUpConfigurationTokenIdentifier("Tracker_Workspace_Sink") != OV_UndefinedIdentifier)
	{
		setSink(m_KernelContext.getConfigurationManager().lookUpConfigurationTokenValue("Tracker_Workspace_Sink"));
	}

	return true;
}