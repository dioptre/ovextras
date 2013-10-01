#include "ovexP300Visualiser.h"

#include <system/Time.h>

//#include <sys/time.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEApplications;

using namespace std;

ExternalP300Visualiser * externalVisualiser;
OpenViBE::float64* s_pResetSymbolProbabilities;

ExternalP300Visualiser::ExternalP300Visualiser()
{
	m_bChanged = false;
	m_bInRest = true;
	m_bInTarget = false;
	m_bInFeedback = false;

	m_ui32FeedbackCueCounter = 0;
	m_ui32FeedbackResultCounter = 0;
	m_ui32PreviousFeedbackResultCounter = 0;

	#ifdef OUTPUT_TIMING
	timingFile = fopen(OpenViBE::Directories::getDataDir() + "/symbol_update_timing.txt","w");		
	timingFile3 = fopen(OpenViBE::Directories::getDataDir() + "/generate_sequence_timing.txt","w");	
	#endif
}

ExternalP300Visualiser::~ExternalP300Visualiser()
{
	std::cout << "P300Visualiser destructor\n";
	delete m_oStimulator;
	
	delete m_pTagger;
	
	//delete m_pMainContainer; //should be delete before m_pScreenLayoutReader is deleted (needed to iterate over buttons)
	
	delete m_pInterfacePropReader; 
	delete m_pScreenLayoutReader; 
	delete m_pStimulatorPropReader;

	#ifdef OUTPUT_TIMING
	fclose(timingFile);
	fclose(timingFile3);
	#endif
}

void ExternalP300Visualiser::initializeOpenViBEKernel()
{
	CKernelLoader l_oKernelLoader;

	cout<<"[  INF  ] Created kernel loader, trying to load kernel module"<<"\n";
	CString m_sError;
	#if defined TARGET_OS_Windows
	if(!l_oKernelLoader.load(OpenViBE::Directories::getLibDir() + "/openvibe-kernel.dll", &m_sError))
	#elif defined TARGET_OS_Linux
	if(!l_oKernelLoader.load(OpenViBE::Directories::getLibDir() + "/libopenvibe-kernel.so", &m_sError))
	#endif
	{
			cout<<"[ FAILED ] Error loading kernel ("<<m_sError<<")"<<"\n";
	}
	else
	{
		cout<<"[  INF  ] Kernel module loaded, trying to get kernel descriptor"<<"\n";
		IKernelDesc* l_pKernelDesc=NULL;
		m_pKernelContext=NULL;
		l_oKernelLoader.initialize();
		l_oKernelLoader.getKernelDesc(l_pKernelDesc);
		if(!l_pKernelDesc)
		{
			cout<<"[ FAILED ] No kernel descriptor"<<"\n";
		}
		else
		{
			cout<<"[  INF  ] Got kernel descriptor, trying to create kernel"<<"\n";
			m_pKernelContext=l_pKernelDesc->createKernel("externalP300Stimulator", OpenViBE::Directories::getDataDir() + "/kernel/openvibe.conf");
			if(!m_pKernelContext)
			{
				cout<<"[ FAILED ] No kernel created by kernel descriptor"<<"\n";
			}
			else
			{
				OpenViBEToolkit::initialize(*m_pKernelContext);
			}
		}
	}
}

void ExternalP300Visualiser::processCallback(OpenViBE::uint32 eventID) 
{
	externalVisualiser->process(eventID);				
}

