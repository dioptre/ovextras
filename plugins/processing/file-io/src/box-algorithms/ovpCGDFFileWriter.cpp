#include "ovpCGDFFileWriter.h"

#include <cmath>
#include <cfloat>
#include <cstring>

#include <algorithm> // std::min, etc on VS2013
#include <openvibe/ovITimeArithmetics.h>

using namespace OpenViBE;
using namespace OpenViBE::Plugins;
using namespace OpenViBE::Kernel;
using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;
using namespace OpenViBEToolkit;
using namespace std;

void CGDFFileWriter::setChannelCount(const uint32 ui32ChannelCount)
{
	m_oFixedHeader.m_ui32NumberOfSignals = ui32ChannelCount;
	m_oFixedHeader.m_i64NumberOfBytesInHeaderRecord=(ui32ChannelCount+1)*256;

	m_oVariableHeader.setChannelCount(ui32ChannelCount);
	m_vSamples.resize(ui32ChannelCount);
	m_vSampleCount.resize(ui32ChannelCount);
}

void CGDFFileWriter::setChannelName(const uint32 ui32ChannelIndex, const char* sChannelName)
{
	const uint32 l_ui32LabelSize = sizeof(m_oVariableHeader[ui32ChannelIndex].m_sLabel);

	// initialize label with spaces
	memset(m_oVariableHeader[ui32ChannelIndex].m_sLabel, ' ', l_ui32LabelSize);

	// copy at most l_ui32LabelSize-1 characters, leaving room for 1 space. Note that the label is not NULL terminated.
	const uint32 l_ui32ChannelNameLen = std::min<uint32>(strlen(sChannelName), l_ui32LabelSize-1);
	memcpy(m_oVariableHeader[ui32ChannelIndex].m_sLabel, sChannelName, l_ui32ChannelNameLen);

	m_oVariableHeader[ui32ChannelIndex].m_ui32ChannelType=17;                //float64
	m_oVariableHeader[ui32ChannelIndex].m_ui32NumberOfSamplesInEachRecord=1;
	m_oVariableHeader[ui32ChannelIndex].m_f64PhysicalMinimum=+DBL_MAX;       //starting value(to compare later)
	m_oVariableHeader[ui32ChannelIndex].m_f64PhysicalMaximum=-DBL_MAX;       //starting value(to compare later)
	m_oVariableHeader[ui32ChannelIndex].m_i64DigitalMinimum=0x8000000000000000LL;
	m_oVariableHeader[ui32ChannelIndex].m_i64DigitalMaximum=0x7fffffffffffffffLL;

	memcpy(m_oVariableHeader[ui32ChannelIndex].m_sPhysicalDimension, "uV", sizeof("uV"));

}

void CGDFFileWriter::setSampleCountPerBuffer(const uint32 ui32SampleCountPerBuffer)
{
	m_ui32SamplesPerChannel = ui32SampleCountPerBuffer;

	//save the fixed header
	if(m_oFixedHeader.save(m_oFile))
	{
		//save the variable header
		if(!m_oVariableHeader.save(m_oFile))
		{
			m_bError = true;
		}
	}
	else
	{
		m_bError = true;
	}

	if(m_bError)
	{
		getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning << "Error while writing to the output file!\n";
	}
}

void CGDFFileWriter::setSamplingRate(const uint32 ui32SamplingFrequency)
{
	m_ui64SamplingFrequency = ui32SamplingFrequency;

	m_oFixedHeader.m_ui32DurationOfADataRecordNumerator=1;
	m_oFixedHeader.m_ui32DurationOfADataRecordDenominator=(uint32)m_ui64SamplingFrequency;
}

void CGDFFileWriter::setSampleBuffer(const float64* pBuffer)
{
	//for each channel
	for(uint32 j=0; j<m_oFixedHeader.m_ui32NumberOfSignals ; j++)
	{
		for(uint32 i=0; i<m_ui32SamplesPerChannel; i++)
		{

			//gets a sample value
			const float64 l_f64Sample = pBuffer[j*m_ui32SamplesPerChannel+i];
			//actualize channel's digital min/max
			if(fabs(l_f64Sample) > m_oVariableHeader[j].m_f64PhysicalMaximum)
			{
				m_oVariableHeader[j].m_f64PhysicalMaximum = fabs(l_f64Sample+(1/m_f64Precision));
				m_oVariableHeader[j].m_f64PhysicalMinimum = -m_oVariableHeader[j].m_f64PhysicalMaximum;
				m_oVariableHeader[j].m_i64DigitalMaximum=static_cast<int64>(1+(ceil(fabs(l_f64Sample)*m_f64Precision)));
				m_oVariableHeader[j].m_i64DigitalMinimum=-m_oVariableHeader[j].m_i64DigitalMaximum;
			}

			//copy its current sample
			m_vSamples[j].push_back(l_f64Sample*m_f64Precision);
		}

		//updates the sample count
		m_vSampleCount[j]+=m_ui32SamplesPerChannel;
	}

 	// this->getLogManager() << LogLevel_Info << "Received up to " << m_vSampleCount[0] << " samples\n";

	//save in the file
	saveMatrixData();

	//updates the fixed header
	m_oFixedHeader.m_i64NumberOfDataRecords=m_vSampleCount[0];
	if(m_oFixedHeader.update(m_oFile))
	{
		//updates the variable header
		if(!m_oVariableHeader.update(m_oFile))
		{
			m_bError = true;
		}
	}
	else
	{
		m_bError = true;
	}

	if(m_bError)
	{
		getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning << "Error while writing to the output file!\n";
	}

}

