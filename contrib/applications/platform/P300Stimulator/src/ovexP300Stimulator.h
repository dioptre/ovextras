#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <map>
#include <queue>
#include <vector>

#include <cstring>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "ovexP300SharedMemoryReader.h"
#include "properties/ovexP300StimulatorPropertyReader.h"
#include "ovexP300SequenceGenerator.h"
//#include "ovexP300SequenceGenerator.h"

#include "SDL.h"

namespace OpenViBEApplications
{		
		typedef void (*Callback2Visualiser)(OpenViBE::uint32);	

		enum
		{
			State_None,
			State_Flash,
			State_NoFlash,
			State_RepetitionRest,
			State_TrialRest,
			State_Target,
			State_Feedback,
			State_InterFlash,
		};		
		
		class ExternalP300Stimulator
		{
			public:

				virtual void run();
				ExternalP300Stimulator(P300StimulatorPropertyReader* propertyObject, P300SequenceGenerator* l_pSequenceGenerator);
				~ExternalP300Stimulator();

				void setCallBack( Callback2Visualiser callback) { m_funcVisualiserCallback = callback; }
				void generateNewSequence() { m_pSequenceGenerator->generateSequence(); }
				std::vector<OpenViBE::uint32>* getNextFlashGroup() { return m_pSequenceGenerator->getNextFlashGroup(); }
				ExternalP300SharedMemoryReader* getSharedMemoryReader() { return &m_oSharedMemoryReader; }
				
			private:
				virtual void adjustForNextTrial(OpenViBE::uint64 currentTime);
				virtual OpenViBE::boolean checkForQuitEvent();

			protected:
				OpenViBE::uint64 m_ui64StartStimulation;
				OpenViBE::uint32 m_ui32RepetitionCountInTrial;
				OpenViBE::uint32 m_ui32MinRepetitions;
				OpenViBE::uint32 m_ui32TrialCount;
				OpenViBE::uint64 m_ui64FlashDuration;
				OpenViBE::uint64 m_ui64InterRepetitionDuration;
				OpenViBE::uint64 m_ui64InterTrialDuration;
				OpenViBE::uint64 m_ui64InterStimulusOnset;
				OpenViBE::uint64 m_ui64TimeToNextFlash;
				OpenViBE::uint64 m_ui64TimeToNextFlashStop;
				OpenViBE::CString m_sTargetWord;

				OpenViBE::CString m_sSharedMemoryName;
				ExternalP300SharedMemoryReader m_oSharedMemoryReader;
				P300SequenceGenerator* m_pSequenceGenerator;
				P300StimulatorPropertyReader* m_pPropertyObject;

			private:

				OpenViBE::uint64 m_ui64LastTime;
				OpenViBE::boolean m_bStopReceived;

				OpenViBE::uint32 m_ui32LastState;
				OpenViBE::uint64 m_ui64TrialStartTime;
				OpenViBE::uint64 m_ui64NextTargetStartTime;
				OpenViBE::uint64 m_ui64NextTargetEndTime;
				OpenViBE::uint64 m_ui64NextFeedbackStartTime;
				OpenViBE::uint64 m_ui64NextFeedbackEndTime;

				OpenViBE::uint32 m_ui32FlashCountInRepetition;
				OpenViBE::uint64 m_ui64RepetitionDuration;
				OpenViBE::uint64 m_ui64TrialDuration;
				OpenViBE::uint32 m_ui32TrialIndex;

				Callback2Visualiser m_funcVisualiserCallback;

				std::map < OpenViBE::uint64, OpenViBE::uint64 > m_vRow;
				std::map < OpenViBE::uint64, OpenViBE::uint64 > m_vColumn;

				OpenViBE::uint64 m_ui64Prediction;
				std::queue< OpenViBE::uint64 >* m_oTargetStimuli;
				
				SDL_Event m_eKeyEvent;

				#ifdef OUTPUT_TIMING
                        FILE* timingFile;
				#endif
				
				OpenViBE::uint64 m_ui64StimulatedCycleTime;
				OpenViBE::uint64 m_ui64RealCycleTime;
		};

};
