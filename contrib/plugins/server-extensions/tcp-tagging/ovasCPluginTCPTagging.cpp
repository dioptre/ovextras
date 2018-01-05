
#include "ovasCPluginTCPTagging.h"

#include <system/ovCTime.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

// #define TCPTAGGING_DEBUG
#if defined(TCPTAGGING_DEBUG)
#include <iomanip>
#endif

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace OpenViBEAcquisitionServerPlugins;

CPluginTCPTagging::CPluginTCPTagging(const OpenViBE::Kernel::IKernelContext& rKernelContext)
	: IAcquisitionServerPlugin(rKernelContext, "AcquisitionServer_Plugin_TCPTagging"),
	  m_port(15361)
{
	m_rKernelContext.getLogManager() << Kernel::LogLevel_Info << "Loading plugin: TCP Tagging\n";
	m_oSettingsHelper.add("TCP_Tagging_Port", &m_port);
	m_oSettingsHelper.load();
}

CPluginTCPTagging::~CPluginTCPTagging()
{
}

void CPluginTCPTagging::startHook(const std::vector<OpenViBE::CString>& vSelectedChannelNames,
	OpenViBE::uint32 ui32SamplingFrequency, OpenViBE::uint32 ui32ChannelCount, OpenViBE::uint32 ui32SampleCountPerSentBlock)
{
	// initialize tag stream
	// this may throw exceptions, e.g. when the port is already in use.
	try {
		m_scopedTagStream.reset(new CTagStream(m_port));
	}
	catch (std::exception& e) {
		m_rKernelContext.getLogManager() << Kernel::LogLevel_Error << "Could not create tag stream: " << e.what();
	}

	// Initialize time counters.
	m_previousClockTime = System::Time::zgetTime();
	m_previousSampleTime = 0;
	m_bWarningPrinted = false;
}

void CPluginTCPTagging::stopHook()
{
    m_scopedTagStream.reset();
}

// n.b. With this version of tcp tagging, all the timestamps are in fixed point
void CPluginTCPTagging::loopHook(std::deque < std::vector < OpenViBE::float32 > >& /*vPendingBuffer*/,
	OpenViBE::CStimulationSet& stimulationSet, uint64 /* start */, uint64 /* end */, uint64 lastSampleTime)
{
	const uint64 clockTime = System::Time::zgetTime(); 

	Tag tag;

	// Collect tags from the stream until exhaustion.
	while(m_scopedTagStream.get() && m_scopedTagStream->pop(tag)) {
		m_rKernelContext.getLogManager() << Kernel::LogLevel_Trace << "New Tag received (" << tag.flags << ", " << tag.identifier << ", " 
			<< ITimeArithmetics::timeToSeconds(tag.timestamp) << "s) at " 
			<< ITimeArithmetics::timeToSeconds(clockTime) << "s\n";

		// Check that the timestamp fits the current chunk.
		if (tag.timestamp < m_previousClockTime) {
			m_rKernelContext.getLogManager() << Kernel::LogLevel_Trace << "The Tag has arrived before the beginning of the current chunk and will be inserted at the beginning of this chunk\n";
			tag.timestamp = m_previousClockTime;
		}

		const uint64 tagOffsetClock = tag.timestamp - m_previousClockTime;  // How far in time the marker is from the last call to this function

		const uint64 adjustedTagTime = m_previousSampleTime + tagOffsetClock;	// Time since the beginning of the current buffer (as approx by time of the last sample of the prev. run)

#if defined(TCPTAGGING_DEBUG)
		std::cout << "Set tag " << tag.identifier 
			<< " at " << std::setprecision(6) << ITimeArithmetics::timeToSeconds(adjustedTagTime)
			<< " (prevS = " << ITimeArithmetics::timeToSeconds(m_previousSampleTime) << "s, "
			<< " prevC = " << ITimeArithmetics::timeToSeconds(m_previousClockTime) << "s, "
			<< " tag = " << ITimeArithmetics::timeToSeconds(tag.timestamp) << "s, "
			<< " d = " << ITimeArithmetics::timeToSeconds(tagOffsetClock)*1000.0 << "ms)"
			<< "\n";
#endif

		// Insert tag into the stimulation set.
		stimulationSet.appendStimulation(tag.identifier, adjustedTagTime, 0 /* duration of tag (ms) */);
	}

	// Update time counters. Basically these counters allow to map the time a stamp was received to the time related to the sample buffers,
	// as we know this function is called right after receiving samples from a device.
	m_previousClockTime = clockTime;
	m_previousSampleTime = lastSampleTime;

}