/*
* Experiment callback
*
*/
void CGDFFileWriter::setExperimentInformation()
{
	uint64 ui64Value;

	ui64Value = m_pExperimentInformationDecoder->getOutputExperimentIdentifier();
	sprintf(m_oFixedHeader.m_sRecordingId, "0x%08X", (unsigned int)ui64Value);
	m_oFixedHeader.m_sRecordingId[10] = ' ';

	ui64Value = m_pExperimentInformationDecoder->getOutputSubjectIdentifier();
	sprintf(m_oFixedHeader.m_sPatientId, "0x%08X ", (unsigned int)ui64Value);
	m_oFixedHeader.m_sPatientId[11] = ' ';


	m_pExperimentInformationDecoder->getOutputSubjectAge();
	// TODO using the experiment date, compute the birthdate?

	ui64Value = m_pExperimentInformationDecoder->getOutputSubjectGender();
	switch(ui64Value)
	{
		case OVTK_Value_Sex_Female:
			m_oFixedHeader.m_sPatientId[17] = 'F';
			break;

		case OVTK_Value_Sex_Male:
			m_oFixedHeader.m_sPatientId[17] = 'M';
			break;

		case OVTK_Value_Sex_Unknown:
		case OVTK_Value_Sex_NotSpecified:
		default:
			m_oFixedHeader.m_sPatientId[17] = 'X';
			break;
	}
	m_oFixedHeader.m_sPatientId[18] = ' ';


	ui64Value = m_pExperimentInformationDecoder->getOutputLaboratoryIdentifier();
	m_oFixedHeader.m_ui64LaboratoryId = ui64Value;


	ui64Value = m_pExperimentInformationDecoder->getOutputTechnicianIdentifier();
	m_oFixedHeader.m_ui64TechnicianId = ui64Value;

	CString* l_sSubjectNameValue = m_pExperimentInformationDecoder->getOutputSubjectName();
	std::string l_sFormattedSubjectName((*l_sSubjectNameValue).toASCIIString());
	l_sFormattedSubjectName.replace(l_sFormattedSubjectName.begin(), l_sFormattedSubjectName.end(), ' ', '_');
	sprintf(m_oFixedHeader.m_sPatientId + 31, "%s", l_sFormattedSubjectName.c_str());

	if(!m_oFixedHeader.save(m_oFile))
	{
		m_bError = true;

		getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning << "Error while writing to the output file!\n";
	}


}

void CGDFFileWriter::setStimulation(const uint64 ui64StimulationIdentifier, const uint64 ui64StimulationDate)
{
	m_oEvents.push_back(pair<uint64,uint64>(ui64StimulationIdentifier, ui64StimulationDate));
}

CGDFFileWriter::CGDFFileWriter(void) :
	m_bError(false)
{
}

boolean CGDFFileWriter::initialize()
{
	const IBox* l_pBox=getBoxAlgorithmContext()->getStaticBoxContext();

	m_pSignalDecoder = new OpenViBEToolkit::TSignalDecoder<CGDFFileWriter>;
	m_pExperimentInformationDecoder = new OpenViBEToolkit::TExperimentInformationDecoder<CGDFFileWriter>;
	m_pStimulationDecoder = new OpenViBEToolkit::TStimulationDecoder<CGDFFileWriter>;

	m_pSignalDecoder->initialize(*this,1);
	m_pExperimentInformationDecoder->initialize(*this,0);
	m_pStimulationDecoder->initialize(*this,2);

	// Parses box settings to find filename
	l_pBox->getSettingValue(0, m_sFileName);

	m_f64Precision = 1000.0;

	return true;
}

