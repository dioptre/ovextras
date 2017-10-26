#include "ovpCBCICompetitionIIIbReader.h"

#include <openvibe/ovITimeArithmetics.h>

#include <iostream>
#include <cmath>
#include <cfloat>
#include <cstdlib>
#include <cstring>

using namespace OpenViBE;
using namespace OpenViBE::Plugins;
using namespace OpenViBE::Kernel;
using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;
using namespace OpenViBEToolkit;

using namespace std;

#define BCICompetitionIIIbReader_UndefinedClass 0xFFFFFFFFFFLL


namespace OpenViBEPlugins
{
	namespace FileIO
	{

		CBCICompetitionIIIbReader::CBCICompetitionIIIbReader()
			: m_bErrorOccurred(false),
			m_ui64FileSize(0),
			m_ui64ClockFrequency(100LL<<32),
			m_ui32SamplesPerBuffer(0),
			m_ui32SamplingRate(125),
			m_ui32SentSampleCount(0),
			m_bEndOfFile(false),
			m_ui32CurrentTrial(0)
		{
			/*TODO*/
		}

		void CBCICompetitionIIIbReader::writeSignalInformation()
		{
			m_oSignalEncoder.getInputSamplingRate() = 125;
			m_oSignalEncoder.getInputMatrix()->setDimensionCount(2);
			m_oSignalEncoder.getInputMatrix()->setDimensionSize(0,2);						// channels
			m_oSignalEncoder.getInputMatrix()->setDimensionSize(1,m_ui32SamplesPerBuffer);	// samples per buffer

			m_oSignalEncoder.getInputMatrix()->setDimensionLabel(0, 0, "+C3a-C3p");
			m_oSignalEncoder.getInputMatrix()->setDimensionLabel(0, 1, "+C4a-C4p");
		}

		OpenViBE::boolean CBCICompetitionIIIbReader::initialize()
		{
			m_oSignalEncoder.initialize(*this,0);
			m_oStimulationEncoder.initialize(*this,1);

			const IBox* l_pBoxContext=getBoxAlgorithmContext()->getStaticBoxContext();

			// Parses box settings to find filename
			const CString l_sSignalFile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

			//opens the file
			if(l_sSignalFile)
			{
				m_oSignalFile.open(l_sSignalFile);
			}
			if(!m_oSignalFile.good())
			{
				this->getLogManager() << LogLevel_ImportantWarning << "Could not open file [" << l_sSignalFile << "]\n";
				return false;
			}

			m_oSignalFile.seekg(0, ios::end);
			m_ui64FileSize = (uint64)m_oSignalFile.tellg();
			m_oSignalFile.seekg(0, ios::beg);

			std::string l_oLine;
			std::istringstream l_oStringStream;

			//get trial length
			m_f64TrialLength = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 10);

			//get CUE display start
			m_f64CueDisplayStart = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 11);

