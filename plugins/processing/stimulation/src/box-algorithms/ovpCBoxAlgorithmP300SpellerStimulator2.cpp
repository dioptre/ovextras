
//
// Generates a timeline like
// 
// ExperimentStart
//   RowStimulus1                                          // identifier for target letter 1 row
//   ColStimulus1                                          // identifier for target letter 1 column
//   RowStimulus2
//   ColStimulus2
//   ...
//   TrialStart
//     SegmentStart                                        // Display target letter if any
//     SegmentStop                                         // Hide target letter
//     VisualStimulationStart                              // Flash
//       Target/Nontarget                                  // Does this flash cover the target letter of the trial?
//     VisualStimulationStop
//     VisualStimulationStart
//       Target/NonTarget
//     VisualStimulationStop
//      ...
//   TrialStop
//   RestStart
//   RestStop
//   TrialStart
//      ...
//   TrialStop
//   ...
// ExperimentStop
//
// Which letters should be flashed is sent out as a 0/1 feature vector per flash.
//
#include "ovpCBoxAlgorithmP300SpellerStimulator2.h"

#include <list>
#include <cctype> // tolower

#include <openvibe/ovITimeArithmetics.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Stimulation;

#define _LOG_(lm, x) { lm << x; }
#define _OPTIONAL_LOG_(lm, x) /* _LOG_(lm, x); */

uint64 CBoxAlgorithmP300SpellerStimulator2::getClockFrequency(void)
{
	return 128LL<<32;
}