boolean CGDFFileWriter::uninitialize()
{
	// See that no event is outside the signal by padding the signal with zeroes if necessary
	padByEvents();

	//If the file is not open, that mean that the box is muted. If the file is not open because of a bug, it should have already been notify
	if(m_oFile.is_open())
	{
		//update the fixed header
		if(m_vSampleCount.size() != 0)
		{
			m_oFixedHeader.m_i64NumberOfDataRecords=m_vSampleCount[0];
			this->getLogManager() << LogLevel_Trace << "Saving " << m_vSampleCount[0] << " data records\n";
		}

		if(m_oFixedHeader.update(m_oFile))
		{
			//To save the Physical/Digital max/min values
			if(!m_oVariableHeader.update(m_oFile))
			{
				m_bError = true;
			}
		}
		else
		{
			m_bError = true;
		}

		//write events
		if(!m_oEvents.empty())
		{
			this->getLogManager() << LogLevel_Trace << "Saving " << (uint32)m_oEvents.size() << " events\n";
			saveEvents();
		}

		if(m_bError)
		{
			getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning << "Error while writing to the output file!\n";
		}

		m_oFile.close();
	}

	if(m_pSignalDecoder)
	{
		m_pSignalDecoder->uninitialize();
		delete m_pSignalDecoder;
	}
	if(m_pExperimentInformationDecoder)
	{
		m_pExperimentInformationDecoder->uninitialize();
		delete m_pExperimentInformationDecoder;
	}
	if(m_pStimulationDecoder)
	{
		m_pStimulationDecoder->uninitialize();
		delete m_pStimulationDecoder;
	}

	return true;
}

