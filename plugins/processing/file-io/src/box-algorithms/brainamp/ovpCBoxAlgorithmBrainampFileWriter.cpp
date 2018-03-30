#include "ovpCBoxAlgorithmBrainampFileWriter.h"
#include <openvibe/ovITimeArithmetics.h>
#include <fs/Files.h>

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#include <strings.h>
#define _strcmpi strcasecmp
#endif

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;

bool CBoxAlgorithmBrainampFileWriter::initialize(void)
{
	// Creates algorithms
	m_pSignalStreamDecoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamDecoder));
	m_pStimulationStreamDecoder=&getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamDecoder));

	m_pSignalStreamDecoder->initialize();
	m_pStimulationStreamDecoder->initialize();

	// Signal stream encoder parameters
	op_ui64SamplingRate.initialize(m_pSignalStreamDecoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_SamplingRate));


	// Connect parameters together
	ip_pSignalMemoryBuffer.initialize(m_pSignalStreamDecoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_InputParameterId_MemoryBufferToDecode));
	ip_pStimulationsMemoryBuffer.initialize(m_pStimulationStreamDecoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_InputParameterId_MemoryBufferToDecode));
	op_pMatrix.initialize(m_pSignalStreamDecoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_OutputParameterId_Matrix));
	op_pStimulationSet.initialize(m_pStimulationStreamDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_OutputParameterId_StimulationSet));

	// Configures settings according to box
	m_sOutputFileFullPath = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_sDictionaryFileName = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_bTransformStimulations = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	m_bShouldWriteFullFileNames = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

	// Markers inside the output file are numbered, we need to keep trace of how many of them we have already written
	m_ui64MarkersWritten = 0;
	m_bWasMarkerHeaderWritten = false;

	// Create header, EEG and marker files
	char l_sOutputParentPath[1024];

	// Add the extension if it wasn't done
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
	if((m_sOutputFileFullPath.length() < 5) || (strcasecmp(m_sOutputFileFullPath.toASCIIString() + m_sOutputFileFullPath.length() - 5, ".vhdr") != 0))
#else
	if((m_sOutputFileFullPath.length() < 5) || (_strcmpi(m_sOutputFileFullPath.toASCIIString() + m_sOutputFileFullPath.length() - 5, ".vhdr") != 0))
#endif
	{
		m_sOutputFileFullPath = m_sOutputFileFullPath + ".vhdr";
	}
	if (!FS::Files::getParentPath(m_sOutputFileFullPath.toASCIIString(), l_sOutputParentPath))
	{
		this->getLogManager() << LogLevel_Error << "Cannot access " << m_sOutputFileFullPath << "\n";
		return false;
	}

	if (!FS::Files::directoryExists(l_sOutputParentPath) && !FS::Files::createParentPath(m_sOutputFileFullPath))
	{
		this->getLogManager() << LogLevel_Error << "Directory [" << l_sOutputParentPath << "] cannot be created\n";
		return false;
	}

	// Create header file
	char l_sOutputFilenameWithoutExtension[1024];
	FS::Files::getFilenameWithoutExtension(m_sOutputFileFullPath.toASCIIString(), l_sOutputFilenameWithoutExtension);

	FS::Files::openOFStream(m_oHeaderFileStream, m_sOutputFileFullPath.toASCIIString());
	if (m_oHeaderFileStream.fail())
	{
		this->getLogManager() << LogLevel_Error << "Cannot open " << m_sOutputFileFullPath << " : " << strerror(errno) << "\n";
		return false;
	}

	// Create EEG file
	std::string l_sEEGFileFullPath = std::string(l_sOutputParentPath) + "/" + std::string(l_sOutputFilenameWithoutExtension) + ".eeg";
	FS::Files::openOFStream(m_oEEGFileStream, l_sEEGFileFullPath.c_str(), std::ios::out | std::ios::binary);
	if (m_oEEGFileStream.fail())
	{
		this->getLogManager() << LogLevel_Error << "Cannot open " << l_sEEGFileFullPath.c_str() << " : " << strerror(errno) << "\n";
		return false;
	}

	// Create Markers file
	std::string l_sMarkerFileFullPath = std::string(l_sOutputParentPath) + "/" + std::string(l_sOutputFilenameWithoutExtension) + ".vmrk";
	FS::Files::openOFStream(m_oMarkerFileStream, l_sMarkerFileFullPath.c_str());
	if (m_oMarkerFileStream.fail())
	{
		this->getLogManager() << LogLevel_Error << "Cannot open " << l_sMarkerFileFullPath.c_str() << " : " << strerror(errno) << "\n";
		return false;
	}

	// Opens Marker to OpenViBE Stimulation dictionary file
	if (m_sDictionaryFileName != CString(""))
	{
		std::ifstream l_oMarkerDictionaryFile;
		FS::Files::openIFStream(l_oMarkerDictionaryFile, m_sDictionaryFileName.toASCIIString());
		if (!l_oMarkerDictionaryFile.good())
		{
			getLogManager() << LogLevel_Error << "Could not open dictionary file [" << m_sDictionaryFileName << "]\n";
		}
		else
		{
			getLogManager() << LogLevel_Trace << "Opening " << m_sDictionaryFileName << " succeeded\n";

			// read the values from the dictionar, they are specified in format
			// <Marker Type> , <Marker Name> , <OpenViBE Stimulation>
			// Stimulations can be either written as decimal numbers, or as strings
			do
			{
				std::string l_sLine;
				std::getline(l_oMarkerDictionaryFile, l_sLine, '\n');

				// optionally removes ending carriage return for windows / linux compatibility
				if (l_sLine.length() != 0)
				{
					if (l_sLine[l_sLine.length() - 1] == '\r')
					{
						l_sLine.erase(l_sLine.length() - 1, 1);
					}
				}

				if (l_sLine.length() != 0)
				{
					if(l_sLine[0] == ';') // comments
					{
					}
					else
					{
						std::string l_sMarkerType;
						std::string l_sMarkerName;
						std::string l_sOpenViBEStimulationId;
						std::stringstream l_sStringStream(l_sLine);

						std::getline(l_sStringStream, l_sMarkerType, ',');
						std::getline(l_sStringStream, l_sMarkerName, ',');
						std::getline(l_sStringStream, l_sOpenViBEStimulationId);

						uint64 l_ui64StimulationId = this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_Stimulation, l_sOpenViBEStimulationId.c_str());
						m_mStimulationToMarker[l_ui64StimulationId] = l_sMarkerType + "," + l_sMarkerName;
					}
				}
			}
			while (l_oMarkerDictionaryFile.good());

		}
	}

	return true;
}

