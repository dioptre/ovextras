
#include <openvibe/ovITimeArithmetics.h>

#include "ovkCSchedulerMulticore.h"
#include "ovkCSimulatedBox.h"
#include "ovkCPlayer.h"
#include "ovkCBoxSettingModifierVisitor.h"

#include <system/ovCTime.h>

#include <xml/IReader.h>

#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <limits>

//___________________________________________________________________//
//                                                                   //

using namespace std;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

//___________________________________________________________________//
//                                                                   //

static uint32 g_ui32NumThreads = 4;


boolean parallelExecutor::initialize(const uint32 nThreads)
{
	// Basically jobContext is the stuff that the scheduler and parallel executor use to communicate the status of the jobs
	m_oJobContext.m_bQuit = false;
	m_oJobContext.m_ui32JobsPending = 0;
	m_oJobContext.m_vJobList.clear();

	for(uint32 i=0;i<nThreads;i++)
	{
		m_vWorkerThread.push_back(new CWorkerThread(m_oJobContext));
		m_vThread.push_back(new boost::thread(boost::bind(&CWorkerThread::startWorkerThread, m_vWorkerThread[i])));
	}

	return true;
}

boolean parallelExecutor::uninitialize() 
{
	// Make sure all jobs have been completed
	waitForAll();

	// Tell the threads waiting in the cond its time to quit
	{ // scope for lock
		boost::unique_lock<boost::mutex> lock(m_oJobContext.m_oJobMutex);	
		m_oJobContext.m_bQuit = true;
	}

	m_oJobContext.m_oHaveWork.notify_all();

	for(uint32 i=0;i<m_vThread.size();i++)
	{
		m_vThread[i]->join();

		delete m_vThread[i];
		delete m_vWorkerThread[i];
	}

	m_vThread.clear();
	m_vWorkerThread.clear();

	return true;
}

boolean parallelExecutor::push(const jobCall& someJob)
{
	// @fixme to add better concurrency, push a list instead; lock();add list;unlock();notify_all();
	{ // lock scope
		boost::lock_guard<boost::mutex> lock(m_oJobContext.m_oJobMutex);

		m_oJobContext.m_vJobList.push_back(someJob);

		m_oJobContext.m_ui32JobsPending++;
	}

	m_oJobContext.m_oHaveWork.notify_one();


	return true;
}


boolean parallelExecutor::pushList(const std::deque<jobCall>& vJobList)
{
	{ // lock scope
		boost::lock_guard<boost::mutex> lock(m_oJobContext.m_oJobMutex);

		if(m_oJobContext.m_ui32JobsPending) {
			std::cout << "Error, trying to push list with old jobs pending\n";
			return false;
		}

		m_oJobContext.m_vJobList = vJobList;

		m_oJobContext.m_ui32JobsPending = vJobList.size();
	}

	m_oJobContext.m_oHaveWork.notify_all();


	return true;
}

boolean parallelExecutor::waitForAll()
{
	{ // lock scope
		boost::unique_lock<boost::mutex> lock(m_oJobContext.m_oJobMutex);
		while(m_oJobContext.m_ui32JobsPending>0) {
			m_oJobContext.m_oJobDone.wait(lock); 
		}
	}

	return true;
}


//___________________________________________________________________//
//                                                                   //

boolean testFunction(void* data) {
	for(uint32 i=0;i<10;i++) {
		std::cout << "Fun: " << *(uint32*)(data) << "\n";
		System::Time::sleep(*(uint32*)data);
	}
	return true;
}

CSchedulerMulticore::CSchedulerMulticore(const IKernelContext& rKernelContext, CPlayer& rPlayer)
	:	CScheduler(rKernelContext, rPlayer)
	,m_rPlayer(rPlayer)
	,m_oScenarioIdentifier(OV_UndefinedIdentifier)
	,m_pScenario(NULL)
	,m_ui64Steps(0)
	,m_ui64Frequency(0)
	,m_ui64CurrentTime(0)
	,m_bIsInitialized(false)
{
 	int stuff[6] = {500,666,50,1000,300,100};

	m_oExecutor.initialize(g_ui32NumThreads);

	// m_oExecutor.push(boost::bind(CSchedulerMulticore::job, this));
	/*
	m_oExecutor.push(boost::bind(testFunction, &stuff[0]));

//	test.push(jobInfo(boost::bind(&CSchedulerMulticore::job, this)));

//		boost::bind(&CWorkerThread::startWorkerThread, m_vWorkerThread[i])));
*/

	/*
	parallelExecutor test;
	test.initialize(3);
	test.push(boost::bind(testFunction, &stuff[0]));
	test.waitForAll();
	test.push(boost::bind(testFunction, &stuff[1]));
	test.push(boost::bind(testFunction, &stuff[2]));
	test.push(boost::bind(testFunction, &stuff[3]));
	test.push(boost::bind(testFunction, &stuff[4]));
	test.push(boost::bind(testFunction, &stuff[2]));
	test.waitForAll();
	test.waitForAll();
	test.uninitialize();
	*/

}

