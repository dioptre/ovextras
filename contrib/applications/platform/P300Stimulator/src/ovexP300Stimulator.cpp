#include "ovexP300Stimulator.h"
#include "ova_defines.h"
#include <system/Time.h>

//#include <boost/thread.hpp>
//#include <sys/time.h>
//#include "boost/thread/thread.hpp"
//#include "boost/date_time/posix_time/posix_time.hpp"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEApplications;

using namespace std;

//struct timeval currentLTime;

#define time2ms(x,y) ((x) * 1000 + y/1000.0) + 0.5

ExternalP300Stimulator::ExternalP300Stimulator(P300StimulatorPropertyReader* propertyObject, P300SequenceGenerator* l_pSequenceGenerator) 
				: m_oSharedMemoryReader(), m_pPropertyObject(propertyObject)
{
	m_pSequenceGenerator = l_pSequenceGenerator;
	
	m_ui32RepetitionCountInTrial = m_pSequenceGenerator->getNumberOfRepetitions();
	m_ui32MinRepetitions = propertyObject->getMinNumberOfRepetitions();
	m_ui32TrialCount = propertyObject->getNumberOfTrials();
	m_ui64FlashDuration = propertyObject->getFlashDuration();
	m_ui64InterStimulusOnset = propertyObject->getInterStimulusOnset();
	m_ui64InterRepetitionDuration = propertyObject->getInterRepetitionDelay();
	m_ui64InterTrialDuration = propertyObject->getInterTrialDuration();
	m_oTargetStimuli = propertyObject->getTargetStimuli();
	m_sSharedMemoryName = propertyObject->getSharedMemoryName();
	
	m_ui64LastTime=0;
	m_bStopReceived = false;

	m_ui32LastState=State_None;
	adjustForNextTrial(0);

	m_ui32FlashCountInRepetition=m_pSequenceGenerator->getNumberOfGroups();
	m_ui64RepetitionDuration = (m_ui32FlashCountInRepetition-1)*m_ui64InterStimulusOnset+m_ui64FlashDuration;
	m_ui64TrialDuration=m_ui32RepetitionCountInTrial*(m_ui64RepetitionDuration+m_ui64InterRepetitionDuration);
	m_ui32TrialIndex=0;					

	m_oSharedMemoryReader.openSharedMemory(m_sSharedMemoryName);
	m_ui64Prediction = 0;				
	
	#ifdef OUTPUT_TIMING
	timingFile = fopen(OpenViBE::Directories::getDataDir() + "/stimulator_round_timing.txt","w");
	#endif
	
	m_ui64RealCycleTime = 0;
	m_ui64StimulatedCycleTime = 0;
}

ExternalP300Stimulator::~ExternalP300Stimulator()
{
	m_oSharedMemoryReader.closeSharedMemory();
	#ifdef OUTPUT_TIMING
	fclose(timingFile);
	#endif
}

void ExternalP300Stimulator::adjustForNextTrial(uint64 currentTime)
{
	m_ui64TrialStartTime=currentTime+m_ui64InterTrialDuration;
	uint64 l_ui64InterTrialPartition = m_ui64InterTrialDuration >> 3;
	m_ui64NextFeedbackStartTime = m_ui64TrialStartTime-m_ui64InterTrialDuration+(l_ui64InterTrialPartition<<1);
	m_ui64NextFeedbackEndTime = m_ui64NextFeedbackStartTime+(l_ui64InterTrialPartition<<1);
	m_ui64NextTargetStartTime = m_ui64NextFeedbackEndTime+l_ui64InterTrialPartition;
	m_ui64NextTargetEndTime = m_ui64NextTargetStartTime+(l_ui64InterTrialPartition<<1);
	
	m_ui64TimeToNextFlash = 0;
	m_ui64TimeToNextFlashStop = m_ui64TimeToNextFlash + m_ui64FlashDuration;

	m_ui32TrialIndex++;	
}

