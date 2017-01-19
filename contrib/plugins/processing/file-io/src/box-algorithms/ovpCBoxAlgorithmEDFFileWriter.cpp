#include "ovpCBoxAlgorithmEDFFileWriter.h"

#include <openvibe/ovITimeArithmetics.h>
#include <limits>
#include <sstream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;

/*******************************************************************************/

boolean CBoxAlgorithmEDFFileWriter::initialize(void)
{
	m_ExperimentInformationDecoder.initialize(*this,0);
	m_SignalDecoder.initialize(*this,1);
	m_StimulationDecoder.initialize(*this,2);

	m_sFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	
	m_iSampleFrequency = 0;
	m_iNumberOfChannels = 0;

	m_bIsFileOpened = false;
	m_iFileHandle = -1;

	return true;
}

/*******************************************************************************/

boolean CBoxAlgorithmEDFFileWriter::uninitialize(void)
{

	if (!m_bIsFileOpened)
	{
		// Fine, we didn't manage to write anything
		this->getLogManager() << LogLevel_Warning << "Exiting without writing a file (open failed or no signal header received).\n";

		// Clear the queue
		std::queue<double> empty;
		std::swap(m_vBuffer, empty);

		m_ExperimentInformationDecoder.uninitialize();
		m_SignalDecoder.uninitialize();
		m_StimulationDecoder.uninitialize();

		return true;
	}

	this->getLogManager() << LogLevel_Info << "Writing the file, this may take a moment.\n";

	for (int channel = 0; channel < m_iNumberOfChannels; channel++)
	{
		if (edf_set_samplefrequency(m_iFileHandle, channel, m_iSampleFrequency) == -1)
		{
			this->getLogManager() << LogLevel_ImportantWarning << "edf_set_samplefrequency failed!\n";
			return false;
		}
		if (edf_set_physical_maximum(m_iFileHandle, channel, m_oChannelInformation[channel].m_f64MaxValue) == -1)//1.7*10^308)
		{
			this->getLogManager() << LogLevel_ImportantWarning << "edf_set_physical_maximum failed!\n";
			return false;
		}
		if (edf_set_physical_minimum(m_iFileHandle, channel, m_oChannelInformation[channel].m_f64MinValue) == -1)//-1.7*10^308)
		{
			this->getLogManager() << LogLevel_ImportantWarning << "edf_set_physical_minimum failed!\n";
			return false;
		}
		if (edf_set_digital_maximum(m_iFileHandle, channel, 32767) == -1)
		{
			this->getLogManager() << LogLevel_ImportantWarning << "edf_set_digital_maximum failed!\n";
			return false;
		}
		if (edf_set_digital_minimum(m_iFileHandle, channel, -32768) == -1)
		{
			this->getLogManager() << LogLevel_ImportantWarning << "edf_set_digital_minimum failed!\n";
			return false;
		}
		if (edf_set_label(m_iFileHandle, channel, const_cast <char*>(m_SignalDecoder.getOutputMatrix()->getDimensionLabel(0, channel))) == -1)
		{
			this->getLogManager() << LogLevel_ImportantWarning << "edf_set_label failed!\n";
			return false;
		}
	}

	double* l_pTemporaryBuffer =        new double[m_iSampleFrequency*m_iNumberOfChannels];   // A buffer chunk
	double* l_pTemporaryBufferToWrite = new double[m_iSampleFrequency*m_iNumberOfChannels];	  // Transposed buffer chunk

	// Write out all the complete buffers
	if((int) m_vBuffer.size() >= m_iSampleFrequency*m_iNumberOfChannels)
	{
		this->getLogManager() << LogLevel_Trace << "((int) buffer.size() >= m_iSampleFrequency*m_iNumberOfChannels)\n";
		while((int)m_vBuffer.size() >= m_iSampleFrequency*m_iNumberOfChannels)
		{
			this->getLogManager() << LogLevel_Trace << "while((int)buffer.size() >= m_iSampleFrequency*m_iNumberOfChannels)\n";
			for(int element=0; element<m_iSampleFrequency*m_iNumberOfChannels; element++)
			{
				l_pTemporaryBuffer[element] = (double) m_vBuffer.front();
				m_vBuffer.pop();
			}
			
			for(int channel=0; channel<m_iNumberOfChannels; channel++)
			{
				for(int sample=0; sample<m_iSampleFrequency; sample++)
				{
					l_pTemporaryBufferToWrite[channel*m_iSampleFrequency+sample] = l_pTemporaryBuffer[sample*m_iNumberOfChannels+channel];
				}
			}
			
			if(edf_blockwrite_physical_samples(m_iFileHandle, l_pTemporaryBufferToWrite) == -1)
			{
				this->getLogManager() << LogLevel_ImportantWarning << "edf_blockwrite_physical_samples: Could not write samples in file [" << m_sFilename << "]\n";
				return false;
			}
		}
	}
		
	// Do we have a partial buffer? If so, write it out padded by zeroes
	if(m_vBuffer.size() > 0)
	{
		this->getLogManager() << LogLevel_Trace << "if(buffer.size() > 0))\n";
		for(int element=0; element<m_iSampleFrequency*m_iNumberOfChannels; element++)
		{
			l_pTemporaryBuffer[element] = 0;
		}
		
		for(int element=0; element<(int)m_vBuffer.size(); element++)
		{
			l_pTemporaryBuffer[element] = (double) m_vBuffer.front();
			m_vBuffer.pop();
		}
		
		for(int channel=0; channel<m_iNumberOfChannels; channel++)
		{
			for(int sample=0; sample<m_iSampleFrequency; sample++)
			{
				l_pTemporaryBufferToWrite[channel*m_iSampleFrequency+sample] = l_pTemporaryBuffer[sample*m_iNumberOfChannels+channel];
			}
		}
		
		if(edf_blockwrite_physical_samples(m_iFileHandle, l_pTemporaryBufferToWrite) == -1)
		{
			this->getLogManager() << LogLevel_ImportantWarning << "edf_blockwrite_physical_samples: Could not write samples in file [" << m_sFilename << "]\n";
			return false;
		}
	}
	
	if (l_pTemporaryBuffer) 
	{
		delete[] l_pTemporaryBuffer;
	}
	if (l_pTemporaryBufferToWrite)
	{
		delete[] l_pTemporaryBufferToWrite;
	}

	if(edfclose_file(m_iFileHandle) == -1)
	{
		this->getLogManager() << LogLevel_ImportantWarning << "edfclose_file: Could not close file [" << m_sFilename << "]\n";
		return false;
	}

	m_ExperimentInformationDecoder.uninitialize();
	m_SignalDecoder.uninitialize();
	m_StimulationDecoder.uninitialize();

	return true;
}