CSchedulerMulticore::~CSchedulerMulticore(void)
{
	m_oExecutor.uninitialize();

	if(m_bIsInitialized)
	{
		this->uninitialize();
	}
}

//___________________________________________________________________//
//                                                                   //

boolean CSchedulerMulticore::setScenario(
	const CIdentifier& rScenarioIdentifier)
{
	this->getLogManager() << LogLevel_Trace << "Scheduler setScenario\n";

	if(m_bIsInitialized)
	{
		this->getLogManager() << LogLevel_Warning << "Trying to configure an intialized scheduler !\n";
		return false;
	}

	m_oScenarioIdentifier=rScenarioIdentifier;
	m_pScenario=NULL;
	return true;
}

boolean CSchedulerMulticore::setFrequency(
	const uint64 ui64Frequency)
{
	this->getLogManager() << LogLevel_Trace << "Scheduler setFrequency\n";

	if(m_bIsInitialized)
	{
		this->getLogManager() << LogLevel_Warning << "Trying to configure an intialized scheduler !\n";
		return false;
	}

	m_ui64Frequency=ui64Frequency;
	return true;
}

//___________________________________________________________________//
//                                                                   //

SchedulerInitializationCode CSchedulerMulticore::initialize(void)
{
	this->getLogManager() << LogLevel_Trace << "Scheduler initialize\n";

	if(m_bIsInitialized)
	{
		this->getLogManager() << LogLevel_Warning << "Trying to initialize an intialized scheduler !\n";
		return SchedulerInitialization_Failed;
	}

	m_pScenario=&getScenarioManager().getScenario(m_oScenarioIdentifier);
	if(!m_pScenario)
	{
		this->getLogManager() << LogLevel_ImportantWarning << "Scenario " << m_oScenarioIdentifier << " does not exist !\n";
		return SchedulerInitialization_Failed;
	}

	CBoxSettingModifierVisitor l_oBoxSettingModifierVisitor(&getKernelContext().getConfigurationManager());
	if(!m_pScenario->acceptVisitor(l_oBoxSettingModifierVisitor)) 
	{
		this->getLogManager() << LogLevel_Error << "Scenario " << m_oScenarioIdentifier << " setting modification with acceptVisitor() failed\n";
		return SchedulerInitialization_Failed;
	}

	CIdentifier l_oBoxIdentifier;
	while((l_oBoxIdentifier=m_pScenario->getNextBoxIdentifier(l_oBoxIdentifier))!=OV_UndefinedIdentifier)
	{
		const IBox* l_pBox=m_pScenario->getBoxDetails(l_oBoxIdentifier);

		int l_iPriority = 0;
		if(l_pBox->hasAttribute(OV_AttributeId_Box_Priority)) 
		{
			// This is mostly retained for debugging use. The value can be entered to .xml by hand but there is no GUI to change this (on purpose)
			::sscanf(l_pBox->getAttributeValue(OV_AttributeId_Box_Priority).toASCIIString(), "%i", &l_iPriority);
		}
		else 
		{
			// Decide the priority based on the location of the box in the GUI. Priority decreases in top->bottom, left->right order.

			int l_iYPosition = 0;
			::sscanf(l_pBox->getAttributeValue(OV_AttributeId_Box_YCenterPosition).toASCIIString(), "%i", &l_iYPosition);
			int l_iXPosition = 0;
			::sscanf(l_pBox->getAttributeValue(OV_AttributeId_Box_XCenterPosition).toASCIIString(), "%i", &l_iXPosition);

			this->getLogManager() << LogLevel_Debug << " Inserting box " << l_pBox->getName() << " with coords (x=" << l_iXPosition << ",y=" << l_iYPosition << ")\n";

			const int32 l_iMaxInt16 = std::numeric_limits<int16>::max();				// Signed max
			l_iYPosition = std::max(std::min(l_iYPosition,l_iMaxInt16),0);				// y = Truncate to 0:int16
			l_iXPosition = std::max(std::min(l_iXPosition,l_iMaxInt16),0);				// x = Truncate to 0:int16
			l_iPriority = -( (l_iYPosition << 15)  + l_iXPosition);						// compose pri32=[int16,int16] = [y,x]

			this->getLogManager() << LogLevel_Debug << "  -> coord-based box priority is " << l_iPriority << "\n";
		}

		CSimulatedBox* l_pSimulatedBox=new CSimulatedBox(getKernelContext(), *this);
		l_pSimulatedBox->setScenarioIdentifier(m_oScenarioIdentifier);
		l_pSimulatedBox->setBoxIdentifier(l_oBoxIdentifier);

		m_vSimulatedBox[std::make_pair(-l_iPriority, l_oBoxIdentifier)]=l_pSimulatedBox;
		m_vSimulatedBoxChrono[l_oBoxIdentifier].reset(static_cast<uint32>(m_ui64Frequency));
		m_vSimulatedBoxInputMutex[l_oBoxIdentifier] = new boost::mutex();
	}

	boolean l_bBoxInitialization = true;

	for(map < pair < int32, CIdentifier >, CSimulatedBox* >::iterator itSimulatedBox=m_vSimulatedBox.begin(); itSimulatedBox!=m_vSimulatedBox.end(); itSimulatedBox++)
	{
		const IBox* l_pBox=m_pScenario->getBoxDetails(itSimulatedBox->first.second);
		this->getLogManager() << LogLevel_Trace << "Scheduled box : id = " << itSimulatedBox->first.second << " priority = " << -itSimulatedBox->first.first << " name = " << l_pBox->getName() << "\n";
		if(itSimulatedBox->second ) // we initialize regardless of mute so that we can bring the box back during the run (in theory...)
		{
			if(!itSimulatedBox->second->initialize())
			{
				l_bBoxInitialization = false;
			}
		}
	}

	m_ui64Steps=0;
	m_ui64CurrentTime=0;
	m_bIsInitialized=true;

	m_oBenchmarkChrono.reset((System::uint32)m_ui64Frequency);
	if(l_bBoxInitialization)
	{
		return SchedulerInitialization_Success;
	}
	return SchedulerInitialization_BoxInitializationFailed;

}

