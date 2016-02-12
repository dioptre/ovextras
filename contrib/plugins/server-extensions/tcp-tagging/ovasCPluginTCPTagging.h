#ifndef __OpenViBE_AcquisitionServer_TCPTagging_H__
#define __OpenViBE_AcquisitionServer_TCPTagging_H__

/**
  * \brief Acquisition Server plugin adding the capability to receive stimulations from external sources
  * via TCP.
  */

#include "ovasIAcquisitionServerPlugin.h"

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

				// Overrides virtual method loopHook inherited from class IAcquisitionServerPlugin.
				void loopHook(std::vector < std::vector < OpenViBE::float32 > >& vPendingBuffer, 
					OpenViBE::CStimulationSet& stimulationSet, OpenViBE::uint64 start, OpenViBE::uint64 end, OpenViBE::uint64 sampleTime);

			private:
				OpenViBE::uint64 m_previousPosixTime;
				OpenViBE::uint64 m_previousSampleTime;
		};


	}
}

#endif // __OpenViBE_AcquisitionServer_TCPTagging_H__