boolean CBoxAlgorithmP300SpellerStimulator2::initialize(void)
{
	const IBox& l_rStaticBoxContext = this->getStaticBoxContext();

	m_ui64StartStimulation      = this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));
	m_ui64RowStimulationBase    = this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));
	m_ui64ColumnStimulationBase = this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2));

	m_ui64RowCount = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
	m_ui64ColumnCount = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);

	if(m_ui64RowCount==0 || m_ui64ColumnCount==0)
	{
		OV_ERROR_K("This stimulator should at least have 1 row and 1 column (got " << m_ui64RowCount << " and " << m_ui64ColumnCount << "\n", OpenViBE::Kernel::ErrorType::BadConfig, false);
	}

	m_ui64FlashesPerTrial = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
	m_ui64TotalTrials = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);

	m_ui64TrialStartDuration = ITimeArithmetics::secondsToTime((float64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7));
	m_ui64FlashDuration = ITimeArithmetics::secondsToTime((float64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8));
	m_ui64AfterFlashDuration = ITimeArithmetics::secondsToTime((float64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 9));
	m_ui64RestDuration = ITimeArithmetics::secondsToTime((float64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 10));

	CString l_sGroupFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 11);
	CString l_sSequenceFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 12);

	m_sKeyboardConfig = ((CString)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 13)).toASCIIString();
	m_sTextToSpell = ((CString)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 14)).toASCIIString();

	// ----------------------------------------------------------------------------------------------------------------------------------------------------------

	if(m_sTextToSpell.length() > 0 && m_sTextToSpell.length() != m_ui64TotalTrials)
	{
		this->getLogManager() << LogLevel_Info << "Matching the number of trials to the length of text to spell\n";
		m_ui64TotalTrials = m_sTextToSpell.length();
	}

	m_oStimulationDecoder.initialize(*this, 0);

	m_oTimelineEncoder.initialize(*this, 0);
	m_oFlashGroupEncoder.initialize(*this, 1);
	m_oSequenceEncoder.initialize(*this, 2);

	if(l_sGroupFilename.length()>0)
	{
		m_pGroupHandler = CSV::createCSVHandler();
		m_pGroupHandler->setFormatType(CSV::EStreamType::FeatureVector);
		if(!m_pGroupHandler->openFile(l_sGroupFilename.toASCIIString(), CSV::EFileAccessMode::Read ))
		{
			OV_ERROR_K("Unable to read flash group file", OpenViBE::Kernel::ErrorType::BadFileRead, false);
		}
		std::vector<std::string> names;
		if(!m_pGroupHandler->getFeatureVectorInformation(names))
		{
			OV_ERROR_K("Unable to read flash froup number of variables", OpenViBE::Kernel::ErrorType::BadFileParsing, false);
		}
		OV_ERROR_UNLESS_K(names.size()==(m_ui64RowCount*m_ui64ColumnCount), "Invalid group vector size",  OpenViBE::Kernel::ErrorType::BadInput, false);
		m_oFlashGroupEncoder.getInputMatrix()->setDimensionCount(1);
		m_oFlashGroupEncoder.getInputMatrix()->setDimensionSize(0, names.size());
	}
	else
	{
		OV_ERROR_K("Flash group file is currently required; this is a .CSV feature vector file with row i containing a sequence of 1 and 0 indicating if a letter should be flashed or not for flash i\n", 
			OpenViBE::Kernel::ErrorType::BadConfig, false);
	}
	if(l_sSequenceFilename.length()>0)
	{
		m_pSequenceHandler = CSV::createCSVHandler();
		m_pSequenceHandler->setFormatType(CSV::EStreamType::FeatureVector);
		if(!m_pSequenceHandler->openFile(l_sSequenceFilename.toASCIIString(), CSV::EFileAccessMode::Read ))
		{
			OV_ERROR_K("Unable to read flash sequence file", OpenViBE::Kernel::ErrorType::BadFileRead, false);
		}
		std::vector<std::string> names;
		if(!m_pSequenceHandler->getFeatureVectorInformation(names))
		{
			OV_ERROR_K("Unable to read flash sequence number of variables", OpenViBE::Kernel::ErrorType::BadFileParsing, false);
		}
		OV_ERROR_UNLESS_K(names.size()==1, "Invalid sequence vector size",  OpenViBE::Kernel::ErrorType::BadInput, false);
		m_oSequenceEncoder.getInputMatrix()->setDimensionCount(1);
		m_oSequenceEncoder.getInputMatrix()->setDimensionSize(0, names.size());
	}
	else
	{
		OV_WARNING_K("No flash sequence file given; sequence information won't be sent.");
	}
	m_ui64LastTime=0;
	m_ui64NextStateTime=0;
	m_ui64TrialCount=0;
	m_ui64FlashCount=0;

	m_oNextState = State_None;

	// this->generate_sequence();
	return true;
}

boolean CBoxAlgorithmP300SpellerStimulator2::uninitialize(void)
{

	m_oStimulationDecoder.uninitialize();

	m_oTimelineEncoder.uninitialize();
	m_oFlashGroupEncoder.uninitialize();
	m_oSequenceEncoder.uninitialize();

	if(m_pGroupHandler)
	{
		m_pGroupHandler->closeFile();
		CSV::releaseCSVHandler(m_pGroupHandler);
	}
	if(m_pSequenceHandler)
	{
		m_pSequenceHandler->closeFile();
		CSV::releaseCSVHandler(m_pSequenceHandler);
	}

	return true;
}

boolean CBoxAlgorithmP300SpellerStimulator2::processInput(uint32 ui32InputIndex)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		if(m_oNextState == State_None)
		{
			m_oStimulationDecoder.decode(i,false);

			if(m_oStimulationDecoder.isBufferReceived())
			{
				const IStimulationSet* l_pSet = m_oStimulationDecoder.getOutputStimulationSet();
				for(uint32 j=0; j<l_pSet->getStimulationCount(); j++)
				{
					if(l_pSet->getStimulationIdentifier(j) == m_ui64StartStimulation)
					{
						m_oNextState = State_Experiment_Start;
						m_ui64NextStateTime=l_pSet->getStimulationDate(j);
					}
				}
			}
		}

		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}

	return true;
}