boolean CSchedulerMulticore::uninitialize(void)
{
	this->getLogManager() << LogLevel_Trace << "Scheduler uninitialize\n";

	if(!m_bIsInitialized)
	{
		this->getLogManager() << LogLevel_Warning << "Trying to uninitialize an uninitialized player !\n";
		return false;
	}

	for(map < pair < int32, CIdentifier >, CSimulatedBox* >::iterator itSimulatedBox=m_vSimulatedBox.begin(); itSimulatedBox!=m_vSimulatedBox.end(); itSimulatedBox++)
	{
		if(itSimulatedBox->second)
		{
			itSimulatedBox->second->uninitialize();
		}
	}

	for(map < pair < int32, CIdentifier >, CSimulatedBox* >::iterator itSimulatedBox=m_vSimulatedBox.begin(); itSimulatedBox!=m_vSimulatedBox.end(); itSimulatedBox++)
	{
		delete itSimulatedBox->second;
	}
	m_vSimulatedBox.clear();

	for(map < CIdentifier, boost::mutex* >::iterator it = m_vSimulatedBoxInputMutex.begin(); it!=m_vSimulatedBoxInputMutex.end();it++)
	{
		delete it->second;
	}
	m_vSimulatedBoxInputMutex.clear();

	m_pScenario=NULL;

	m_bIsInitialized=false;
	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CSchedulerMulticore::job(CSchedulerMulticore *context, CIdentifier id, CSimulatedBox* box)
{
	return context->runBox(id, box);
}

boolean CSchedulerMulticore::runBox(CIdentifier id, CSimulatedBox* box)
{
//	std::cout << "id " << id.toString() << "\n";

	// we check once a cycle if the box is indeed muted.
	IBox* l_pBox=m_pScenario->getBoxDetails(id);
	boolean l_bIsMuted = false;
	if(l_pBox->hasAttribute(OV_AttributeId_Box_Muted))
	{
		CString l_sIsMuted = l_pBox->getAttributeValue(OV_AttributeId_Box_Muted);
		if (l_sIsMuted==CString("true"))
		{
			l_bIsMuted = true;
		}
	}

	System::CChrono& l_rSimulatedBoxChrono=m_vSimulatedBoxChrono[id];

	l_rSimulatedBoxChrono.stepIn();
	if(box)
	{
		if(!l_bIsMuted)
		{
			box->processClock();

			if(box->isReadyToProcess())
			{
				box->process();
			}
		}

		//if the box is muted we still have to erase chunks that arrives at the input
		{
			// CSimulatedBox in a thread may call m_rScheduler.sendInput() inside a process() call with modifies the
			// map of inputs. Hence, at least as long as the boxes are not synchronized into dependency layers, 
			// we need to guard accesses to the input map.
			boost::lock_guard<boost::mutex> lock(*m_vSimulatedBoxInputMutex[id]);

			map < uint32, list < CChunk > >& l_rSimulatedBoxInput=m_vSimulatedBoxInput[id];
			map < uint32, list < CChunk > >::iterator itSimulatedBoxInput;
			for(itSimulatedBoxInput=l_rSimulatedBoxInput.begin(); itSimulatedBoxInput!=l_rSimulatedBoxInput.end(); itSimulatedBoxInput++)
			{
				list < CChunk >& l_rSimulatedBoxInputChunkList=itSimulatedBoxInput->second;
				if(!l_bIsMuted)
				{
					list < CChunk >::iterator itSimulatedBoxInputChunkList;
					for(itSimulatedBoxInputChunkList=l_rSimulatedBoxInputChunkList.begin(); itSimulatedBoxInputChunkList!=l_rSimulatedBoxInputChunkList.end(); itSimulatedBoxInputChunkList++)
					{
						box->processInput(itSimulatedBoxInput->first, *itSimulatedBoxInputChunkList);

						if(box->isReadyToProcess())	
						{
							box->process();
						}
					}
				}
				l_rSimulatedBoxInputChunkList.clear();
			}
		}
		//process and processClock have been called for this cycle, we can clean the messages
		box->cleanupMessages();
	}
	l_rSimulatedBoxChrono.stepOut();

	if(l_rSimulatedBoxChrono.hasNewEstimation())
	{
		IBox* l_pBox=m_pScenario->getBoxDetails(id);
		l_pBox->addAttribute(OV_AttributeId_Box_ComputationTimeLastSecond, "");
		l_pBox->setAttributeValue(OV_AttributeId_Box_ComputationTimeLastSecond, CIdentifier(l_rSimulatedBoxChrono.getTotalStepInDuration()).toString());
	}

	return true;
}

boolean CSchedulerMulticore::loop(void)
{
	if(!m_bIsInitialized)
	{
		return false;
	}

	m_oBenchmarkChrono.stepIn();

	std::deque<jobCall> l_vJobList;

	// Send all the 'thread safe' stuff to the parallel execution
	for(map < pair < int32, CIdentifier >, CSimulatedBox* >::iterator itSimulatedBox=m_vSimulatedBox.begin(); itSimulatedBox!=m_vSimulatedBox.end(); itSimulatedBox++)
	{
		CSimulatedBox* l_pSimulatedBox=itSimulatedBox->second;
		// runBox(itSimulatedBox->first.second, l_pSimulatedBox);

		CIdentifier l_oBoxAlgId;
		l_pSimulatedBox->getBoxAlgorithmClassIdentifier(l_oBoxAlgId);
		const IPluginObjectDesc* l_pPOD = getPluginManager().getPluginObjectDescCreating(l_oBoxAlgId);

		if(g_ui32NumThreads > 0 && (l_pPOD && !l_pPOD->hasFunctionality(Kernel::PluginFunctionality_Visualization)))
		{
			l_vJobList.push_back(boost::bind(CSchedulerMulticore::job, this, itSimulatedBox->first.second, l_pSimulatedBox));
			// m_oExecutor.push(boost::bind(CSchedulerMulticore::job, this, itSimulatedBox->first.second, l_pSimulatedBox));
		}

	}

	m_oExecutor.pushList(l_vJobList);

	// Do the rest in the main thread
	for(map < pair < int32, CIdentifier >, CSimulatedBox* >::iterator itSimulatedBox=m_vSimulatedBox.begin(); itSimulatedBox!=m_vSimulatedBox.end(); itSimulatedBox++)
	{
		CSimulatedBox* l_pSimulatedBox=itSimulatedBox->second;
		// runBox(itSimulatedBox->first.second, l_pSimulatedBox);

		CIdentifier l_oBoxAlgId;
		l_pSimulatedBox->getBoxAlgorithmClassIdentifier(l_oBoxAlgId);
		const IPluginObjectDesc* l_pPOD = getPluginManager().getPluginObjectDescCreating(l_oBoxAlgId);

		if(g_ui32NumThreads==0 || (l_pPOD && l_pPOD->hasFunctionality(Kernel::PluginFunctionality_Visualization)))
		{
			job(this, itSimulatedBox->first.second, l_pSimulatedBox);
		}

	}

	// Wait for threads to finish their jobs
	m_oExecutor.waitForAll();

	m_oBenchmarkChrono.stepOut();

	if((m_ui64Steps%m_ui64Frequency)==0)
	{
		this->getLogManager() << LogLevel_Debug
			<< "<" << LogColor_PushStateBit << LogColor_ForegroundBlue << "Scheduler" << LogColor_PopStateBit
			<< "::" << LogColor_PushStateBit << LogColor_ForegroundBlue << "elapsed time" << LogColor_PopStateBit << "> "
			<< m_ui64Steps/m_ui64Frequency << "s\n";
	}

	if(m_oBenchmarkChrono.hasNewEstimation())
	{
		this->getLogManager() << LogLevel_Benchmark
			<< "<" << LogColor_PushStateBit << LogColor_ForegroundBlue << "Scheduler" << LogColor_PopStateBit
			<< "::" << LogColor_PushStateBit << LogColor_ForegroundBlue << "processor use" << LogColor_PopStateBit << "> "
			<< m_oBenchmarkChrono.getStepInPercentage() << "%\n";
	}

	m_ui64Steps++;

	m_ui64CurrentTime=m_ui64Steps*ITimeArithmetics::sampleCountToTime(m_ui64Frequency, 1LL);

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CSchedulerMulticore::sendInput(
	const CChunk& rChunk,
	const CIdentifier& rBoxIdentifier,
	const uint32 ui32InputIndex)
{
	IBox* l_pBox=m_pScenario->getBoxDetails(rBoxIdentifier);
	if(!l_pBox)
	{
		getLogManager() << LogLevel_Warning << "Tried to send data chunk with invalid box identifier " << rBoxIdentifier << "\n";
		return false;
	}

	if(ui32InputIndex >= l_pBox->getInputCount())
	{
		getLogManager() << LogLevel_Warning << "Tried to send data chunk with invalid input index " << ui32InputIndex << " for box identifier" << rBoxIdentifier << "\n";
		return false;
	}
#if 1
	map < pair < int32, CIdentifier >, CSimulatedBox* >::iterator itSimulatedBox=m_vSimulatedBox.begin();
	while(itSimulatedBox!=m_vSimulatedBox.end() && itSimulatedBox->first.second != rBoxIdentifier)
	{
		itSimulatedBox++;
	}
	if(itSimulatedBox==m_vSimulatedBox.end())
	{
		getLogManager() << LogLevel_ImportantWarning << "Tried to send data chunk with valid box identifier but invalid simulated box identifier " << rBoxIdentifier << "\n";
		return false;
	}
#endif
	CSimulatedBox* l_pSimulatedBox=itSimulatedBox->second;
	if(!l_pSimulatedBox)
	{
		getLogManager() << LogLevel_ImportantWarning << "Tried to send data chunk with valid box identifier, valid simulated box identifier " << rBoxIdentifier << " but the box has never been created\n";
		return false;
	}

	// TODO: check if ui32InputIndex does not overflow
	{
		boost::lock_guard<boost::mutex> lock(*m_vSimulatedBoxInputMutex[rBoxIdentifier]);
		m_vSimulatedBoxInput[rBoxIdentifier][ui32InputIndex].push_back(rChunk);
	}

	return true;
}

uint64 CSchedulerMulticore::getCurrentTime(void) const
{
	return m_ui64CurrentTime;
}

uint64 CSchedulerMulticore::getFrequency(void) const
{
	return m_ui64Frequency;
}

float64 CSchedulerMulticore::getCPUUsage(void) const
{
	return (const_cast<System::CChrono&>(m_oBenchmarkChrono)).getStepInPercentage();
}

bool CSchedulerMulticore::sendMessage(const IMessageWithData &msg, CIdentifier targetBox, uint32 inputIndex)
{
	CSimulatedBox* l_oReceiverSimulatedBox= NULL;

	map < pair < int32, CIdentifier >, CSimulatedBox* >::iterator itSimulatedBox;
	itSimulatedBox = m_vSimulatedBox.begin();
	CIdentifier l_oCurrentIdentifier = itSimulatedBox->first.second;
	// Find out the simulatedbox of targetBox, ask it to process it
	while ((l_oCurrentIdentifier != targetBox)&&(itSimulatedBox != m_vSimulatedBox.end()))
	{
		itSimulatedBox++;
		l_oCurrentIdentifier = itSimulatedBox->first.second;
	}

	if (l_oCurrentIdentifier == targetBox)
	{
		// Process
		l_oReceiverSimulatedBox = itSimulatedBox->second;
		return l_oReceiverSimulatedBox->receiveMessage(msg, inputIndex);
	}
	else
	{
		getLogManager() << LogLevel_ImportantWarning << "The box identifier provided for this message does not correspond to any box in this scenario\n";
		return false;
	}
}
