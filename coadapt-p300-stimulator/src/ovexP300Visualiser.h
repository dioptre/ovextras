#ifndef __ovExternalP300Visualiser__
#define __ovExternalP300Visualiser__

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <queue>

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "ovexP300Stimulator.h"
#include "ovexCSoftTagger.h"
#include "ovexParallelPort.h"
#include "visualisation/glP300MainContainer.h"
#include "properties/ovexP300InterfacePropertyReader.h"
#include "properties/ovexP300StimulatorPropertyReader.h"
#include "properties/ovexP300ScreenLayoutReader.h"
#include "ovexP300RipRandSequenceGenerator.h"
#include "ovexP300RowColumnSequenceGenerator.h"
#include "ovexP300SequenceFileWriter.h"

#include "ova_defines.h"

namespace OpenViBEApplications
{
		/**
		* The implementation file includes the main function (press 's' to start the stimulator). 
		* The class itself will receive events from the stimulator ExternalP300Stimulator through the processCallback function.
		* Based on those events/stimuli it will make the appropriate calls for displaying the flashes. 
		*/
		class ExternalP300Visualiser
		{

		public:

			/**
			 * Constructor initializes some private working variables
			 */
			ExternalP300Visualiser();
			
			/**
			 * Deconstructor deletes the stimulator object, the tagger object and all the property readers
			 */
			~ExternalP300Visualiser();

			/**
			 * This function is called by processCallback. Based on the event received by the stimulator it will call the necessary functions.
			 * @param event the stimulus or event number. For a list of the stimuli you can take a look in the ova_defines.h file.
			 */
			void process(OpenViBE::uint32 event);
			
			/**
			 * This is the callback function called by the stimulator ExternalP300Stimulator in case of an event. It calls the process function 
			 * @param event the stimulus or event number. For a list of the stimuli you can take a look in the ova_defines.h file.
			 */
			static void processCallback(OpenViBE::uint32 event);

			/**
			 * Initializes the OpenViBE kernel so that we can use some facilities such as the LogManager
			 */
			void initializeOpenViBEKernel();
			
			/**
			 * 
			 */
			void start();
			
			/**
			 * 
			 */
			OpenViBE::uint32 getNumberOfKeys() { return m_pScreenLayoutReader->getNumberOfKeys(); }

		private:
			
			/**
			 * Iterates over states and changes the value at each index either to ifState or elseState depending on the current value (either one or zero) 
			 * @param states contains either a zero or one for each symbol on the keyboard
			 * @param ifState in case states[i]==1 it will replace the state to ifState
			 * @param elseState in case states[i]==1 it will replace the state to elseState
			 */
			void changeStates(OpenViBE::uint32* states, VisualState ifState, VisualState elseState = NONE);

		protected:
			/**
			 * The stimulator object that generates the stimuli/events given a certain timing
			 */
			ExternalP300Stimulator * m_oStimulator;
			
			/**
			 * The tagger to write the stimuli either to OpenViBE's acquisition server or to the parallel port
			 */
			ITagger* m_pTagger;						
			
			/**
			 * OpenViBE's kernel context
			 */
			OpenViBE::Kernel::IKernelContext* m_pKernelContext;
			
			/**
			 * ExternalP300PropertyReader that reads the interface-properties file in share/openvibe/applications/externalP300Stimulator/
			 */
			P300InterfacePropertyReader* m_pInterfacePropReader;
			
			/**
			 * ExternalP300PropertyReader that reads one of the keyboard layout files such as 5by10grid-abc-gray.xml in share/openvibe/applications/externalP300Stimulator/
			 */
			P300ScreenLayoutReader* m_pScreenLayoutReader;
			
			/**
			 * ExternalP300PropertyReader that reads the stimulator-properties file in share/openvibe/applications/externalP300Stimulator/
			 */
			P300StimulatorPropertyReader* m_pStimulatorPropReader;
			
			/**
			 * The main container that will initialize the OpenGL context, the main regions of the P300 keyboard and attach handlers/listeners to them
			 */
			P300MainContainer* m_pMainContainer;
			
			/**
			 * 
			 */
			P300SequenceWriter* m_pSequenceWriter;
			
			/**
			 * 
			 */
			P300SequenceGenerator* m_pSequenceGenerator;

		private:
			/**
			 * A queue with events that should be written by the ITagger
			 */
			std::queue<OpenViBE::uint32> m_qEventQueue;			
			
			OpenViBE::boolean m_bChanged;
			OpenViBE::boolean m_bInRest;
			OpenViBE::boolean m_bInFeedback;
			OpenViBE::boolean m_bInTarget;
			OpenViBE::uint32 m_bTargetId;	
			
			OpenViBE::uint32 m_ui32FeedbackCueCounter;
			OpenViBE::uint32 m_ui32FeedbackResultCounter;
			OpenViBE::uint32 m_ui32PreviousFeedbackResultCounter;

			#ifdef OUTPUT_TIMING
            FILE* timingFile;
			FILE* timingFile3;
			#endif
		};
};

#endif