void ExternalP300Visualiser::process(uint32 eventID)
{
	m_bChanged = false;
	//std::vector<uint32>* l_lSymbolChangeList;
	uint32* l_lSymbolChangeList;					
	IMatrix* l_pLetterProbabilities;
	
	#ifdef OUTPUT_TIMING
	float64 l_f64TimeBefore;
	float64 l_f64TimeAfter;
	#endif
	
      switch(eventID)
	{
		case 0:
			m_pKernelContext->getLogManager() << LogLevel_Warning << "Something bad happened, probably during prediction nothing was received\n";
			break;
		case OVA_StimulationId_ExperimentStart:
			m_pMainContainer->getKeyboardHandler()->resetChildStates();
			//m_pMainContainer->setChanged(true);
			m_bChanged = true;
			m_qEventQueue.push(eventID);		
			break;
		case OVA_StimulationId_TrialStart:
			m_pTagger->write(eventID);
			break;
		case OVA_StimulationId_SegmentStart:
			break;
		case OVA_StimulationId_VisualStimulationStop:			
			m_pMainContainer->getKeyboardHandler()->resetChildStates();
			//m_pMainContainer->setChanged(true);
			m_bChanged = true;
			m_qEventQueue.push(eventID);
			break;
		case OVA_StimulationId_RestStop:
			#ifdef OUTPUT_TIMING
			l_f64TimeBefore = float64((System::Time::zgetTime()>>22)/1024.0);
			fprintf(timingFile3, "%f \n",l_f64TimeBefore);
			#endif
			
			m_oStimulator->generateNewSequence();
			
			#ifdef OUTPUT_TIMING
			l_f64TimeAfter = float64((System::Time::zgetTime()>>22)/1024.0);
			fprintf(timingFile3, "%f \n",l_f64TimeAfter);
			#endif
			
			m_bInRest = false;
			m_bInTarget = false;
			break;
		case OVA_StimulationId_ExperimentStop:
			m_pTagger->write(eventID);
			break;
		case OVA_StimulationId_TrialStop:	
			m_pTagger->write(eventID);
			break;
		case OVA_StimulationId_SegmentStop:	
			break;
		case OVA_StimulationId_RestStart:
			m_pTagger->write(eventID);
			m_bInRest = true;		
			m_pMainContainer->getKeyboardHandler()->updateChildProbabilities(s_pResetSymbolProbabilities);
			break;
		case OVA_StimulationId_TargetCue:
			m_bInFeedback = false;
			m_bInTarget = true;
			m_pTagger->write(eventID);
			break;
		case OVA_StimulationId_FeedbackCue:
			m_ui32FeedbackCueCounter++;
			//std::cout << "FeedbackCueCounter " << m_ui32FeedbackCueCounter << "\n";
			m_bInFeedback = true;
			m_pTagger->write(eventID);
			break;
		case OVA_StimulationId_LetterColorFeedback:
			if (!m_bInRest)
			{
				l_pLetterProbabilities = m_oStimulator->getSharedMemoryReader()->readNextSymbolProbabilities();
				m_oStimulator->getSharedMemoryReader()->clearSymbolProbabilities();
				if (l_pLetterProbabilities!=NULL)
				{
					m_pMainContainer->getKeyboardHandler()->updateChildProbabilities(l_pLetterProbabilities->getBuffer());
					delete l_pLetterProbabilities;
				}
			}
			break;
		case OVA_StimulationId_FlashStop:
			/*TODO: should use the screen layout property reader to find out the background color
			if (m_pInterfacePropReader->isPhotoDiodeEnabled())
				m_pMainContainer->changeBackgroundColorDiodeArea(m_pInterfacePropReader->getNoFlashBackgroundColor());
			*/
		
			m_pMainContainer->getKeyboardHandler()->resetMostActiveChildStates();
			m_bChanged = true;
			m_qEventQueue.push(eventID);			
			break;
		case OVA_StimulationId_Flash:
			/* TODO: should use the screen layout property reader to find out the foreground color
			 * if (m_pInterfacePropReader->isPhotoDiodeEnabled())
				m_pMainContainer->changeBackgroundColorDiodeArea(m_pInterfacePropReader->getFlashForegroundColor());*/
			
			m_qEventQueue.push(eventID);
			
			l_lSymbolChangeList = m_oStimulator->getNextFlashGroup()->data();
			changeStates(l_lSymbolChangeList,FLASH);
			m_pMainContainer->getKeyboardHandler()->updateChildStates(l_lSymbolChangeList);	
					
			m_bChanged = true;
			
			//flagging sequence stimuli as either target or non target
			if (m_bChanged && m_pInterfacePropReader->getSpellingMode()!=FREE_MODE)
			{
				if(l_lSymbolChangeList[m_bTargetId]==1)	
					m_qEventQueue.push(OVA_StimulationId_Target);
				else
					m_qEventQueue.push(OVA_StimulationId_NonTarget);
			}		
			//if (m_bChanged && m_pInterfacePropReader->getSpellingMode()==FREE_MODE)
			//	m_qEventQueue.push(OVA_StimulationId_NonTarget);
			break;
		default:
			eventID--;
			m_qEventQueue.push(eventID);
			l_lSymbolChangeList = new uint32[m_pScreenLayoutReader->getNumberOfKeys()]();
			if (m_bInFeedback)
			{
				m_ui32FeedbackResultCounter++;
				//std::cout << "FeedbackResultCounter " << m_ui32FeedbackResultCounter << "\n";
				m_pKernelContext->getLogManager() << LogLevel_Info << "Feedback received, eventID " << eventID << "\n";
				if(m_pInterfacePropReader->getSpellingMode()==FREE_MODE)
				{
					l_lSymbolChangeList[eventID] = 1;
					
					if(m_pInterfacePropReader->getCentralFeedbackFreeMode())
						changeStates(l_lSymbolChangeList,CENTRAL_FEEDBACK_CORRECT,NOFLASH);						
					else
						changeStates(l_lSymbolChangeList,NONCENTRAL_FEEDBACK_CORRECT,NOFLASH);
					
					m_bChanged = true;
				}
				else if(m_pInterfacePropReader->getSpellingMode()==COPY_MODE)
				{	
					if(m_pInterfacePropReader->getCentralFeedbackCopyMode())	
					{
						l_lSymbolChangeList[eventID] = 1;
																				
						if(eventID==m_bTargetId)
							changeStates(l_lSymbolChangeList,CENTRAL_FEEDBACK_CORRECT,NOFLASH);							
						else
							changeStates(l_lSymbolChangeList,CENTRAL_FEEDBACK_WRONG,NOFLASH);							
					}
					else
					{
						l_lSymbolChangeList[m_bTargetId] = 1;
													
						if(eventID==m_bTargetId)
							changeStates(l_lSymbolChangeList,NONCENTRAL_FEEDBACK_CORRECT,NOFLASH);	
						else
						{
							changeStates(l_lSymbolChangeList,NONCENTRAL_FEEDBACK_WRONG,NOFLASH);
							l_lSymbolChangeList[eventID]=NONCENTRAL_FEEDBACK_WRONG_SELECTED;	
						}		
					}
					m_bChanged = true;
				}
				m_bInFeedback = false;
			}
			else if(m_bInTarget && m_pInterfacePropReader->getSpellingMode()!=FREE_MODE) 
			{
				l_lSymbolChangeList[eventID] = 1;	
				changeStates(l_lSymbolChangeList,TARGET,NOFLASH);
				
				m_bTargetId = eventID;
				m_bChanged = true;
				m_bInTarget = false;
			}
			
			m_pMainContainer->getKeyboardHandler()->updateChildStates(l_lSymbolChangeList);
			delete l_lSymbolChangeList;
			
			break;
	}
	
	if (m_bChanged)
	{
		#ifdef OUTPUT_TIMING
            l_f64TimeBefore = float64((System::Time::zgetTime()>>22)/1024.0);
            fprintf(timingFile, "%f \n",l_f64TimeBefore);
		#endif

		m_pMainContainer->getKeyboardHandler()->updateChildProperties();

		#ifdef OUTPUT_TIMING
            l_f64TimeAfter = float64((System::Time::zgetTime()>>22)/1024.0);
            fprintf(timingFile, "%f \n",l_f64TimeAfter);
		#endif
		
		//send a stimulus to openvibe as to update the model
		if (m_ui32FeedbackResultCounter!= 0 && 
			m_ui32PreviousFeedbackResultCounter!=m_ui32FeedbackResultCounter && 
			m_ui32FeedbackResultCounter%5 == 0) //TODO: should  be a configurable parameter
		{
			m_ui32PreviousFeedbackResultCounter = m_ui32FeedbackResultCounter;
			m_qEventQueue.push(OVA_StimulationId_UpdateModel);
			m_pKernelContext->getLogManager() << LogLevel_Info << "Sending stimulus to OpenViBE to update model\n";
		}
            m_pMainContainer->drawAndSync(m_pTagger,m_qEventQueue);	
	}
}

