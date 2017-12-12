
#if defined TARGET_HAS_ThirdPartyOpenAL

#include "ovpCBoxAlgorithmOpenALSoundPlayer.h"
#include <tcptagging/IStimulusSender.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Stimulation;

using namespace std;

#define boolean OpenViBE::boolean

#define BUFFER_SIZE 32768
#define UNIQUE_SOURCE 1

CBoxAlgorithmOpenALSoundPlayer::CBoxAlgorithmOpenALSoundPlayer(void)
: m_pStimulusSender(NULL) 
{ 
	// nop
}

boolean CBoxAlgorithmOpenALSoundPlayer::initialize(void)
{

	m_oStreamDecoder.initialize(*this, 0);
	m_oStreamEncoder.initialize(*this, 0);

	m_ui64PlayTrigger = FSettingValueAutoCast(*this->getBoxAlgorithmContext(),0);
	m_ui64StopTrigger = FSettingValueAutoCast(*this->getBoxAlgorithmContext(),1);
	m_sFileName = FSettingValueAutoCast(*this->getBoxAlgorithmContext(),2);
	m_bLoop = FSettingValueAutoCast(*this->getBoxAlgorithmContext(),3);

	m_ui64LastOutputChunkDate = -1;
	m_bStartOfSoundSent = false;
	m_bEndOfSoundSent = false;

	if(alutInit(NULL,NULL) != AL_TRUE)
	{
		if(alutGetError () == ALUT_ERROR_INVALID_OPERATION)
		{
			this->getLogManager() << LogLevel_Trace << "ALUT already initialized.\n";
		}
		else
		{
			this->getLogManager() << LogLevel_Error << "ALUT initialization returned a bad status.\n";
			this->getLogManager() << LogLevel_Error << "ALUT ERROR:\n"<<alutGetErrorString(alutGetError ())<<"\n";
			return false;
		}
	}

	m_iFileFormat = FILE_FORMAT_UNSUPPORTED;

	string l_sFile((const char *)m_sFileName);
	if(l_sFile.find(".wav") !=string::npos)
	{
		m_iFileFormat = FILE_FORMAT_WAV;
	}
	if(l_sFile.find(".ogg") !=string::npos)
	{
		m_iFileFormat = FILE_FORMAT_OGG;
	}

	m_pStimulusSender = TCPTagging::createStimulusSender();

	if (!m_pStimulusSender->connect("localhost", "15361"))
	{
		this->getLogManager() << LogLevel_Warning << "Unable to connect to AS's TCP Tagging plugin, stimuli wont be forwarded.\n";
	}

	return openSoundFile();
}