void ExternalP300Stimulator::run()
{
	uint32 l_ui32State=State_NoFlash;

	if (m_ui32TrialCount==0)
		m_ui32TrialCount = UINT_MAX-1;

	uint32 l_ui32StimulatorFrequency = 250; //TODO should be a configurable parameter
	uint64 l_ui64TimeStep = static_cast<uint64>((1LL<<32)/l_ui32StimulatorFrequency);
	uint64 l_ui64CurrentTime = 0;
	
	while (m_ui32TrialIndex<=m_ui32TrialCount)
	{
		uint64 l_ui64TimeBefore = System::Time::zgetTime();
		#ifdef OUTPUT_TIMING
            fprintf(timingFile, "%f \n",float64((System::Time::zgetTime()>>22)/1024.0));
		#endif
		
		//very often one cycle of this loop does not take much time, so we just put the program to sleep until the next time step
		if (m_ui64RealCycleTime<m_ui64StimulatedCycleTime)
			System::Time::sleep(static_cast<uint32>(std::ceil(1000.0*((m_ui64StimulatedCycleTime-m_ui64RealCycleTime+l_ui64TimeStep)>>22)/1024.0)));		
            
		uint64 l_ui64Prediction = m_oSharedMemoryReader.readNextPrediction();
		IMatrix * l_pLetterProbabilities;
		l_pLetterProbabilities = m_oSharedMemoryReader.readNextSymbolProbabilities();

		if (l_pLetterProbabilities!=NULL)
		{
			m_funcVisualiserCallback(OVA_StimulationId_LetterColorFeedback);
			delete l_pLetterProbabilities;
		}
			
		if (l_ui64Prediction!=0 && l_ui32State!=State_TrialRest && l_ui32State!=State_Feedback && l_ui32State!=State_Target ) //induce early stopping
		{
			this->adjustForNextTrial(l_ui64CurrentTime);
			m_pPropertyObject->getKernelContext()->getLogManager() << LogLevel_Info << "Prediction received that is different from zero, induce early stopping...\n";
		}

		//rest, target or feedback
		if(l_ui64CurrentTime<m_ui64TrialStartTime)
		{
			l_ui32State=State_TrialRest;
			if ((l_ui64CurrentTime>=m_ui64NextFeedbackStartTime) && (l_ui64CurrentTime<=m_ui64NextFeedbackEndTime))
			{
				l_ui32State=State_Feedback;
				if (m_ui32LastState==State_Feedback && l_ui64Prediction!=0)
				{
					m_pPropertyObject->getKernelContext()->getLogManager() << LogLevel_Info << "Forced prediction from OpenViBE " << l_ui64Prediction << "\n";// << (uint32)currentLTime.tv_sec << "," << (uint32)currentLTime.tv_usec << "\n";
					m_ui64Prediction=l_ui64Prediction;
					l_ui32State=State_TrialRest;
				}
			}
			//showing targets on screen
			if (l_ui64CurrentTime>=m_ui64NextTargetStartTime && l_ui64CurrentTime<=m_ui64NextTargetEndTime)
				l_ui32State=State_Target;
		}
		else if (m_ui32TrialIndex!=m_ui32TrialCount)
		{
			uint64 l_ui64CurrentTimeInTrial     =l_ui64CurrentTime-m_ui64TrialStartTime;
			uint64 l_ui64CurrentTimeInRepetition=l_ui64CurrentTimeInTrial%(m_ui64RepetitionDuration+m_ui64InterRepetitionDuration);

			if(l_ui64CurrentTimeInTrial >= m_ui64TrialDuration)
			{
				if(m_ui32TrialCount==0 || m_ui32TrialIndex<m_ui32TrialCount)
				{
					this->adjustForNextTrial(l_ui64CurrentTime);
					l_ui32State=State_TrialRest;
				}
				else
					m_pPropertyObject->getKernelContext()->getLogManager() << LogLevel_Warning << "This should not occur\n";
			}
			else
			{
				if(l_ui64CurrentTimeInRepetition >= m_ui64RepetitionDuration)
				{
					l_ui32State=State_RepetitionRest;
					m_ui64TimeToNextFlash = 0;
					m_ui64TimeToNextFlashStop = m_ui64TimeToNextFlash + m_ui64FlashDuration;					
				}
				else
				{
					if (l_ui64CurrentTimeInRepetition<m_ui64TimeToNextFlash)
						l_ui32State = State_InterFlash;
					if (l_ui64CurrentTimeInRepetition>=m_ui64TimeToNextFlash && m_ui32LastState!=State_Flash && 
					    (m_ui64TimeToNextFlash<=m_ui64RepetitionDuration-m_ui64FlashDuration))
					{
						l_ui32State = State_Flash;
						m_ui64TimeToNextFlash += m_ui64InterStimulusOnset;
						m_pPropertyObject->getKernelContext()->getLogManager() << LogLevel_Debug << "Flash at " << l_ui64CurrentTimeInRepetition << " next flash at " << m_ui64TimeToNextFlash << "\n";
					}
					if (l_ui64CurrentTimeInRepetition>=m_ui64TimeToNextFlashStop && m_ui32LastState!=State_NoFlash)
					{
						l_ui32State = State_NoFlash;
						m_ui64TimeToNextFlashStop += m_ui64InterStimulusOnset;
						m_pPropertyObject->getKernelContext()->getLogManager() << LogLevel_Debug << "Flash stop at " << l_ui64CurrentTimeInRepetition << " next flash stop at " << m_ui64TimeToNextFlashStop << "\n";
					}					
				}
			}
		}
		else
		{
			m_ui32TrialIndex++; //to exit the main while loop
			l_ui32State=State_None;
		}

		if(l_ui32State!=m_ui32LastState)
		{
			m_ui64RealCycleTime = 0;
			m_ui64StimulatedCycleTime = 0;

			switch(m_ui32LastState)
			{
				case State_Flash:
					break;

				case State_NoFlash:
					break;

				case State_RepetitionRest:
					/*if(l_ui32State!=State_TrialRest && l_ui32State!=State_None)
						m_funcVisualiserCallback(OVA_StimulationId_SegmentStart);*/
					break;

				case State_TrialRest:
					if (l_ui32State!=State_Target && l_ui32State!=State_Feedback)
					{
						m_funcVisualiserCallback(OVA_StimulationId_RestStop);
						if (l_ui32State!=State_None)
						{
							m_funcVisualiserCallback(OVA_StimulationId_TrialStart);
							//m_funcVisualiserCallback(OVA_StimulationId_SegmentStart);
						}
					}

					break;

				case State_Target:
					m_ui64Prediction = 0;
					break;

				case State_Feedback:
					break;

				case State_None:
					m_funcVisualiserCallback(OVA_StimulationId_ExperimentStart);
					break;

				default:
					break;
			}
			uint32 targetStimulus = 0;
			switch(l_ui32State)
			{
				case State_Flash:
					m_funcVisualiserCallback(OVA_StimulationId_Flash);
					break;

				case State_NoFlash:
					m_funcVisualiserCallback(OVA_StimulationId_FlashStop);
					break;

				case State_RepetitionRest:
					m_funcVisualiserCallback(OVA_StimulationId_VisualStimulationStop);
					break;

				case State_TrialRest:
					if (m_ui32LastState==State_Target || m_ui32LastState==State_Feedback)
					{
						m_funcVisualiserCallback(OVA_StimulationId_VisualStimulationStop);
					}
					if(m_ui32LastState!=State_None && m_ui32LastState!=State_Target && m_ui32LastState!=State_Feedback)
					{
						m_funcVisualiserCallback(OVA_StimulationId_TrialStop);
						m_funcVisualiserCallback(OVA_StimulationId_RestStart);
					}
					break;

				case State_Target:
					if (m_oTargetStimuli->size()!=0)
					{
						targetStimulus = static_cast<uint32>(m_oTargetStimuli->front()); //TODO: we should type all stimuli consistently as either uint64 or uint32
						m_oTargetStimuli->pop();
						m_funcVisualiserCallback(OVA_StimulationId_TargetCue);
						m_funcVisualiserCallback(targetStimulus);
					}
					break;

				case State_Feedback:
					if(m_ui64Prediction==0)
					{		
						m_pPropertyObject->getKernelContext()->getLogManager() << LogLevel_Info << "Stimulator forces feedback in OpenViBE at time\n ";// << (uint32)currentLTime.tv_sec << "," << (uint32)currentLTime.tv_usec << "\n";
						m_funcVisualiserCallback(OVA_StimulationId_FeedbackCue);
					}
					else
					{		
						m_pPropertyObject->getKernelContext()->getLogManager() << LogLevel_Info << "Received feedback: \n";// << m_ui64Prediction << " at time " << (uint32)currentLTime.tv_sec << "," << (uint32)currentLTime.tv_usec <<  "\n";
						m_funcVisualiserCallback(static_cast<uint32>(m_ui64Prediction));//TODO: we should type all stimuli consistently as either uint64 or uint32
						m_ui64Prediction=0;
					}
					break;

				case State_None:
					if(m_ui32LastState!=State_RepetitionRest && m_ui32LastState!=State_TrialRest)
						m_pPropertyObject->getKernelContext()->getLogManager() << LogLevel_Warning << "We should not reach this state\n";
					m_funcVisualiserCallback(OVA_StimulationId_TrialStop);
					break;

				default:
					break;
			}

			m_ui32LastState=l_ui32State;
		}
		
		l_ui64CurrentTime += l_ui64TimeStep;
		m_ui64StimulatedCycleTime += l_ui64TimeStep;

		//SDL_PollEvent(&m_eKeyEvent);
		//std::cout << "Stimulator waiting 0 " << std::endl;
		m_funcVisualiserWaitCallback(0);
		if(checkForQuitEvent())
			m_ui32TrialIndex = UINT_MAX;
		
		uint64 l_ui64TimeDifference = System::Time::zgetTime()-l_ui64TimeBefore;
		m_ui64RealCycleTime += l_ui64TimeDifference;

		#ifdef OUTPUT_TIMING
		fprintf(timingFile, "%f \n",float64((System::Time::zgetTime()>>22)/1024.0));	
		#endif
	}
	
	//in case it is not stopped in the middle of the stimulation process we want to wait on an event before quitting the application
	//TODO this is SDL code and should be separated from the stimulator code, a new SDL thread with the same openGL context should be created
	//to monitor for certain events
	if (m_ui32TrialIndex != UINT_MAX)
	{
		//SDL_WaitEvent(&m_eKeyEvent);
		std::cout << "Stimulator waiting " << std::endl;
		m_funcVisualiserWaitCallback(1);
		while (!checkForQuitEvent())// && SDL_WaitEvent(&m_eKeyEvent))
		{
			m_funcVisualiserWaitCallback(1);
			System::Time::sleep(50);
		}
	}
	m_funcVisualiserCallback(OVA_StimulationId_ExperimentStop);
}

boolean ExternalP300Stimulator::checkForQuitEvent()
{
	boolean l_bQuitEventReceived = false;
	/*
	switch (m_eKeyEvent.type) {
		case SDL_KEYDOWN:
			if(m_eKeyEvent.key.keysym.sym==SDLK_ESCAPE)
				l_bQuitEventReceived = true;
			break;
		case SDL_QUIT:
			l_bQuitEventReceived = true;
			break;
		default:
			break;
	}
	//*/
	l_bQuitEventReceived = m_quitevent();
	return l_bQuitEventReceived;
}