/*******************************************************************************/

boolean CBoxAlgorithmEDFFileWriter::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

/*******************************************************************************/

boolean CBoxAlgorithmEDFFileWriter::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
	
	//iterate over all chunk on signal input
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(1); i++)
	{
		m_SignalDecoder.decode(i);
		
		if(m_SignalDecoder.isHeaderReceived())
		{
			m_iSampleFrequency = (int) m_SignalDecoder.getOutputSamplingRate();
			m_iNumberOfChannels = (int) m_SignalDecoder.getOutputMatrix()->getDimensionSize(0);
			m_iNumberOfSamplesPerChunk = (int) m_SignalDecoder.getOutputMatrix()->getDimensionSize(1);
						
			m_iFileHandle = edfopen_file_writeonly(const_cast <char*>(m_sFilename.toASCIIString()), EDFLIB_FILETYPE_EDFPLUS, m_iNumberOfChannels);
			if(m_iFileHandle<0)
			{
				this->getLogManager() << LogLevel_ImportantWarning << "Could not open file [" << m_sFilename << "]\n";
				switch(m_iFileHandle)
				{
					case EDFLIB_MALLOC_ERROR:
						this->getLogManager() << LogLevel_ImportantWarning << "EDFLIB_MALLOC_ERROR: ";
						break;
					case EDFLIB_NO_SUCH_FILE_OR_DIRECTORY:
						this->getLogManager() << LogLevel_ImportantWarning << "EDFLIB_NO_SUCH_FILE_OR_DIRECTORY: ";
						break;
					case EDFLIB_MAXFILES_REACHED:
						this->getLogManager() << LogLevel_ImportantWarning << "EDFLIB_MAXFILES_REACHED: ";
						break;
					case EDFLIB_FILE_ALREADY_OPENED:
						this->getLogManager() << LogLevel_ImportantWarning << "EDFLIB_FILE_ALREADY_OPENED: ";
						break;
					case EDFLIB_NUMBER_OF_SIGNALS_INVALID:
						this->getLogManager() << LogLevel_ImportantWarning << "EDFLIB_NUMBER_OF_SIGNALS_INVALID: ";
						break;
					default:
						this->getLogManager() << LogLevel_ImportantWarning << "EDFLIB_UNKNOWN_ERROR: ";
						break;
				}
				return false;
			}
			
			//set file parameters
			edf_set_startdatetime(m_iFileHandle, 0, 0, 0, 0, 0, 0);

			for(int channel=0; channel<m_iNumberOfChannels; channel++)
			{
				//Creation of one information channel structure per channel
				SChannelInfo l_oChannelInfo = {std::numeric_limits<float64>::max(), -std::numeric_limits<float64>::max()};
				m_oChannelInformation.push_back(l_oChannelInfo);
			}
			
			m_bIsFileOpened = true;
		}
		
		if(m_SignalDecoder.isBufferReceived())
		{
			//put sample in the buffer
			IMatrix* l_pMatrix = m_SignalDecoder.getOutputMatrix();
			for(int sample=0; sample<m_iNumberOfSamplesPerChunk; sample++)
			{
				for(int channel=0; channel<m_iNumberOfChannels; channel++)
				{
					float64 l_f64TempValue = l_pMatrix->getBuffer()[channel*m_iNumberOfSamplesPerChunk+sample];
					if(l_f64TempValue > m_oChannelInformation[channel].m_f64MaxValue)
					{
						m_oChannelInformation[channel].m_f64MaxValue = l_f64TempValue;
					}
					if(l_f64TempValue < m_oChannelInformation[channel].m_f64MinValue)
					{
						m_oChannelInformation[channel].m_f64MinValue = l_f64TempValue;
					}
					m_vBuffer.push(l_f64TempValue);
				}
			}
		}
		
		if(m_SignalDecoder.isEndReceived())
		{
			
		}
	}
	
	if(m_bIsFileOpened)
	{
		//iterate over all chunk on experiment information input
		for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
		{
			m_ExperimentInformationDecoder.decode(i);
			
			if(m_ExperimentInformationDecoder.isHeaderReceived())
			{
				//set patient code
				char l_PatientCode [400];
				sprintf(l_PatientCode, "%llu", (unsigned long long int)m_ExperimentInformationDecoder.getOutputSubjectIdentifier());
				edf_set_patientcode(m_iFileHandle, const_cast <char *>(l_PatientCode));

				//set patient gender
				switch(m_ExperimentInformationDecoder.getOutputSubjectGender())
				{
					case 9://unspecified
						break;
					case 2://female
						edf_set_gender(m_iFileHandle, 0);
						break;
					case 1://male
						edf_set_gender(m_iFileHandle, 1);
						break;
					case 0://unknown
						break;
				}
				
				//set patient age
				char l_PatientAge [40];
				strcpy(l_PatientAge, "Patient age = ");
				char l_TemporyPatientAge [10];
				sprintf(l_TemporyPatientAge, "%u", (unsigned int)m_ExperimentInformationDecoder.getOutputSubjectAge());
				strcat(l_PatientAge, l_TemporyPatientAge);
				edf_set_patient_additional(m_iFileHandle, const_cast <char *>(l_PatientAge));
			}
		}
	}
	
	
	//iterate over all chunk on stimulation input
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(2); i++)
	{
		m_StimulationDecoder.decode(i);
		
		if(m_StimulationDecoder.isHeaderReceived())
		{
			
		}
		
		if(m_StimulationDecoder.isBufferReceived())
		{
			const IStimulationSet* l_pStimulationSet = m_StimulationDecoder.getOutputStimulationSet();
			for(uint32 stimulation_index = 0; stimulation_index < l_pStimulationSet->getStimulationCount(); stimulation_index++)
			{
				const uint64 l_ui64StimulationDate = l_pStimulationSet->getStimulationDate(stimulation_index);
				const long long l_StimulationDate = (long long)(ITimeArithmetics::timeToSeconds(l_ui64StimulationDate)/0.0001);

				const uint64 l_ui64StimulationDuration = l_pStimulationSet->getStimulationDuration(stimulation_index);
				const long long l_StimulationDuration = (long long)(ITimeArithmetics::timeToSeconds(l_ui64StimulationDuration)/0.0001);
				
				const uint64 l_ui64StimulationIdentifier = l_pStimulationSet->getStimulationIdentifier(stimulation_index);
				CString l_sStimulationIdentifier = this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, l_ui64StimulationIdentifier);
				if (l_sStimulationIdentifier.length() == 0) 
				{
					// If the stimulation number has not been registered as an enum, just pass the number
					std::stringstream ss; ss << l_ui64StimulationIdentifier;
					l_sStimulationIdentifier = ss.str().c_str();
				}
				const int result = edfwrite_annotation_utf8(m_iFileHandle, l_StimulationDate, l_StimulationDuration, const_cast <char *>(l_sStimulationIdentifier.toASCIIString()));
				if(result == -1)
				{
					this->getLogManager() << LogLevel_ImportantWarning << "edfwrite_annotation_utf8 failed!\n";
					return false;
				}
			}
		}
		
		if(m_StimulationDecoder.isEndReceived())
		{
			
		}
	}
	
    return true;
}