boolean CGDFFileWriter::processInput(uint32 ui32InputIndex)
{
	if(m_bError)
	{
		return false;
	}

	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

void CGDFFileWriter::saveMatrixData()
{
	if(!m_oFile.is_open())
	{
		m_oFile.open(m_sFileName, ios::binary | ios::trunc);

		if(!m_oFile.good())
		{
			m_bError = true;

			getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "Could not open file [" << m_sFileName << "]\n";
			return;
		}
	}

	size_t i,j;
	m_oFile.seekp(0, ios::end);

	//to "convert" (if needed) the float64 in little endian format
	uint8 l_pLittleEndianBuffer[8];

	for(i=0; i<m_vSamples[0].size(); i++)
	{
		for(j=0; j<m_vSamples.size(); j++)
		{
			float64 v=m_vSamples[j][i];

			System::Memory::hostToLittleEndian(v, l_pLittleEndianBuffer);
			m_oFile.write(reinterpret_cast<char *>(l_pLittleEndianBuffer), sizeof(l_pLittleEndianBuffer));
		}
	}
	for(j=0; j<m_vSamples.size(); j++)
	{
		m_vSamples[j].clear();
	}
}

void CGDFFileWriter::padByEvents()
{
	if (!m_oFile.is_open())
	{
		m_oFile.open(m_sFileName, ios::binary | ios::trunc);

		if (!m_oFile.good())
		{
			m_bError = true;

			getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "Could not open file [" << m_sFileName << "]\n";
			return;
		}
	}

	// Make sure first that the stims are not outside the file boundary; if so, pad the file with zeroes
	vector<pair<uint64, uint64> >::iterator l_oIterator;
	uint32 l_ui32MaxPos = 0;
	for (l_oIterator = m_oEvents.begin(); l_oIterator != m_oEvents.end(); l_oIterator++)
	{
		// GDF Spec v2.51, #33: sample indexing starts from 1, hence +1
		const uint32 l_ui32Position = static_cast<uint32>(ITimeArithmetics::timeToSampleCount(m_ui64SamplingFrequency, l_oIterator->second)) + 1;
		if (l_ui32Position > m_vSampleCount[0] + 1)
		{
			this->getLogManager() << LogLevel_Warning << "Stimulation " << (uint16)(l_oIterator->first & 0xFFFF) << " will be written at " << l_ui32Position << " (after last sample at " << m_vSampleCount[0] + 1 << "), padding the signal\n";
			l_ui32MaxPos = std::max<uint32>(l_ui32Position, l_ui32MaxPos);
		}
	}
	if (l_ui32MaxPos > 0)
	{
		const IMatrix* l_pOutputMatrix = m_pSignalDecoder->getOutputMatrix();
		CMatrix l_oZeros;
		OpenViBEToolkit::Tools::Matrix::copyDescription(l_oZeros, *l_pOutputMatrix);
		OpenViBEToolkit::Tools::Matrix::clearContent(l_oZeros);

		while (l_ui32MaxPos >= m_vSampleCount[0] + 1)
		{
			setSampleBuffer(l_oZeros.getBuffer());
		}
	}
}

void CGDFFileWriter::saveEvents()
{
	if(!m_oFile.is_open())
	{
		m_oFile.open(m_sFileName, ios::binary | ios::trunc);

		if(!m_oFile.good())
		{
			m_bError = true;

			getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "Could not open file [" << m_sFileName << "]\n";
			return;
		}
	}

	m_oFile.seekp(0, ios::end);

	//event mode
	m_oFile.put(1);

	//sample rate associated with event positions
	m_oFile.put(0);
	m_oFile.put(0);
	m_oFile.put(0);

	//number of events
	uint8 l_pLittleEndianBuffer[4];
	System::Memory::hostToLittleEndian((uint32)m_oEvents.size(), l_pLittleEndianBuffer);
	m_oFile.write(reinterpret_cast<char *>(l_pLittleEndianBuffer), sizeof(l_pLittleEndianBuffer));

	// write event positions
	vector<pair<uint64, uint64> >::iterator l_oIterator;
	for (l_oIterator = m_oEvents.begin(); l_oIterator != m_oEvents.end(); l_oIterator++)
	{
		// GDF Spec v2.51, #33: sample indexing starts from 1, hence +1
		const uint32 l_ui32Position = static_cast<uint32>(ITimeArithmetics::timeToSampleCount(m_ui64SamplingFrequency, l_oIterator->second)) + 1;
		// Legacy code: 
		// l_ui32Position = (uint32)(((l_oIterator->second + 1) * m_ui64SamplingFrequency - 1)>>32);
		// std::cout << "Time " << ITimeArithmetics::timeToSeconds(l_oIterator->second) << "s to " << l_ui32Position
		//	<< " samples, is " << ITimeArithmetics::timeToSeconds( ITimeArithmetics::sampleCountToTime(m_ui64SamplingFrequency, l_ui32Position)) << "sec backmapped\n";

		System::Memory::hostToLittleEndian(l_ui32Position, l_pLittleEndianBuffer);
		m_oFile.write(reinterpret_cast<char *>(l_pLittleEndianBuffer), 4);
	}

	// write event types
	for(l_oIterator=m_oEvents.begin() ; l_oIterator!=m_oEvents.end() ; l_oIterator++)
	{
		//Force to use only 16bits stimulations IDs
		const uint16 l_ui16Type = (uint16)(l_oIterator->first&0xFFFF);

		System::Memory::hostToLittleEndian(l_ui16Type, l_pLittleEndianBuffer);
		m_oFile.write(reinterpret_cast<char *>(l_pLittleEndianBuffer), 2);
	}
}

boolean CGDFFileWriter::process()
{
	if(!m_oFile.is_open())
	{
		m_oFile.open(m_sFileName, ios::binary | ios::trunc);

		if(!m_oFile.good())
		{
			m_bError = true;

			getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_ImportantWarning << "Could not open file [" << m_sFileName << "]\n";
			return false;
		}
	}

	IBoxIO* l_pBoxIO=getBoxAlgorithmContext()->getDynamicBoxContext();

	//Experiment information
	for(uint32 i=0; i<l_pBoxIO->getInputChunkCount(0); i++)
	{
		m_pExperimentInformationDecoder->decode(i);
		if(m_pExperimentInformationDecoder->isHeaderReceived())
		{
			setExperimentInformation();
		}
		l_pBoxIO->markInputAsDeprecated(0, i);
	}


	//Signal
	for(uint32 i=0; i<l_pBoxIO->getInputChunkCount(1); i++)
	{
		m_pSignalDecoder->decode(i);
		if(m_pSignalDecoder->isHeaderReceived())
		{
			IMatrix* l_pOutputMatrix = m_pSignalDecoder->getOutputMatrix();
			uint32 l_ui32ChannelCount = l_pOutputMatrix->getDimensionSize(0);
			setChannelCount(l_ui32ChannelCount);
			for(uint32 i=0; i<l_ui32ChannelCount; i++)
			{
				setChannelName(i, l_pOutputMatrix->getDimensionLabel(0,i));
			}
			setSamplingRate((uint32)m_pSignalDecoder->getOutputSamplingRate());
			setSampleCountPerBuffer(l_pOutputMatrix->getDimensionSize(1));

		}
		if(m_pSignalDecoder->isBufferReceived())
		{
			IMatrix* l_pOutputMatrix = m_pSignalDecoder->getOutputMatrix();
			float64* l_pBuffer = l_pOutputMatrix->getBuffer();
			setSampleBuffer(l_pBuffer);
		}
		l_pBoxIO->markInputAsDeprecated(1, i);
	}

	//Stimulations
	for(uint32 i=0; i<l_pBoxIO->getInputChunkCount(2); i++)
	{
		m_pStimulationDecoder->decode(i);

		if(m_pStimulationDecoder->isBufferReceived())
		{
			IStimulationSet* l_pStimulationSet = m_pStimulationDecoder->getOutputStimulationSet();
			for(uint32 i=0; i<l_pStimulationSet->getStimulationCount(); i++)
			{
				setStimulation(l_pStimulationSet->getStimulationIdentifier(i), l_pStimulationSet->getStimulationDate(i));
			}
		}

		l_pBoxIO->markInputAsDeprecated(2, i);
	}

	return true;
}