boolean CBoxAlgorithmP300SpellerStimulator2::processClock(IMessageClock& rMessageClock)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmP300SpellerStimulator2::process(void)
{
	// IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	uint64 l_ui64CurrentTime=this->getPlayerContext().getCurrentTime();

	if(l_ui64CurrentTime==0)
	{
		m_oTimelineEncoder.encodeHeader();
		m_oSequenceEncoder.encodeHeader();
		m_oFlashGroupEncoder.encodeHeader();
		l_rDynamicBoxContext.markOutputAsReadyToSend(0, 0, 0);
		l_rDynamicBoxContext.markOutputAsReadyToSend(1, 0, 0);
		l_rDynamicBoxContext.markOutputAsReadyToSend(2, 0, 0);
	}

	IStimulationSet* l_oStimulationSet = m_oTimelineEncoder.getInputStimulationSet();
	l_oStimulationSet->clear();

	if(m_ui64NextStateTime<=l_ui64CurrentTime)
	{
		if(m_oNextState == State_None) {
			// NOP
		} 
		else if(m_oNextState == State_Experiment_Start)
		{
			l_oStimulationSet->appendStimulation(OVTK_StimulationId_ExperimentStart, l_ui64CurrentTime, 0);
			_OPTIONAL_LOG_(this->getLogManager(), LogLevel_Trace << "sends OVTK_StimulationId_ExperimentStart\n");	
			sendTextToSpell(m_sTextToSpell, l_ui64CurrentTime, *l_oStimulationSet, true);

			m_oNextState = State_Trial_Start;
			m_ui64NextStateTime = l_ui64CurrentTime + ITimeArithmetics::secondsToTime(1.0);
		} 
		else if(m_oNextState == State_Trial_Start)
		{
			l_oStimulationSet->appendStimulation(OVTK_StimulationId_TrialStart, l_ui64CurrentTime, 0);
			_OPTIONAL_LOG_(this->getLogManager(), LogLevel_Trace << "sends OVTK_StimulationId_TrialStart\n");

			m_oNextState = State_Display_Target_Start;
			m_ui64NextStateTime = l_ui64CurrentTime + ITimeArithmetics::secondsToTime(0.1);
		}
		else if(m_oNextState == State_Display_Target_Start)
		{
			l_oStimulationSet->appendStimulation(OVTK_StimulationId_SegmentStart, l_ui64CurrentTime, 0);
			_OPTIONAL_LOG_(this->getLogManager(), LogLevel_Trace << "sends OVTK_StimulationId_SegmentStart\n");

			m_oNextState = State_Display_Target_End;
			m_ui64NextStateTime = l_ui64CurrentTime + m_ui64TrialStartDuration;
		}
		else if(m_oNextState == State_Display_Target_End)
		{
			l_oStimulationSet->appendStimulation(OVTK_StimulationId_SegmentStop, l_ui64CurrentTime, 0);
			_OPTIONAL_LOG_(this->getLogManager(), LogLevel_Trace << "sends OVTK_StimulationId_SegmentStop\n");

			m_oNextState = State_Flash_Start;
			m_ui64NextStateTime = l_ui64CurrentTime + ITimeArithmetics::secondsToTime(1.0);
		}
		else if(m_oNextState == State_Flash_Start)
		{
			l_oStimulationSet->appendStimulation(OVTK_StimulationId_VisualStimulationStart, l_ui64CurrentTime, 0);
			_OPTIONAL_LOG_(this->getLogManager(), LogLevel_Trace << "sends OVTK_StimulationId_VisualStimulationStart\n");
			m_ui64FlashCount++;

			sendFlashParameters(l_ui64CurrentTime, *l_oStimulationSet);

			m_oNextState = State_Flash_End;
			m_ui64NextStateTime = l_ui64CurrentTime + m_ui64FlashDuration;
		}
		else if(m_oNextState == State_Flash_End)
		{
			l_oStimulationSet->appendStimulation(OVTK_StimulationId_VisualStimulationStop, l_ui64CurrentTime, 0);
			_OPTIONAL_LOG_(this->getLogManager(), LogLevel_Trace << "sends OVTK_StimulationId_VisualStimulationStop\n");			
			if(m_ui64FlashCount % m_ui64FlashesPerTrial == 0)
			{
				m_oNextState = State_Trial_End;
			}
			else
			{
				m_oNextState = State_Flash_Start;
			}
			m_ui64NextStateTime = l_ui64CurrentTime + m_ui64AfterFlashDuration;
		}
		else if(m_oNextState == State_Trial_End)
		{
			l_oStimulationSet->appendStimulation(OVTK_StimulationId_TrialStop, l_ui64CurrentTime, 0);
			_OPTIONAL_LOG_(this->getLogManager(), LogLevel_Trace << "sends OVTK_StimulationId_TrialStop\n");			
			m_ui64TrialCount++;

			m_oNextState = State_Rest_Start;
			m_ui64NextStateTime = l_ui64CurrentTime + ITimeArithmetics::secondsToTime(0.1);
		}
		else if(m_oNextState == State_Rest_Start)
		{
			l_oStimulationSet->appendStimulation(OVTK_StimulationId_RestStart, l_ui64CurrentTime, 0);
			_OPTIONAL_LOG_(this->getLogManager(), LogLevel_Trace << "sends OVTK_StimulationId_RestStart\n");

			m_oNextState = State_Rest_End;
			m_ui64NextStateTime = l_ui64CurrentTime + m_ui64RestDuration;
		}
		else if(m_oNextState == State_Rest_End)
		{
			l_oStimulationSet->appendStimulation(OVTK_StimulationId_RestStop, l_ui64CurrentTime, 0);
			_OPTIONAL_LOG_(this->getLogManager(), LogLevel_Trace << "sends OVTK_StimulationId_RestStop\n");

			if(m_ui64TrialCount==m_ui64TotalTrials)
			{
				m_oNextState = State_Experiment_Stop;
			}
			else
			{
				m_oNextState = State_Trial_Start;
			}
			m_ui64NextStateTime = l_ui64CurrentTime + ITimeArithmetics::secondsToTime(0.1);
		}
		else if(m_oNextState == State_Experiment_Stop)
		{
			l_oStimulationSet->appendStimulation(OVTK_StimulationId_ExperimentStop, l_ui64CurrentTime, 0);
			_OPTIONAL_LOG_(this->getLogManager(), LogLevel_Trace << "sends OVTK_StimulationId_ExperimentStop\n");	

			m_oNextState = State_None;
			m_ui64NextStateTime = l_ui64CurrentTime + ITimeArithmetics::secondsToTime(3.0);
		}
		else
		{
			OV_ERROR_K("Unknown state", OpenViBE::Kernel::ErrorType::Internal, false);
		}
	}

	if(m_ui64LastTime!=l_ui64CurrentTime)
	{
		m_oTimelineEncoder.encodeBuffer();
		l_rDynamicBoxContext.markOutputAsReadyToSend(0, m_ui64LastTime, l_ui64CurrentTime);
	}
	m_ui64LastTime=l_ui64CurrentTime;

	return true;
}

