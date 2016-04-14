#include "ovasCPluginTCPTagging.h"

#include <sys/timeb.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace OpenViBEAcquisitionServerPlugins;

CPluginTCPTagging::CPluginTCPTagging(const OpenViBE::Kernel::IKernelContext& rKernelContext)
	: IAcquisitionServerPlugin(rKernelContext, "AcquisitionServer_Plugin_TCPTagging"),
	  m_port("15361")
{
	m_rKernelContext.getLogManager() << Kernel::LogLevel_Info << "Loading plugin: TCP Tagging\n";
	m_oSettingsHelper.add("TCP Tagging Port", &m_port);
}

CPluginTCPTagging::~CPluginTCPTagging()
{
}

void CPluginTCPTagging::startHook(const std::vector<OpenViBE::CString>& vSelectedChannelNames,
	OpenViBE::uint32 ui32SamplingFrequency, OpenViBE::uint32 ui32ChannelCount, OpenViBE::uint32 ui32SampleCountPerSentBlock)
{
	// initialize tag stream
	m_scopedTagStream.reset(new CTagStream());

	// Get POSIX time (number of milliseconds since epoch)
	timeb time_buffer;
	ftime(&time_buffer);
	uint64 posixTime = time_buffer.time*1000ULL + time_buffer.millitm;

	// Initialize time counters.
	m_previousPosixTime = posixTime;
	m_previousSampleTime = 0;

	// Clear Tag stream
	Tag tag;
	while(m_scopedTagStream->pop(tag));
}

void CPluginTCPTagging::stopHook()
{
    m_scopedTagStream.reset();
}

void CPluginTCPTagging::loopHook(std::vector < std::vector < OpenViBE::float32 > >& /*vPendingBuffer*/,
	OpenViBE::CStimulationSet& stimulationSet, uint64 start, uint64 end, uint64 sampleTime)
{
	// Get POSIX time (number of milliseconds since epoch)
	timeb time_buffer;
	ftime(&time_buffer);
	uint64 posixTime = time_buffer.time*1000ULL + time_buffer.millitm;

	Tag tag;

	// Collect tags from the stream until exhaustion.
	while(m_scopedTagStream->pop(tag)) {
		m_rKernelContext.getLogManager() << Kernel::LogLevel_Info << "New Tag received (" << tag.padding << ", " << tag.identifier << ", " << tag.timestamp << ") at " << posixTime << " (posix time in ms)\n";

		// Check that the timestamp fits the current chunk.
		if (tag.timestamp < m_previousPosixTime) {
			m_rKernelContext.getLogManager() << Kernel::LogLevel_Warning << "The Tag has arrived before the beginning of the current chunk and will be inserted at the beginning of this chunk\n";
			tag.timestamp = m_previousPosixTime;
		}

		// Marker time correction (simple local linear interpolation).
		if (m_previousPosixTime != posixTime) {
			tag.timestamp = m_previousSampleTime + (tag.timestamp - m_previousPosixTime)*((sampleTime - m_previousSampleTime) / (posixTime - m_previousPosixTime));
		}

		// Insert tag into the stimulation set.
		stimulationSet.appendStimulation(tag.identifier, tag.timestamp, 0 /* duration of tag (ms) */);
	}

	// Update time counters.
	m_previousPosixTime = posixTime;
	m_previousSampleTime = sampleTime;
}
