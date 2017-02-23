#ifndef __OpenViBEPlugins_BoxAlgorithm_OpenALSoundPlayer_H__
#define __OpenViBEPlugins_BoxAlgorithm_OpenALSoundPlayer_H__

#if defined TARGET_HAS_ThirdPartyOpenAL

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <AL/alut.h>
#include <vorbis/vorbisfile.h>
#include <iostream>
#include <vector>

#define OVP_ClassId_BoxAlgorithm_OpenALSoundPlayer      OpenViBE::CIdentifier(0x7AC2396F, 0x7EE52EFE)
#define OVP_ClassId_BoxAlgorithm_OpenALSoundPlayerDesc  OpenViBE::CIdentifier(0x6FD040EF, 0x7E2F1284)

namespace TCPTagging
{
	class IStimulusSender; // fwd declare
};

namespace OpenViBEPlugins
{
	namespace Stimulation
	{
		class CBoxAlgorithmOpenALSoundPlayer : public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:
			struct OggVorbisStream{
				OggVorbis_File Stream;
				FILE*          File;
				ALenum         Format;
				ALsizei        SampleRate;
			};
			enum ESupportedFileFormat{
				FILE_FORMAT_WAV = 0,
				FILE_FORMAT_OGG,
				FILE_FORMAT_UNSUPPORTED
			};

			CBoxAlgorithmOpenALSoundPlayer(void);

			virtual void release(void) { delete this; }

			virtual OpenViBE::uint64 getClockFrequency(void){ return (128LL<<32); }
			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean processClock(OpenViBE::CMessageClock& rMessageClock);
			virtual OpenViBE::boolean process(void);

			virtual OpenViBE::boolean openSoundFile();
			virtual OpenViBE::boolean playSound();
			virtual OpenViBE::boolean stopSound();
		
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_OpenALSoundPlayer);

		protected:

			OpenViBEToolkit::TStimulationDecoder<CBoxAlgorithmOpenALSoundPlayer> m_oStreamDecoder;
			OpenViBEToolkit::TStimulationEncoder<CBoxAlgorithmOpenALSoundPlayer> m_oStreamEncoder;

			OpenViBE::uint64 m_ui64LastOutputChunkDate;
			OpenViBE::boolean m_bStartOfSoundSent;
			OpenViBE::boolean m_bEndOfSoundSent;
			OpenViBE::boolean m_bLoop;
			OpenViBE::uint64 m_ui64PlayTrigger;
			OpenViBE::uint64 m_ui64StopTrigger;
			OpenViBE::CString m_sFileName;

			std::vector<ALuint> m_vOpenALSources;
			ESupportedFileFormat m_iFileFormat;

			//The handles
			ALuint m_uiSoundBufferHandle;
			ALuint m_uiSourceHandle;
			//OGG
			OggVorbisStream m_oOggVorbisStream;
			std::vector < char > m_vRawOggBufferFromFile;
			
			// For sending stimulations to the TCP Tagging
			TCPTagging::IStimulusSender* m_pStimulusSender;
		};

		class CBoxAlgorithmOpenALSoundPlayerDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Sound Player"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Laurent Bonnet"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Play/Stop a sound, with or without loop."); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Available format : WAV / OGG. Play and stop with input stimulations. Box based on OpenAL."); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Stimulation"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.1"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_OpenALSoundPlayer; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Stimulation::CBoxAlgorithmOpenALSoundPlayer; }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-media-play"); }

			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput  ("Input triggers", OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addOutput ("Resync triggers (deprecated)", OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addSetting("PLAY trigger", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
				rBoxAlgorithmPrototype.addSetting("STOP trigger", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
				rBoxAlgorithmPrototype.addSetting("File to play", OV_TypeId_Filename, "${Path_Data}/plugins/stimulation/ov_beep.wav");
				rBoxAlgorithmPrototype.addSetting("Loop", OV_TypeId_Boolean, "false");
				rBoxAlgorithmPrototype.addSetting("TCP Tagging Host address", OV_TypeId_String, "localhost");
				rBoxAlgorithmPrototype.addSetting("TCP Tagging Host port", OV_TypeId_Integer, "15361");
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_OpenALSoundPlayerDesc);
		};
	};
};

#endif //TARGET_HAS_ThirdPartyOpenAL
#endif // __OpenViBEPlugins_BoxAlgorithm_OpenALSoundPlayer_H__