bool CBoxAlgorithmP300SpellerStimulator2::sendFlashParameters(uint64 timeToSend, OpenViBE::IStimulationSet& oSet)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	if(m_pGroupHandler)
	{
		std::vector<CSV::SMatrixChunk> l_sTmp;
		std::vector<CSV::SStimulationChunk> l_sDummy;
		if(!m_pGroupHandler->readSamplesAndEventsFromFile(1, l_sTmp, l_sDummy))
		{
			return false;
		}
		IMatrix* ptr = m_oFlashGroupEncoder.getInputMatrix();
		OV_ERROR_UNLESS_K(ptr->getDimensionSize(0)==l_sTmp[0].matrix.size(), "Invalid group vector size",  OpenViBE::Kernel::ErrorType::BadInput, false);
		memcpy(ptr->getBuffer(), (void* )&(l_sTmp[0].matrix[0]), sizeof(double)*l_sTmp[0].matrix.size());
		m_oFlashGroupEncoder.encodeBuffer();
		l_rDynamicBoxContext.markOutputAsReadyToSend(1, m_ui64LastTime, timeToSend);

		if(m_sTextToSpell.length()>0)
		{
			// If we're going to send a flash that highlights the current target letter, send "target", otherwise send "nontarget"
			const uint32 currentLetter = std::tolower(m_sTextToSpell[size_t(m_ui64TrialCount)]);
			for(size_t i=0;i<m_sKeyboardConfig.length();i++)
			{
				if(std::tolower(m_sKeyboardConfig[i])==currentLetter)
				{
					const float64 active = l_sTmp[0].matrix[i];
					if(active>0)
					{
						oSet.appendStimulation( OVTK_StimulationId_Target, timeToSend,0);
					}
					else
					{
						oSet.appendStimulation( OVTK_StimulationId_NonTarget, timeToSend, 0);
					}
					break;
				}
			}
		}
	}
	if(m_pSequenceHandler)
	{
		std::vector<CSV::SMatrixChunk> l_sTmp;
		std::vector<CSV::SStimulationChunk> l_sDummy;
		if(!m_pSequenceHandler->readSamplesAndEventsFromFile(1, l_sTmp, l_sDummy))
		{
			return false;
		}
		IMatrix* ptr = m_oSequenceEncoder.getInputMatrix();
		OV_ERROR_UNLESS_K(ptr->getDimensionSize(0)==l_sTmp[0].matrix.size(), "Invalid sequence vector size",  OpenViBE::Kernel::ErrorType::BadInput, false);
		memcpy(ptr->getBuffer(), (void*)&(l_sTmp[0].matrix[0]), sizeof(double)*l_sTmp[0].matrix.size());
		m_oSequenceEncoder.encodeBuffer();
		l_rDynamicBoxContext.markOutputAsReadyToSend(2, m_ui64LastTime, timeToSend);
	}

	return true;
}