boolean CBoxAlgorithmOpenALSoundPlayer::uninitialize(void)
{

	m_oStreamDecoder.uninitialize();
	m_oStreamEncoder.uninitialize();
	
	boolean l_bStatus = stopSound();

#if UNIQUE_SOURCE
	alDeleteSources(1, &m_uiSourceHandle);
#endif
	alDeleteBuffers(1, &m_uiSoundBufferHandle);


	if(alutExit() != AL_TRUE)
	{
		if(alutGetError () == ALUT_ERROR_INVALID_OPERATION)
		{
			this->getLogManager() << LogLevel_Trace << "ALUT already exited.\n";
		}
		else
		{
			this->getLogManager() << LogLevel_Error << "ALUT uninitialization returned a bad status.\n";
			this->getLogManager() << LogLevel_Error << "ALUT ERROR:\n"<<alutGetErrorString(alutGetError ())<<"\n";
			return false;
		}
	}

	if (m_pStimulusSender)
	{
		delete m_pStimulusSender;
		m_pStimulusSender = NULL;
	}

	return l_bStatus;

}
boolean CBoxAlgorithmOpenALSoundPlayer::processClock(OpenViBE::CMessageClock& rMessageClock)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmOpenALSoundPlayer::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmOpenALSoundPlayer::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	if(m_ui64LastOutputChunkDate==(uint64)(-1))
	{
		// Send header on initialize
		m_oStreamEncoder.encodeHeader();
		l_rDynamicBoxContext.markOutputAsReadyToSend(0, 0, 0);
		m_ui64LastOutputChunkDate=0;
	}

	// Look for command stimulations
	for (uint32 i = 0; i < l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		m_oStreamDecoder.decode(i);

		if (m_oStreamDecoder.isHeaderReceived())
		{
			// NOP
		}

		if (m_oStreamDecoder.isBufferReceived())
		{
			const IStimulationSet* l_pStimulationSet = m_oStreamDecoder.getOutputStimulationSet();

			for(uint32 j=0; j<l_pStimulationSet->getStimulationCount(); j++)
			{
				const uint64 l_ui64Stimulation = l_pStimulationSet->getStimulationIdentifier(j);
				if(l_ui64Stimulation == m_ui64PlayTrigger)
				{
					playSound();
					m_bEndOfSoundSent = false;
					m_bStartOfSoundSent = false;
				}
				else if(l_ui64Stimulation == m_ui64StopTrigger)
				{
					stopSound();
				}
				else
				{
					// Immediate passthrough
					m_pStimulusSender->sendStimulation(l_ui64Stimulation);
				}
			}
		}

		if (m_oStreamDecoder.isEndReceived())
		{
			// @fixme potentially bad behavior: the box may send chunks after sending this end
			m_oStreamEncoder.encodeEnd();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, m_ui64LastOutputChunkDate, this->getPlayerContext().getCurrentTime());
			m_ui64LastOutputChunkDate = this->getPlayerContext().getCurrentTime();
		}
	}

	// n.b. TCP Tagging should be used instead of this socket output. This code is kept for backwards compatibility.
	IStimulationSet* l_pOutputStimulationSet = m_oStreamEncoder.getInputStimulationSet();
	l_pOutputStimulationSet->clear();

	ALint l_uiStatus;
	alGetSourcei(m_uiSourceHandle, AL_SOURCE_STATE, &l_uiStatus);
	// CASE : the sound has stopped, and we need to send the stimulation
	if(l_uiStatus == AL_STOPPED && !m_bEndOfSoundSent)
	{
		l_pOutputStimulationSet->appendStimulation(
			m_ui64StopTrigger,
			this->getPlayerContext().getCurrentTime(),
			0);
		m_bEndOfSoundSent = true;
	}
	// CASE : the sound has started playing, and we need to send the stimulation
	if(l_uiStatus == AL_PLAYING && !m_bStartOfSoundSent)
	{
		l_pOutputStimulationSet->appendStimulation(
			m_ui64PlayTrigger,
			this->getPlayerContext().getCurrentTime(),
			0);
		m_bStartOfSoundSent = true;
	}

	m_oStreamEncoder.encodeBuffer();
	l_rDynamicBoxContext.markOutputAsReadyToSend(0, m_ui64LastOutputChunkDate, this->getPlayerContext().getCurrentTime());

	m_ui64LastOutputChunkDate = this->getPlayerContext().getCurrentTime();

	return true;
}

