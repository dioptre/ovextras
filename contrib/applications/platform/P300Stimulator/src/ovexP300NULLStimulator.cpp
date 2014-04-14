#include "ovexP300NULLStimulator.h"
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

ExternalP300NULLStimulator::ExternalP300NULLStimulator(P300StimulatorPropertyReader* propertyObject, P300SequenceGenerator* l_pSequenceGenerator)
				: ExternalP300IStimulator(), m_oSharedMemoryReader(), m_pPropertyObject(propertyObject)
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

ExternalP300NULLStimulator::~ExternalP300NULLStimulator()
{
	m_oSharedMemoryReader.closeSharedMemory();
	#ifdef OUTPUT_TIMING
	fclose(timingFile);
	#endif
}

//*
void ExternalP300NULLStimulator::adjustForNextTrial(uint64 currentTime)
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
//*/

void ExternalP300NULLStimulator::run()
{
	uint32 l_ui32State=State_NoFlash;

	if (m_ui32TrialCount==0)
		m_ui32TrialCount = UINT_MAX-1;

	uint32 l_ui32StimulatorFrequency = 250; //TODO should be a configurable parameter
	uint64 l_ui64TimeStep = static_cast<uint64>((1LL<<32)/l_ui32StimulatorFrequency);
	uint64 l_ui64CurrentTime = 0;
	
	//m_oSharedMemoryReader


	while (m_ui32TrialIndex<=m_ui32TrialCount)
	{
		uint64 l_ui64TimeBefore = System::Time::zgetTime();
		#ifdef OUTPUT_TIMING
            fprintf(timingFile, "%f \n",float64((System::Time::zgetTime()>>22)/1024.0));
		#endif
		
		//very often one cycle of this loop does not take much time, so we just put the program to sleep until the next time step
		if (m_ui64RealCycleTime<m_ui64StimulatedCycleTime)
			System::Time::sleep(static_cast<uint32>(std::ceil(1000.0*((m_ui64StimulatedCycleTime-m_ui64RealCycleTime+l_ui64TimeStep)>>22)/1024.0)));		

		//uint64 l_ui64Prediction = m_oSharedMemoryReader.readNextPrediction();
		IStimulationSet* l_pStimSet = m_oSharedMemoryReader.readStimulation();
		IMatrix * l_pLetterProbabilities;
		l_pLetterProbabilities = m_oSharedMemoryReader.readNextSymbolProbabilities();

		if(l_pStimSet!=NULL)//&&(l_ui64Prediction!=OVA_StimulationId_Flash))
		{
			for(int i=0; i<l_pStimSet->getStimulationCount(); i++)
			{
				int l_ui64Prediction = l_pStimSet->getStimulationIdentifier(i);
				std::cout << "\n		Stimulator callback on visu for  " << int(l_ui64Prediction) <<  std::endl;
				m_funcVisualiserCallback(l_ui64Prediction);
			}
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

boolean ExternalP300NULLStimulator::checkForQuitEvent()
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