			//get feedback start
			m_f64FeedbackStart = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 12);

			//read triggers
			std::ifstream l_oTriggerFile;
			const CString l_sTriggerFile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

			if(l_sTriggerFile)
			{
				l_oTriggerFile.open(l_sTriggerFile);
			}

			uint64 l_ui64Trigger;
			while(getline(l_oTriggerFile, l_oLine))
			{
				l_oStringStream.clear();
				l_oStringStream.str(l_oLine);
				l_oStringStream>>l_ui64Trigger;

				m_oTriggerTime.push_back(l_ui64Trigger);
				m_oCueDisplayStart.push_back(l_ui64Trigger + (uint64)floor(m_ui32SamplingRate * m_f64CueDisplayStart));
				m_oFeedbackStart.push_back(l_ui64Trigger + (uint64)floor(m_ui32SamplingRate * m_f64FeedbackStart));
				m_oEndOfTrial.push_back(l_ui64Trigger + (uint64)floor(m_ui32SamplingRate * m_f64TrialLength));
			}
			l_oTriggerFile.close();

			//read labels
			std::ifstream l_oLabelsFile;
			const CString l_sLabelsFile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

			if(l_sLabelsFile)
			{
				l_oLabelsFile.open(l_sLabelsFile);
			}

			uint64 l_ui64Label;
			while(getline(l_oLabelsFile, l_oLine))
			{
				if(l_oLine.compare(0, 3, "NaN", 0, 3) == 0)
				{
					m_oClassLabels.push_back(BCICompetitionIIIbReader_UndefinedClass);
				}
				else
				{
					l_oStringStream.clear();
					l_oStringStream.str(l_oLine);
					l_oStringStream>>l_ui64Label;
					m_oClassLabels.push_back(l_ui64Label);
				}
			}
			l_oLabelsFile.close();

			//read artifacts
			std::ifstream l_oArtifactFile;
			const CString l_sArtifactFile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

			if(l_sArtifactFile)
			{
				l_oArtifactFile.open(l_sArtifactFile);
			}

			uint64 l_ui64Artifact;
			while(getline(l_oArtifactFile, l_oLine))
			{
				l_oStringStream.clear();
				l_oStringStream.str(l_oLine);
				l_oStringStream>>l_ui64Artifact;
				m_oArtifacts.push_back(l_ui64Artifact == 1);
			}
			l_oArtifactFile.close();

			//read true labels
			std::ifstream l_oTrueLabelsFile;
			const CString l_sTrueLabelsFile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
			if(l_sTrueLabelsFile)
			{
				l_oTrueLabelsFile.open(l_sTrueLabelsFile);
			}
			while(getline(l_oTrueLabelsFile, l_oLine))
			{
				l_oStringStream.clear();
				l_oStringStream.str(l_oLine);
				l_oStringStream>>l_ui64Label;
				m_oTrueLabels.push_back(l_ui64Label);
			}
			l_oTrueLabelsFile.close();

			// Gets the size of output buffers
			m_ui32SamplesPerBuffer = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

			//Offline/Online
			const bool l_bOffline = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);
			if(!l_bOffline)
			{
				//computes clock frequency
				if(m_ui32SamplesPerBuffer <= m_ui32SamplingRate)
				{
					if(m_ui32SamplingRate % m_ui32SamplesPerBuffer != 0)
					{
						getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Warning <<
							"The sampling rate isn't a multiple of the buffer size\n" <<
							"Please consider adjusting the BCI Competition IIIb reader settings to correct this!\n";
					}

					if(m_ui32SamplesPerBuffer==0) 
					{
						getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << LogLevel_Error <<
							"SamplesPerBuffer is 0, this will not work\n";
						return false;
					}

					// Intentional parameter swap to get the frequency
					m_ui64ClockFrequency = ITimeArithmetics::sampleCountToTime(m_ui32SamplesPerBuffer, m_ui32SamplingRate);
				}

			}

			//Test/Training
			m_bKeepTrainingSamples = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);
			m_bKeepTestSamples = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8);
			m_bKeepArtifactSamples = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 9);

			writeSignalInformation();

			m_oSignalEncoder.encodeHeader();
			m_oStimulationEncoder.encodeHeader();

			getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, 0, 0);
			getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(1, 0, 0);

			m_pMatrixBuffer = m_oSignalEncoder.getInputMatrix();

			return true;
		}

		OpenViBE::boolean CBCICompetitionIIIbReader::uninitialize()
		{
			m_oStimulationEncoder.uninitialize();
			m_oSignalEncoder.uninitialize();

			if(m_oSignalFile)
			{
				m_oSignalFile.close();
			}

			return true;
		}

		OpenViBE::boolean CBCICompetitionIIIbReader::processClock(CMessageClock& rMessageClock)
		{
			if(!m_bEndOfFile)
			{
				getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
			}

			return true;
		}

		OpenViBE::boolean CBCICompetitionIIIbReader::process()
		{
			if(m_bErrorOccurred)
			{
				return false;
			}

			IBoxIO * l_pBoxIO = getBoxAlgorithmContext()->getDynamicBoxContext();

			//reading signal
			//reset vector
			float64* l_pBuffer = m_pMatrixBuffer->getBuffer();
			for(uint32 i=0;i<m_pMatrixBuffer->getBufferElementCount();i++)
			{
				l_pBuffer[i] = 0;
			}

			std::istringstream l_oStringStream;
			std::string l_oLine;
			float64 l_f64Sample;

			uint32 count = 0;
			for( ; count<m_ui32SamplesPerBuffer && !m_bEndOfFile; count++)
			{
				if( !getline(m_oSignalFile, l_oLine) )
				{
					m_bEndOfFile = true;
				}

				l_oStringStream.clear();
				l_oStringStream.str(l_oLine);

				l_oStringStream>>l_f64Sample;
				if(std::isnan(l_f64Sample))
				{
					l_pBuffer[count] = (count != 0) ? l_pBuffer[count-1] : 0.0;
				}
				else
				{
					l_pBuffer[count] = l_f64Sample;
				}

				l_oStringStream>>l_f64Sample;
				if(std::isnan(l_f64Sample))
				{
					l_pBuffer[count+m_ui32SamplesPerBuffer] = (count != 0) ? l_pBuffer[count+m_ui32SamplesPerBuffer-1] : 0.0;
				}
				else
				{
					l_pBuffer[count+m_ui32SamplesPerBuffer] = l_f64Sample;
				}
			}

			m_ui32SentSampleCount += count;

			//A signal matrix is ready to be output
			m_oSignalEncoder.encodeBuffer();

			const uint64 l_ui64StartTime = ITimeArithmetics::sampleCountToTime(m_ui32SamplingRate, (uint64)(m_ui32SentSampleCount - count));
			const uint64 l_ui64EndTime  =  ITimeArithmetics::sampleCountToTime(m_ui32SamplingRate, (uint64)(m_ui32SentSampleCount));

			l_pBoxIO->markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);
			//////

			//Stimulations
			vector<std::pair<OpenViBE::uint64, OpenViBE::uint64> > l_oEvents;
			boolean l_bChanged = true;

			while(l_bChanged)
			{
				l_bChanged = false;

				const boolean l_bKeepCurrentTrial =
					((m_oArtifacts[m_ui32CurrentTrial] && m_bKeepArtifactSamples) || !m_oArtifacts[m_ui32CurrentTrial]) &&
					((m_oClassLabels[m_ui32CurrentTrial] == BCICompetitionIIIbReader_UndefinedClass && m_bKeepTestSamples) ||
					 (m_oClassLabels[m_ui32CurrentTrial] != BCICompetitionIIIbReader_UndefinedClass && m_bKeepTrainingSamples));

				if(m_oTriggerTime[m_ui32CurrentTrial]>(m_ui32SentSampleCount-count) &&
						m_oTriggerTime[m_ui32CurrentTrial]<=m_ui32SentSampleCount
				     )
				{
					if(l_bKeepCurrentTrial)
					{
						//start of trial
						l_oEvents.push_back(pair<uint64,uint64>(0x300,m_oTriggerTime[m_ui32CurrentTrial]));

						//display cross
						l_oEvents.push_back(pair<uint64,uint64>(0x312,m_oTriggerTime[m_ui32CurrentTrial]));
					}
				}

				//send CUE stimulation
				if(m_oCueDisplayStart[m_ui32CurrentTrial]>(m_ui32SentSampleCount-count) &&
						m_oCueDisplayStart[m_ui32CurrentTrial]<=m_ui32SentSampleCount
				     )
				{
					if(l_bKeepCurrentTrial)
					{
						if(m_oClassLabels[m_ui32CurrentTrial] != BCICompetitionIIIbReader_UndefinedClass)
						{
							//send class label
							l_oEvents.push_back(pair<uint64,uint64>(0x300+m_oClassLabels[m_ui32CurrentTrial], m_oCueDisplayStart[m_ui32CurrentTrial]));
						}
						else
						{
							//send true label
							l_oEvents.push_back(pair<uint64,uint64>(0x300+m_oTrueLabels[m_ui32CurrentTrial], m_oCueDisplayStart[m_ui32CurrentTrial]));
						}
					}
				}

				//send feedback start stimulation
				if(m_oFeedbackStart[m_ui32CurrentTrial]>(m_ui32SentSampleCount-count) &&
						m_oFeedbackStart[m_ui32CurrentTrial]<=m_ui32SentSampleCount
				     )
				{
					if(l_bKeepCurrentTrial)
					{
						l_oEvents.push_back(pair<uint64,uint64>(0x30D, m_oFeedbackStart[m_ui32CurrentTrial]));
					}
				}

				//send end of trial stimulation
				if(m_oEndOfTrial[m_ui32CurrentTrial]>(m_ui32SentSampleCount-count) &&
						m_oEndOfTrial[m_ui32CurrentTrial]<=m_ui32SentSampleCount
				     )
				{
					if(l_bKeepCurrentTrial)
					{
						l_oEvents.push_back(pair<uint64,uint64>(0x320, m_oEndOfTrial[m_ui32CurrentTrial]));
					}
					m_ui32CurrentTrial++;
					l_bChanged=true;
				}
			}

			if(!l_oEvents.empty() || m_bEndOfFile)
			{
				IStimulationSet* l_pStimulationSet = m_oStimulationEncoder.getInputStimulationSet();

				l_pStimulationSet->setStimulationCount(l_oEvents.size() + ((m_bEndOfFile)? 1 : 0) );

				uint64 l_ui64EventDate = 0;

				for(size_t j=0 ; j<l_oEvents.size() ; j++)
				{
					//compute date
					l_ui64EventDate = ITimeArithmetics::sampleCountToTime(m_ui32SamplingRate, l_oEvents[j].second);
					l_pStimulationSet->insertStimulation(j, l_oEvents[j].first, l_ui64EventDate, 0);
				}

				//add the ending stim
				if(m_bEndOfFile)
				{
					//compute date
					l_ui64EventDate = ITimeArithmetics::sampleCountToTime(m_ui32SamplingRate, m_ui32SentSampleCount);
					l_pStimulationSet->insertStimulation(l_oEvents.size(), 0x3FF, l_ui64EventDate, 0);
				}

				m_oStimulationEncoder.encodeBuffer();

				l_pBoxIO->markOutputAsReadyToSend(1, l_ui64StartTime, l_ui64EndTime);
			}

			return true;
		}
	};
};