bool CBoxAlgorithmBrainampFileWriter::uninitialize(void)
{
	m_pStimulationStreamDecoder->uninitialize();
	m_pSignalStreamDecoder->uninitialize();

	this->getAlgorithmManager().releaseAlgorithm(*m_pStimulationStreamDecoder);
	this->getAlgorithmManager().releaseAlgorithm(*m_pSignalStreamDecoder);

	if (m_oEEGFileStream.is_open())
	{
		m_oEEGFileStream.close();
	}

	if (m_oMarkerFileStream.is_open())
	{
		m_oMarkerFileStream.close();
	}

	return true;
}

bool CBoxAlgorithmBrainampFileWriter::processInput(uint32 ui32InputIndex)
{
	this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

bool CBoxAlgorithmBrainampFileWriter::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	// Signal
	for (uint32 l_uiSignalChunkIndex = 0; l_uiSignalChunkIndex < l_rDynamicBoxContext.getInputChunkCount(0); l_uiSignalChunkIndex++)
	{
		ip_pSignalMemoryBuffer = l_rDynamicBoxContext.getInputChunk(0, l_uiSignalChunkIndex);
		m_pSignalStreamDecoder->process();
		if (m_pSignalStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			// BrainAmp format uses sampling interval (between two samples) in microseconds
			uint64 l_ui64SamplingInterval = 1000000 / static_cast<OpenViBE::uint64>(op_ui64SamplingRate);
			uint64 l_ui64NumberOfChannels = op_pMatrix->getDimensionSize(0);

			if (m_oHeaderFileStream.is_open())
			{
				std::string l_sAuxFileName = "$b";
				if (m_bShouldWriteFullFileNames)
				{
					char l_sOutputFilenameWithoutExtension[1024];
					FS::Files::getFilenameWithoutExtension(m_sOutputFileFullPath.toASCIIString(), l_sOutputFilenameWithoutExtension);
					l_sAuxFileName = l_sOutputFilenameWithoutExtension;
				}

				m_oHeaderFileStream << "Brain Vision Data Exchange Header File Version 1.0" << std::endl;
				m_oHeaderFileStream << "[Common Infos]" << std::endl;
				m_oHeaderFileStream << "DataFile=" << l_sAuxFileName << ".eeg" << std::endl;
				m_oHeaderFileStream << "MarkerFile=" << l_sAuxFileName << ".vmrk" << std::endl;
				m_oHeaderFileStream << "DataFormat=BINARY" << std::endl;
				m_oHeaderFileStream << "DataOrientation=MULTIPLEXED" << std::endl;
				m_oHeaderFileStream << "NumberOfChannels=" << l_ui64NumberOfChannels << std::endl;
				m_oHeaderFileStream << "SamplingInterval=" << l_ui64SamplingInterval << std::endl;
				m_oHeaderFileStream << "[Binary Infos]" << std::endl;
				m_oHeaderFileStream << "BinaryFormat=IEEE_FLOAT_32" << std::endl; // TODO_JL extend this to more formats
				m_oHeaderFileStream << "[Channel Infos]" << std::endl;

				for (uint32 l_ui32ChannelIndex = 0; l_ui32ChannelIndex < l_ui64NumberOfChannels; l_ui32ChannelIndex++)
				{
					// OpenViBE format does not specify any units
					m_oHeaderFileStream << "Ch" << (l_ui32ChannelIndex + 1) << "=" << op_pMatrix->getDimensionLabel(0, l_ui32ChannelIndex) << ",,1" << std::endl;
				}
				m_oHeaderFileStream.close();
			}
			else
			{
				this->getLogManager() << LogLevel_Error << "Cannot open " << m_sOutputFileFullPath << "\n";
				return false;
			}


		}

		if (m_pSignalStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedBuffer))
		{
			if (m_oEEGFileStream.good())
			{

				for (uint32 l_ui32SampleIndex = 0; l_ui32SampleIndex < op_pMatrix->getDimensionSize(1); l_ui32SampleIndex++)
				{
					for (uint32 l_ui32ChannelIndex = 0; l_ui32ChannelIndex < op_pMatrix->getDimensionSize(0); l_ui32ChannelIndex++)
					{
						float32 l_f32SampleValue = static_cast<float32>(op_pMatrix->getBuffer()[l_ui32ChannelIndex * op_pMatrix->getDimensionSize(1) + l_ui32SampleIndex]);
						m_oEEGFileStream.write(reinterpret_cast<char*>(&l_f32SampleValue), sizeof(float32));
					}
				}
			}
		}

		l_rDynamicBoxContext.markInputAsDeprecated(0, l_uiSignalChunkIndex);

	}

	// Stimulations
	for (uint32 l_uiStimulationChunkIndex = 0; l_uiStimulationChunkIndex < l_rDynamicBoxContext.getInputChunkCount(1); l_uiStimulationChunkIndex++)
	{
		ip_pStimulationsMemoryBuffer = l_rDynamicBoxContext.getInputChunk(1, l_uiStimulationChunkIndex);

		m_pStimulationStreamDecoder->process();

		// Handle Header
		if (!m_bWasMarkerHeaderWritten && m_pStimulationStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			std::string l_sAuxFileName = "$b";
			if (m_bShouldWriteFullFileNames)
			{
				char l_sOutputFilenameWithoutExtension[1024];
				FS::Files::getFilenameWithoutExtension(m_sOutputFileFullPath.toASCIIString(), l_sOutputFilenameWithoutExtension);
				l_sAuxFileName = l_sOutputFilenameWithoutExtension;
			}

			m_oMarkerFileStream << "Brain Vision Data Exchange Marker File, Version 1.0" << std::endl;
			m_oMarkerFileStream << "[Common Infos]" << std::endl;
			m_oMarkerFileStream << "DataFile=" << l_sAuxFileName << ".eeg" << std::endl;
			m_oMarkerFileStream << "[Marker Infos]" << std::endl;
			m_bWasMarkerHeaderWritten = true;
		}

		if (m_pStimulationStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedBuffer))
		{
			for(uint32 l_uiStimulationIndex = 0; l_uiStimulationIndex < op_pStimulationSet->getStimulationCount(); l_uiStimulationIndex++)
			{
				uint64 l_ui64StimulationId = op_pStimulationSet->getStimulationIdentifier(l_uiStimulationIndex);
				uint64 l_ui64StimulationDate = op_pStimulationSet->getStimulationDate(l_uiStimulationIndex);
				uint64 l_ui64StimulationDuration = op_pStimulationSet->getStimulationDuration(l_uiStimulationIndex);

				std::string l_sWrittenMarker = "";
				bool l_bWriteStimulation = false;

				if (m_mStimulationToMarker.count(l_ui64StimulationId))
				{
					l_sWrittenMarker = m_mStimulationToMarker[l_ui64StimulationId];
					l_bWriteStimulation = true;
				}
				else if (m_bTransformStimulations)
				{
					std::string l_sStimulusName = this->getTypeManager().getEnumerationEntryNameFromValue(OVTK_TypeId_Stimulation, l_ui64StimulationId).toASCIIString();
					l_sWrittenMarker = "Stimulus," + (l_sStimulusName.empty() ? std::to_string(l_ui64StimulationId) : l_sStimulusName);
					l_bWriteStimulation = true;
				}

				if (l_bWriteStimulation && m_oMarkerFileStream.good())
				{
					m_ui64MarkersWritten++;
					m_oMarkerFileStream << "Mk" << m_ui64MarkersWritten << "=" << l_sWrittenMarker << ",";

					// Calclulate the index of the sample to which the marker is attached
					uint64 l_ui64StimulationSampleIndex = OpenViBE::ITimeArithmetics::timeToSampleCount(op_ui64SamplingRate, l_ui64StimulationDate);
					m_oMarkerFileStream << l_ui64StimulationSampleIndex << ",";

					// Minimal duration of markers inside BrainAmp format seems to be 1 sample
					uint64 l_ui64StimulationDurationInSamples = OpenViBE::ITimeArithmetics::timeToSampleCount(op_ui64SamplingRate, l_ui64StimulationDuration) + 1;
					m_oMarkerFileStream << l_ui64StimulationDurationInSamples << ",";

					// 0 means that the stimulation is attached to all channels
					m_oMarkerFileStream << "0";


					m_oMarkerFileStream << std::endl;
				}
			}
		}

		l_rDynamicBoxContext.markInputAsDeprecated(1, l_uiStimulationChunkIndex);
	}
	return true;
}
