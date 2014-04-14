#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#ifndef __IStimulator_H__
#define __IStimulator_H__

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
#include "sequence/ovexP300SequenceGenerator.h"

//TODO get rid of the SDL dependency, should not be in the stimulator, create a separate SDL_thread that listens for key events
//#include "SDL.h"

namespace OpenViBEApplications
{		
		typedef void (*Callback2Visualiser)(OpenViBE::uint32);	
		typedef OpenViBE::boolean (*getFromVisualiser)(void);

		/**
		 * States of the stimulator (depends on the time within a trial)
		 */
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
		
		/**
		 * The stimulator class is the main loop of the program. Based on stimulated time steps it will go from one state to another
		 * (e.g. if the duration of flash is over then it will go to the noflash state). Each time it changes to another state it notifies the 
		 * main ExternalP300Visualiser class by means of a stimulation id as defined in ova_defines.h
		 */
		class ExternalP300IStimulator
		{
			public:

				/**
				 * The main loop of the program
				 */
				virtual void run()=0;
				
				/**
				 * Constructor that will create an ExternalP300SharedMemoryReader object for reading the predictions and probabilities of the letters
				 * as computed by the openvibe-designer TODO the stimulator should not handle reading from shared memory, 
				 * a separate thread should do that and then notify the the stimulator of that event
				 * @param propertyObject the object containing the properties for the stimulator such as flash duration, interflash duration, intertrial...
				 * @param l_pSequenceGenerator the sequence generator that defines which letters are flashed at one single point in time (does that for the whole trial)
				 */
				//ExternalP300IStimulator(P300StimulatorPropertyReader* propertyObject, P300SequenceGenerator* l_pSequenceGenerator)=0;
				
				ExternalP300IStimulator(){};

				virtual ~ExternalP300IStimulator(){};

				/**
				 * The callback that the stimulator will call to notify the ExternalP300Visualiser that the state has changed and the display should be updated
				 */
				virtual void setCallBack( Callback2Visualiser callback) {m_funcVisualiserCallback = callback;}


				virtual void setWaitCallBack( Callback2Visualiser callback) {m_funcVisualiserWaitCallback = callback;}
				virtual void setQuitEventCheck( getFromVisualiser callback) {m_quitevent = callback;}
				
				/**
				 * At the beginning of the the next trial, generate the whole sequence of letters that have to be flashed in the trial
				 */ 
				virtual void generateNewSequence()=0;
				
				/**
				 * @return return vector of zeros and ones defining which letters will be flashed next
				 */
				virtual std::vector<OpenViBE::uint32>* getNextFlashGroup()=0;
				
				/**
				 * @return The shared memory reader that is created during construction of the stimulator
				 */
				virtual ExternalP300SharedMemoryReader* getSharedMemoryReader() { return &m_oSharedMemoryReader; }

		private:
			/**
			 * If you stop early then it will adjust the variables such as m_ui64TrialStartTime so that the next
			 * trial can begin
			 */
			//virtual void adjustForNextTrial(OpenViBE::uint64 currentTime);

			/**
			 * Checks if the escape button is pressed TODO should be in separate SDL_thread
			 */
			virtual OpenViBE::boolean checkForQuitEvent()
			{
				OpenViBE::boolean l_bQuitEventReceived = false;
				l_bQuitEventReceived = m_quitevent();
				return l_bQuitEventReceived;
			}

				getFromVisualiser m_quitevent;
				Callback2Visualiser m_funcVisualiserCallback;
				Callback2Visualiser m_funcVisualiserWaitCallback;
				ExternalP300SharedMemoryReader m_oSharedMemoryReader;
				
		};

};
#endif