void ExternalP300Visualiser::changeStates(uint32* states, VisualState ifState, VisualState elseState)
{
	for(uint32 it=0; it<m_pScreenLayoutReader->getNumberOfKeys() ; it++)
	{
		if (states[it]==1)
			states[it] = ifState;
		else
			states[it] = elseState;
	}	
}

/*
MAIN
*/

int main (int argc, char *argv[])
{	
	externalVisualiser = new ExternalP300Visualiser();
	externalVisualiser->initializeOpenViBEKernel();
	
	externalVisualiser->m_pInterfacePropReader = new P300InterfacePropertyReader(externalVisualiser->m_pKernelContext);
	externalVisualiser->m_pInterfacePropReader->readPropertiesFromFile(OpenViBE::Directories::getDistRootDir() + "/share/openvibe/applications/externalP300Stimulator/interface-properties.xml");
	externalVisualiser->m_pScreenLayoutReader = new P300ScreenLayoutReader(externalVisualiser->m_pKernelContext);
	externalVisualiser->m_pScreenLayoutReader->readPropertiesFromFile(externalVisualiser->m_pInterfacePropReader->getScreenDefinitionFile());
	externalVisualiser->m_pStimulatorPropReader = new P300StimulatorPropertyReader(externalVisualiser->m_pKernelContext, externalVisualiser->m_pScreenLayoutReader->getSymbolList());
	externalVisualiser->m_pStimulatorPropReader->readPropertiesFromFile(externalVisualiser->m_pInterfacePropReader->getStimulatorConfigFile());	
	
	if (externalVisualiser->m_pInterfacePropReader->getHardwareTagging())
	{
		externalVisualiser->m_pTagger = new ParallelPort(externalVisualiser->m_pInterfacePropReader->getParallelPortNumber(),
									externalVisualiser->m_pInterfacePropReader->getSampleFrequency());
		if (externalVisualiser->m_pTagger->open())
			externalVisualiser->m_pKernelContext->getLogManager() << LogLevel_Info << "Opened parallel port\n";
	}
	else
	{
		try
		{
			externalVisualiser->m_pTagger = new CSoftTagger();
		}
		catch (exception& e)
		{
			externalVisualiser->m_pKernelContext->getLogManager() << LogLevel_Info << "Opening software tagger failed: " << e.what() << "\n";
		}
		if (externalVisualiser->m_pTagger->open())
			externalVisualiser->m_pKernelContext->getLogManager() << LogLevel_Info << "Opened software tagger\n";
	}		
	
	P300SequenceWriter* l_pSequenceWriter = new P300SequenceFileWriter(externalVisualiser->m_pInterfacePropReader->getFlashGroupDefinitionFile()); 
	P300SequenceGenerator* l_pSequenceGenerator;
	if (externalVisualiser->m_pInterfacePropReader->getFlashMode()==CString("rowcol"))
		l_pSequenceGenerator = new P300RowColumnSequenceGenerator(
			externalVisualiser->m_pScreenLayoutReader->getNumberOfKeys(), 
			externalVisualiser->m_pStimulatorPropReader->getNumberOfGroups(), 
			externalVisualiser->m_pStimulatorPropReader->getNumberOfRepetitions());		
	else
		l_pSequenceGenerator = new P300RipRandSequenceGenerator(
			externalVisualiser->m_pScreenLayoutReader->getNumberOfKeys(), 
			externalVisualiser->m_pStimulatorPropReader->getNumberOfGroups(), 
			externalVisualiser->m_pStimulatorPropReader->getNumberOfRepetitions());
	l_pSequenceGenerator->setSequenceWriter(l_pSequenceWriter);
	
	externalVisualiser->m_oStimulator = new ExternalP300Stimulator(externalVisualiser->m_pStimulatorPropReader, l_pSequenceGenerator); 
	externalVisualiser->m_oStimulator->setCallBack(ExternalP300Visualiser::processCallback);	

	P300MainContainer::initializeGL(externalVisualiser->m_pInterfacePropReader->getFullScreen(),
						externalVisualiser->m_pInterfacePropReader->getWidth(),
						externalVisualiser->m_pInterfacePropReader->getHeight());
	externalVisualiser->m_pMainContainer = new P300MainContainer(externalVisualiser->m_pInterfacePropReader, externalVisualiser->m_pScreenLayoutReader);
	//externalVisualiser->m_pMainContainer->initialize(externalVisualiser->m_pScreenLayoutReader->getNumberOfStandardKeys());
	//externalVisualiser->m_pMainContainer->addSymbolsToGrid(externalVisualiser->m_pScreenLayoutReader->getSymbolList());
	
	s_pResetSymbolProbabilities = new float64[externalVisualiser->m_pScreenLayoutReader->getNumberOfKeys()];
	for (uint32 i=0;i<externalVisualiser->m_pScreenLayoutReader->getNumberOfKeys();i++)
	{
		s_pResetSymbolProbabilities[i] = 1.0/externalVisualiser->m_pScreenLayoutReader->getNumberOfKeys();
		//std::cout << " " << s_pResetSymbolProbabilities[i];
	}
	//std::cout << "\n";
	/*externalVisualiser->m_pP300KeyboardHandler = boost::shared_ptr<P300KeyboardHandler>(
		new P300KeyboardHandler(externalVisualiser->m_pMainContainer->getKeyboardObject(), 
						externalVisualiser->m_pInterfacePropReader, 
						externalVisualiser->m_pScreenLayoutReader));
	externalVisualiser->m_pP300KeyboardHandler->addObserver(externalVisualiser->m_pP300KeyboardHandler);*/
	//externalVisualiser->m_pP300KeyboardHandler->initializeKeyboard();
						
	externalVisualiser->m_pMainContainer->drawAndSync(externalVisualiser->m_pTagger,externalVisualiser->m_qEventQueue);	
	
	SDL_Event event;
	boolean l_bEventReceived = false;
	while (!l_bEventReceived && SDL_WaitEvent(&event)) {
		switch (event.type) {
			case SDL_KEYDOWN:
				if(event.key.keysym.sym==SDLK_s)
				{
					l_bEventReceived = true;
					externalVisualiser->m_oStimulator->run();
				}
				break;
			default:
				break;
		}
		System::Time::sleep(10);
	}
	
	delete l_pSequenceGenerator;
	delete l_pSequenceWriter;
	delete externalVisualiser;
	delete[] s_pResetSymbolProbabilities;
	SDL_Quit();

	return 0;
}
