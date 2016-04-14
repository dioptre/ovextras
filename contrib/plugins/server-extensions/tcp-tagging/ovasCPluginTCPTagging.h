#ifndef __OpenViBE_AcquisitionServer_TCPTagging_H__
#define __OpenViBE_AcquisitionServer_TCPTagging_H__

/**
  * \brief Acquisition Server plugin adding the capability to receive stimulations from external sources
  * via TCP/IP.
  * 
  * The stimulation format is the same as with Shared Memory Tagging. It comprises three blocks of 8 bytes:
  *
  * ----------------------------------------------------------------------
  * |  padding  (8 bytes) |  event id (8 bytes)  |  timestamp (8 bytes)  |
  * ----------------------------------------------------------------------
  *  
  * The padding is only for consistency with Shared Memory Tagging and has no utility.
  * The event id informs about the type of event happening.
  * The timestamp is the posix time (ms since Epoch) at the moment of the event.
  * It the latter is set to 0, the acquisition server issues its own timestamp upon reception of the stimulation.
  *
  * Have a look at contrib/plugins/server-extensions/tcp-tagging/client-example to learn about the protocol
  * to send stimulations from the client. 
  */

#include "ovasIAcquisitionServerPlugin.h"
#include "ovasCTagStream.h"

#include <boost/scoped_ptr.hpp>

namespace OpenViBEAcquisitionServer
{
	namespace OpenViBEAcquisitionServerPlugins
	{
		class CPluginTCPTagging : public IAcquisitionServerPlugin
		{
			public:
				CPluginTCPTagging(const OpenViBE::Kernel::IKernelContext& rKernelContext);
				~CPluginTCPTagging();

				// Overrides virtual method startHook inherited from class IAcquisitionServerPlugin.
				void startHook(const std::vector<OpenViBE::CString>& vSelectedChannelNames, OpenViBE::uint32 ui32SamplingFrequency,
					OpenViBE::uint32 ui32ChannelCount, OpenViBE::uint32 ui32SampleCountPerSentBlock);

				// Overrides virtual method stopHook inherited from class IAcquisitionServerPlugin
				void stopHook();

				// Overrides virtual method loopHook inherited from class IAcquisitionServerPlugin.
				void loopHook(std::vector < std::vector < OpenViBE::float32 > >& vPendingBuffer, 
					OpenViBE::CStimulationSet& stimulationSet, OpenViBE::uint64 start, OpenViBE::uint64 end, OpenViBE::uint64 sampleTime);

			private:
				OpenViBE::uint64 m_previousPosixTime;
				OpenViBE::uint64 m_previousSampleTime;
				boost::scoped_ptr<CTagStream> m_scopedTagStream;
				OpenViBE::CString m_port;
		};


	}
}

#endif // __OpenViBE_AcquisitionServer_TCPTagging_H__
