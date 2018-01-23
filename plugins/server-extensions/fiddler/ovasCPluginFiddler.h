#ifndef __OpenViBE_AcquisitionServer_Fiddler_H__
#define __OpenViBE_AcquisitionServer_Fiddler_H__


/**
  * \brief Acquisition Server plugin that tinkers with the signal for P300 debugging purposes
  * \version 0.1
  * \author Jussi T. Lindgren / Inria
  */

#include "ovasIAcquisitionServerPlugin.h"

namespace OpenViBEAcquisitionServer
{
	class CAcquisitionServer;

	namespace OpenViBEAcquisitionServerPlugins
	{
		class CPluginFiddler : public IAcquisitionServerPlugin
		{
			// Plugin interface
			public:
				CPluginFiddler(const OpenViBE::Kernel::IKernelContext& rKernelContext);
				virtual ~CPluginFiddler();

				virtual void startHook(const std::vector<OpenViBE::CString>& vSelectedChannelNames, uint32_t ui32SamplingFrequency, uint32_t ui32ChannelCount, uint32_t ui32SampleCountPerSentBlock);
				virtual void stopHook();
				virtual void loopHook(std::deque < std::vector < OpenViBE::float32 > >& vPendingBuffer,
					OpenViBE::CStimulationSet& stimulationSet, uint64_t start, uint64_t end, uint64_t sampleTime);

				// Plugin implementation

				OpenViBE::float32 m_f32FiddlerStrength;
				uint64_t m_ui64StartSample;
				uint64_t m_ui64EndSample;

				uint32_t m_ui32SamplingFrequency;
				uint64_t m_ui64ProcessedSampleCount;
				uint64_t m_ui64LastPendingBufferSize;
				uint64_t m_ui64Counter;

			private:

				uint32_t m_ui32SampleCountPerSentBlock;
		};


	}
}

#endif // __OpenViBE_AcquisitionServer_Fiddler_H__

