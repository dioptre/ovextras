#ifndef __ovExternalP300Visualiser__
#define __ovExternalP300Visualiser__

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <map>
#include <list>
#include <stack>
#include <queue>

#include <cstring>
#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <cmath>

#include "ovexP300Stimulator.h"
#include "ovexITagger.h"
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

#include "GL/gl.h"
#include "GL/glu.h"
#include "SDL.h"
#include "FTGL/ftgl.h"

//#include <boost/shared_ptr.hpp>

namespace OpenViBEApplications
{
		class ExternalP300Visualiser
		{

		public:

			ExternalP300Visualiser();
			~ExternalP300Visualiser();

			void release(void) { delete this; }
			void initialize();

			void process(OpenViBE::uint32 event);
			static void processCallback(OpenViBE::uint32 event);

			void initializeOpenViBEKernel();

		private:

			void readPropertiesFromFile(OpenViBE::CString propertyFile);
			void changeStates(OpenViBE::uint32* states, VisualState ifState, VisualState elseState = NONE);

		public:
			ExternalP300Stimulator * m_oStimulator;
			
			std::queue<OpenViBE::uint32> m_qEventQueue;
			ITagger* m_pTagger;						
			
			OpenViBE::Kernel::IKernelContext* m_pKernelContext;
			
			P300InterfacePropertyReader* m_pInterfacePropReader;
			P300ScreenLayoutReader* m_pScreenLayoutReader;
			P300StimulatorPropertyReader* m_pStimulatorPropReader;
			
			P300MainContainer* m_pMainContainer;
			//boost::shared_ptr<P300KeyboardHandler> m_pP300KeyboardHandler;

		private:
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