//
// If the string is the whole sequence of letters to be spelled, set bIsWholeText true. Otherwise, it means the target letter of this trial.
//
bool CBoxAlgorithmP300SpellerStimulator2::sendTextToSpell(const std::string& sString, uint64 ui64TimeToSend, IStimulationSet& oSet, bool bIsWholeText)
{
//	if(bIsWholeText)
//	{
//		oSet.appendStimulation(OVTK_StimulationId_Target, ui64TimeToSend, 0);
//	}

	for(size_t i=0;i<sString.size();i++)
	{
		bool found = false;
		for(size_t j=0;j<m_sKeyboardConfig.size();j++)
		{
			if(std::tolower(sString[i])==std::tolower(m_sKeyboardConfig[j]))
			{
				const uint64 l_ui64Row = j / m_ui64ColumnCount + m_ui64RowStimulationBase;
				const uint64 l_ui64Column = j % m_ui64ColumnCount + m_ui64ColumnStimulationBase;
				oSet.appendStimulation(l_ui64Row,    ui64TimeToSend, 0);
				oSet.appendStimulation(l_ui64Column, ui64TimeToSend, 0);
				found = true;
				break;
			}
		}
		OV_ERROR_UNLESS_K(found==true, "Letter (" << m_sTextToSpell.substr(i,1).c_str() << ") not found in keyboard config", OpenViBE::Kernel::ErrorType::BadSetting, false);
	}

//	if(bIsWholeText)
//	{
//		oSet.appendStimulation(OVTK_StimulationId_NonTarget, ui64TimeToSend, 0);
//	}

	return true;
}
