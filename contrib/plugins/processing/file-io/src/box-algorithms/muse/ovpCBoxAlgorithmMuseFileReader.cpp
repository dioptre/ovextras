#ifdef TARGET_HAS_Protobuf

#include "ovpCBoxAlgorithmMuseFileReader.h"

#include <openvibe/ovITimeArithmetics.h>

#include <cassert>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;

using namespace Muse;

#include "ovpCMuseReaderHelper.h"

boolean CBoxAlgorithmMuseFileReader::initialize(void)
{
	m_oSignalEncoder.initialize(*this, 0);
	m_oStimulationEncoder.initialize(*this, 1);

	m_pMuseReaderHelper = NULL;
	m_ui32SampleCountPerBuffer = static_cast<uint32>((uint64)FSettingValueAutoCast(*getBoxAlgorithmContext(), 1));

	if (m_ui32SampleCountPerBuffer == 0) 
	{
		getLogManager() << LogLevel_Error << "SampleCountPerBuffer is 0, this will not work\n";
		return false;
	}

	uint64 l_ui64DefaultEEGSampleRate = FSettingValueAutoCast(*getBoxAlgorithmContext(), 2);

	if (l_ui64DefaultEEGSampleRate == 0)
	{
		getLogManager() << LogLevel_Error << "DefaultEEGSampleRate is 0, this will not work\n";
		return false;
	}

	float64 l_f64MaxClockSkew = FSettingValueAutoCast(*getBoxAlgorithmContext(), 3);

	if (l_f64MaxClockSkew <= 0)
	{
		getLogManager() << LogLevel_Error << "MaxClockSkew is nonpositive, this will not work\n";
		return false;
	}

	CString l_sFilename = FSettingValueAutoCast(*getBoxAlgorithmContext(), 0);
	m_oFile.open(l_sFilename);
	if (!m_oFile.is_open()) {
		getLogManager() << LogLevel_Error << "Could not open file [" << l_sFilename << "]\n";
		return false;
	}
	try
	{
		m_pMuseReaderHelper = new CMuseReaderHelper(m_oFile, l_ui64DefaultEEGSampleRate, ITimeArithmetics::secondsToTime(l_f64MaxClockSkew));
	}
	catch(CMuseReaderHelper::ParseError e)
	{
		getLogManager() << LogLevel_Error << "Parse error reading [" << l_sFilename << "]: " << e.what() << "\n";
		return false;
	}

	m_oSignalEncoder.getInputSamplingRate() = m_pMuseReaderHelper->getEEGSampleRate();
	m_oSignalEncoder.getInputMatrix()->setDimensionCount(2);
	m_oSignalEncoder.getInputMatrix()->setDimensionSize(1, m_ui32SampleCountPerBuffer);

	m_oStimulationEncoder.encodeHeader();
	getDynamicBoxContext().markOutputAsReadyToSend(1, 0, 0);

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmMuseFileReader::uninitialize(void)
{
	delete m_pMuseReaderHelper;
	m_oFile.close();

	m_oSignalEncoder.uninitialize();
	m_oStimulationEncoder.uninitialize();

	return true;
}
/*******************************************************************************/


boolean CBoxAlgorithmMuseFileReader::processClock(IMessageClock& rMessageClock)
{
	// some pre-processing code if needed...

	// ready to process !
	if (m_pMuseReaderHelper->messageAvailable())
	{
		getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	}

	return true;
}

/*******************************************************************************/


uint64 CBoxAlgorithmMuseFileReader::getClockFrequency(void)
{
	// Note that the time is coded on a 64 bits unsigned integer, fixed decimal point (32:32)
												// arguments in reverse order inverts from period to frequency
	return ITimeArithmetics::sampleCountToTime(m_ui32SampleCountPerBuffer, m_pMuseReaderHelper->getEEGSampleRate());
}

/*******************************************************************************/

boolean CBoxAlgorithmMuseFileReader::process(void)
{
	
	// the static box context describes the box inputs, outputs, settings structures
	//IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	// here is some useful functions:
	
	// - To send an output chunk :
	// l_rDynamicBoxContext.markOutputAsReadyToSend(output_index, chunk_start_time, chunk_end_time);
	
	try
	{
		m_pMuseReaderHelper->parseSamples(m_ui32SampleCountPerBuffer, getLogManager());
		if (m_pMuseReaderHelper->getEEGSampleCount() != m_ui32SampleCountPerBuffer)
		{
			getLogManager() << LogLevel_Error << "Asked for " << m_ui32SampleCountPerBuffer << " samples but provided with " << m_pMuseReaderHelper->getEEGSampleCount() << "\n";
			return false;
		}
	}
	catch(CMuseReaderHelper::ParseError e)
	{
		getLogManager() << LogLevel_Error << ".muse parse error: " << e.what() << "\n";
		return false;
	}

	uint64 l_ui64SignalTime = m_pMuseReaderHelper->getEEGTime();

	uint32 l_ui32EEGChannelCount = m_pMuseReaderHelper->getEEGChannelCount();
	if (l_ui32EEGChannelCount != m_oSignalEncoder.getInputMatrix()->getDimensionSize(0))
	{
		m_oSignalEncoder.getInputMatrix()->setDimensionSize(0, l_ui32EEGChannelCount);
		m_oSignalEncoder.encodeHeader();

		l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64SignalTime, l_ui64SignalTime);
	}

	float64 * l_pBuffer = m_oSignalEncoder.getInputMatrix()->getBuffer();
	float64 * l_pBufferTail = l_pBuffer + m_oSignalEncoder.getInputMatrix()->getBufferElementCount();
	for (uint32 l_ui32Sample = 0; l_ui32Sample < m_ui32SampleCountPerBuffer; ++ l_ui32Sample)
	{
		float32 const * l_pSample = m_pMuseReaderHelper->getEEGSample(l_ui32Sample);
		for (uint32 l_ui32Channel = 0; l_ui32Channel < l_ui32EEGChannelCount; ++ l_ui32Channel)
		{
			*(l_pBuffer++) = l_pSample[l_ui32Channel];
		}
	}
	while (l_pBuffer < l_pBufferTail)
	{
		*(l_pBuffer++) = 0;
	}

	m_oSignalEncoder.encodeBuffer();
	uint64 l_ui64SignalPeriod = m_pMuseReaderHelper->getEEGSamplePeriod() * m_pMuseReaderHelper->getEEGSampleCount();
	l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64SignalTime, l_ui64SignalTime + l_ui64SignalPeriod);


	if (m_pMuseReaderHelper->getStimulationCount() > 0) {
		IStimulationSet * l_pStimulationSet = m_oStimulationEncoder.getInputStimulationSet();

		uint64 l_ui64StartTime = l_ui64SignalTime;
		uint64 l_ui64EndTime = l_ui64SignalTime + l_ui64SignalPeriod;

		l_pStimulationSet->setStimulationCount(m_pMuseReaderHelper->getStimulationCount());
		for (uint32 l_ui32Index = 0; l_ui32Index < m_pMuseReaderHelper->getStimulationCount(); ++ l_ui32Index)
		{
			uint64 l_ui64Stimulation = m_pMuseReaderHelper->getStimulation(l_ui32Index);
			uint64 l_ui64Time = m_pMuseReaderHelper->getStimulationTime(l_ui32Index);
			l_pStimulationSet->insertStimulation(l_ui32Index, l_ui64Stimulation, l_ui64Time, 0);

			if (l_ui64Time < l_ui64StartTime)
			{
				l_ui64StartTime = l_ui64Time;
			}

			if (l_ui64Time > l_ui64EndTime)
			{
				l_ui64EndTime = l_ui64Time;
			}
		}

		m_oStimulationEncoder.encodeBuffer();
		l_rDynamicBoxContext.markOutputAsReadyToSend(1, l_ui64StartTime, l_ui64EndTime);
	}
	
	// A typical process iteration may look like this.
	// This example only iterate over the first input of type Signal, and output a modified Signal.
	// thus, the box uses 1 decoder (m_oSignalDecoder) and 1 encoder (m_oSignalEncoder)
	/*
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//iterate over all chunk on input 0
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		// decode the chunk i
		m_oSignalDecoder.decode(i);
		// the decoder may have decoded 3 different parts : the header, a buffer or the end of stream.
		if(m_oSignalDecoder.isHeaderReceived())
		{
			// Header received. This happens only once when pressing "play". For example with a StreamedMatrix input, you now know the dimension count, sizes, and labels of the matrix
			// ... maybe do some process ...
			
			// Pass the header to the next boxes, by encoding a header on the output 0:
			m_oSignalEncoder.encodeHeader(0);
			// send the output chunk containing the header. The dates are the same as the input chunk:
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
		if(m_oSignalDecoder.isBufferReceived())
		{
			// Buffer received. For example the signal values
			// Access to the buffer can be done thanks to :
			IMatrix* l_pMatrix = m_oSignalDecoder.getOutputMatrix(); // the StreamedMatrix of samples.
			uint64 l_uiSamplingFrequency = m_oSignalDecoder.getOutputSamplingRate(); // the sampling rate of the signal
			
			// ... do some process on the matrix ...

			// Encode the output buffer :
			m_oSignalEncoder.encodeBuffer(0);
			// and send it to the next boxes :
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			
		}
		if(m_oSignalDecoder.isEndReceived())
		{
			// End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
			m_oSignalEncoder.encodeEnd(0);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

		// The current input chunk has been processed, and automaticcaly discarded.
		// you don't need to call "l_rDynamicBoxContext.markInputAsDeprecated(0, i);"
	}
	*/

	// check the official developer documentation webpage for more example and information :
	
	// Tutorials:
	// http://openvibe.inria.fr/documentation/#Developer+Documentation
	// Codec Toolkit page :
	// http://openvibe.inria.fr/codec-toolkit-references/
	
	// Feel free to ask experienced developers on the forum (http://openvibe.inria.fr/forum) and IRC (#openvibe on irc.freenode.net).

	return true;
}

#endif