boolean CBoxAlgorithmOpenALSoundPlayer::openSoundFile()
{
	switch(m_iFileFormat)
	{
		case FILE_FORMAT_WAV:
		{
			this->getLogManager() << LogLevel_Trace << "Buffering WAV file (this step may take some times for long files).\n";
			m_uiSoundBufferHandle = alutCreateBufferFromFile(m_sFileName);
			this->getLogManager() << LogLevel_Trace << "WAV file buffered.\n";
			if(m_uiSoundBufferHandle == AL_NONE)
			{
				this->getLogManager() << LogLevel_Error << "ALUT can't create buffer from file "<<m_sFileName<<"\n";
				this->getLogManager() << LogLevel_Error << "ALUT ERROR:\n"<<alutGetErrorString(alutGetError ())<<"\n";
				return false;
			}
			break;
		}
		case FILE_FORMAT_OGG:
		{
			// On windows using fopen+ov_open can lead to failure, as stated in the vorbis official documentation:
			//http://xiph.org/vorbis/doc/vorbisfile/ov_open.html
			// using ov_fopen instead.
			//m_oOggVorbisStream.File = fopen((const char *)m_sFileName, "rb");
			//if (m_oOggVorbisStream.File == NULL)
			//{
			//	this->getLogManager() << LogLevel_Error << "Can't open file "<<m_sFileName<<": IO error\n.";
			//	return false;
			//}
			
#if defined TARGET_OS_Windows
			if(ov_fopen(const_cast<char*>(m_sFileName.toASCIIString()), &m_oOggVorbisStream.Stream) < 0)
#elif defined TARGET_OS_Linux
			if((m_oOggVorbisStream.File = fopen((const char *)m_sFileName, "rb")) == NULL)
			{
				this->getLogManager() << LogLevel_Error << "Can't open file "<<m_sFileName<<": IO error\n.";
				return false;
			}
			if(ov_open(m_oOggVorbisStream.File, &(m_oOggVorbisStream.Stream), NULL, 0) < 0)
#else
#error "Please port this code"
#endif
			{
				this->getLogManager() << LogLevel_Error << "Can't open file "<<m_sFileName<<": OGG VORBIS stream error\n";
				return false;
			}
			
			vorbis_info* l_pInfos = ov_info(&m_oOggVorbisStream.Stream, -1);
			m_oOggVorbisStream.Format     = l_pInfos->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
			m_oOggVorbisStream.SampleRate = l_pInfos->rate;
			
			//Now we fill the raw buffer (good for small piece of sound... use buffering for big files)
			this->getLogManager() << LogLevel_Trace << "Buffering OGG file (this step may take some times for long files).\n";
			int32 l_iBytesRead;
			int32 bitStream;
			char l_pBuffer[BUFFER_SIZE];
			do {
				// Read up to a buffer's worth of decoded sound data
				l_iBytesRead = ov_read(&m_oOggVorbisStream.Stream, l_pBuffer, BUFFER_SIZE, 0, 2, 1, &bitStream);
				// Append to end of buffer
				m_vRawOggBufferFromFile.insert(m_vRawOggBufferFromFile.end(), l_pBuffer, l_pBuffer + l_iBytesRead);
			} while (l_iBytesRead > 0);
			this->getLogManager() << LogLevel_Trace << "OGG file buffered.\n";
			
			//we have decoded all the file. we drop the decoder (file is closed for us).
			ov_clear(&m_oOggVorbisStream.Stream);

			//create empty buffer
			alGenBuffers(1, &m_uiSoundBufferHandle);
			//fill it with raw data
			alBufferData(m_uiSoundBufferHandle, m_oOggVorbisStream.Format, &m_vRawOggBufferFromFile[0], static_cast < ALsizei > (m_vRawOggBufferFromFile.size()), m_oOggVorbisStream.SampleRate);

			break;
		}
		default:
		{
			this->getLogManager() << LogLevel_Error << "Unsupported file format. Please use only WAV or OGG files.\n";
			return false;
		}
	}

#if UNIQUE_SOURCE
	alGenSources(1, &m_uiSourceHandle);
	alSourcei (m_uiSourceHandle, AL_BUFFER, m_uiSoundBufferHandle);
	alSourcei (m_uiSourceHandle, AL_LOOPING, (m_bLoop?AL_TRUE:AL_FALSE));
#endif
	return true;
}
boolean CBoxAlgorithmOpenALSoundPlayer::playSound()
{
	switch(m_iFileFormat)
	{
		case FILE_FORMAT_WAV:
		case FILE_FORMAT_OGG:
		{
#if UNIQUE_SOURCE
			ALint l_uiStatus;
			alGetSourcei(m_uiSourceHandle, AL_SOURCE_STATE, &l_uiStatus);
			if(l_uiStatus == AL_PLAYING)
			{
				// we start back again
				alSourceStop(m_uiSourceHandle);
			}
			alSourcePlay(m_uiSourceHandle);
#else
			ALuint l_uiSource;
			alGenSources(1, &l_uiSource);
			m_vOpenALSources.push_back(l_uiSource);
			alSourcei (l_uiSource, AL_BUFFER, m_uiSoundBufferHandle);
			alSourcei (l_uiSource, AL_LOOPING, (m_bLoop?AL_TRUE:AL_FALSE));
			alSourcePlay(l_uiSource);
#endif
			break;
		}
		default:
		{
			this->getLogManager() << LogLevel_Error << "Unsupported file format. Please use only WAV or OGG files.\n";
			return false;
		}
	}

	m_pStimulusSender->sendStimulation(m_ui64PlayTrigger, 0);		// n.b. 0 is intentional

	return true;
}
boolean CBoxAlgorithmOpenALSoundPlayer::stopSound()
{
	switch(m_iFileFormat)
	{
		case FILE_FORMAT_WAV:
		case FILE_FORMAT_OGG:
		{
			
#if UNIQUE_SOURCE
			alSourceStop(m_uiSourceHandle);
#else
			for(uint32 i = 0;i<m_vOpenALSources.size();i++)
			{
				//stop all sources
				alSourceStop(m_vOpenALSources[i]);
				alDeleteSources(1, &m_vOpenALSources[i]);
			}
			m_vOpenALSources.clear();
#endif
			break;
		}
		default:
		{
			this->getLogManager() << LogLevel_Error << "Unsupported file format. Please use only WAV or OGG files.\n";
			return false;
		}
	}

	m_pStimulusSender->sendStimulation(m_ui64StopTrigger, 0);	// n.b. 0 is intentional

	return true;
}


#endif //TARGET_HAS_ThirdPartyOpenAL
