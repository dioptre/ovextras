#ifndef __OpenViBE_AcquisitionServer_LSLOutput_H__
#define __OpenViBE_AcquisitionServer_LSLOutput_H__

#if defined TARGET_HAS_ThirdPartyLSL


/**
  * \brief Acquisition Server plugin outputting signals and stimulations to LabStreamingLayer (LSL) streams
  * \version 0.1
  * \author Jussi T. Lindgren / Inria
  */

#include <lsl_cpp.h>

#include "ovasIAcquisitionServerPlugin.h"

namespace OpenViBEAcquisitionServer
{
	class CAcquisitionServer;

	namespace OpenViBEAcquisitionServerPlugins
	{
		class CPluginLSLOutput : public IAcquisitionServerPlugin
		{
			// Plugin interface
			public:
				CPluginLSLOutput(const OpenViBE::Kernel::IKernelContext& rKernelContext);
				virtual ~CPluginLSLOutput();

				virtual void startHook(const std::vector<OpenViBE::CString>& vSelectedChannelNames, uint32_t ui32SamplingFrequency, uint32_t ui32ChannelCount, uint32_t ui32SampleCountPerSentBlock);
				virtual void stopHook();
				virtual void loopHook(std::vector < std::vector < OpenViBE::float32 > >& vPendingBuffer,
					OpenViBE::CStimulationSet& stimulationSet, uint64_t start, uint64_t end, uint64_t sampleTime);

				// Plugin implementation

				bool m_bIsLSLOutputEnabled;
				OpenViBE::CString m_sSignalStreamName;
				OpenViBE::CString m_sSignalStreamID;
				OpenViBE::CString m_sMarkerStreamName;
				OpenViBE::CString m_sMarkerStreamID;


			private:
				lsl::stream_outlet* m_oSignalOutlet;
				lsl::stream_outlet* m_oStimulusOutlet;

				uint32_t m_ui32SampleCountPerSentBlock;
		};


	}
}

#endif // TARGET_HAS_ThirdPartyLSL

#endif // __OpenViBE_AcquisitionServer_LSLOutput_H__

