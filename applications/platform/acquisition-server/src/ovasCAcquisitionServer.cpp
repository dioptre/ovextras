#include "ovasCAcquisitionServer.h"
#include "ovasIAcquisitionServerPlugin.h"

#include <toolkit/ovtk_all.h>
#include <openvibe/ovITimeArithmetics.h>

#include <ovp_global_defines.h>

#include <system/ovCMemory.h>
#include <system/ovCTime.h>

#include <fstream>
#include <sstream>

#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <cstring>
#include <cmath> // std::isnan, std::isfinite
#include <condition_variable>

#include <cassert>

#include <mutex>

#include <iostream>

#define boolean OpenViBE::boolean

namespace
{
	// because std::tolower has multiple signatures,
	// it can not be easily used in std::transform
	// this workaround is taken from http://www.gcek.net/ref/books/sw/cpp/ticppv2/
	template <class charT>
	charT to_lower(charT c)
	{
		return std::tolower(c);
	}
};

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;

namespace OpenViBEAcquisitionServer
{
	class CDriverContext : public OpenViBEAcquisitionServer::IDriverContext
	{
	public:

		CDriverContext(const OpenViBE::Kernel::IKernelContext& rKernelContext, OpenViBEAcquisitionServer::CAcquisitionServer& rAcquisitionServer)
			:m_rKernelContext(rKernelContext)
			,m_rAcquisitionServer(rAcquisitionServer)
		{
		}

		virtual ILogManager& getLogManager(void) const
		{
			return m_rKernelContext.getLogManager();
		}

		virtual IConfigurationManager& getConfigurationManager(void) const
		{
			return m_rKernelContext.getConfigurationManager();
		}

		virtual boolean isConnected(void) const
		{
			return m_rAcquisitionServer.isConnected();
		}

		virtual boolean isStarted(void) const
		{
			return m_rAcquisitionServer.isStarted();
		}

		virtual boolean isImpedanceCheckRequested(void) const
		{
			return m_rAcquisitionServer.isImpedanceCheckRequested();
		}

		virtual boolean isChannelSelectionRequested(void) const
		{
			return m_rAcquisitionServer.isChannelSelectionRequested();
		}

		virtual int64 getDriftSampleCount(void) const
		{
			return m_rAcquisitionServer.m_oDriftCorrection.getDriftSampleCount();
		}

		virtual boolean correctDriftSampleCount(int64 i64SampleCount)
		{
			return m_rAcquisitionServer.m_oDriftCorrection.correctDrift(i64SampleCount, 
				m_rAcquisitionServer.m_ui64SampleCount,
				m_rAcquisitionServer.m_vPendingBuffer,
				m_rAcquisitionServer.m_oPendingStimulationSet,
				m_rAcquisitionServer.m_vSwapBuffer);
		}

		virtual int64 getDriftToleranceSampleCount(void) const
		{
			return m_rAcquisitionServer.m_oDriftCorrection.getDriftToleranceSampleCount();
		}

		virtual int64 getSuggestedDriftCorrectionSampleCount(void) const
		{
			return m_rAcquisitionServer.m_oDriftCorrection.getSuggestedDriftCorrectionSampleCount();
		}

		virtual boolean setInnerLatencySampleCount(int64 i64SampleCount)
		{
			return m_rAcquisitionServer.m_oDriftCorrection.setInnerLatencySampleCount(i64SampleCount);
		}

		virtual int64 getInnerLatencySampleCount(void) const
		{
			return m_rAcquisitionServer.m_oDriftCorrection.getInnerLatencySampleCount();
		}

		virtual boolean updateImpedance(const uint32 ui32ChannelIndex, const float64 f64Impedance)
		{
			return m_rAcquisitionServer.updateImpedance(ui32ChannelIndex, f64Impedance);
		}

		virtual uint64 getStartTime(void) const
		{
			return m_rAcquisitionServer.m_ui64StartTime;
		}

	protected:

		const IKernelContext& m_rKernelContext;
		CAcquisitionServer& m_rAcquisitionServer;
	};

	class CConnectionServerHandlerThread
	{
	public:
		CConnectionServerHandlerThread(CAcquisitionServer& rAcquisitionServer, Socket::IConnectionServer& rConnectionServer)
			:m_rAcquisitionServer(rAcquisitionServer)
			,m_rConnectionServer(rConnectionServer)
		{
		}

		void operator()(void)
		{
			Socket::IConnection* l_pConnection=NULL;
			do
			{
				l_pConnection=m_rConnectionServer.accept();
				m_rAcquisitionServer.acceptNewConnection(l_pConnection);
			}
			while(l_pConnection && m_rAcquisitionServer.isConnected());
		}

		CAcquisitionServer& m_rAcquisitionServer;
		Socket::IConnectionServer& m_rConnectionServer;
	};

	class CConnectionClientHandlerThread
	{
	public:
		CConnectionClientHandlerThread(CAcquisitionServer& rAcquisitionServer, Socket::IConnection& rConnection)
			:m_rAcquisitionServer(rAcquisitionServer)
			,m_rConnection(rConnection)
			,m_bPleaseQuit(false)
		{
		}

		void operator()(void)
		{
			std::unique_lock<std::mutex> oLock(m_oClientThreadMutex, std::defer_lock);

			while(true)
			{
				oLock.lock();

				// Wait until something interesting happens...
				m_oPendingBufferCondition.wait(oLock,
					[this]() { 
						return (m_bPleaseQuit ||  !m_rConnection.isConnected() || m_vClientPendingBuffer.size()>0);
					}
				);

				// Exit the loop if we're told to quit or if we've lost the connection
				if(m_bPleaseQuit || !m_rConnection.isConnected())
				{
					oLock.unlock();
					break;
				}

				// At this point, we should have a buffer
				if(!m_vClientPendingBuffer.size())
				{
					// n.b. Shouldn't happen, but we don't have an error reporting mechanism in the thread...
					oLock.unlock();
					continue;
				}

				CMemoryBuffer* l_pMemoryBuffer=m_vClientPendingBuffer.front();
				m_vClientPendingBuffer.pop_front();

				// Don't go into blocking send while holding the lock; ok to unlock as l_pMemoryBuffer ptr+mem is now owned by this thread
				oLock.unlock();

				const uint64 l_ui64MemoryBufferSize=l_pMemoryBuffer->getSize();
				m_rConnection.sendBufferBlocking(&l_ui64MemoryBufferSize, sizeof(l_ui64MemoryBufferSize));
				m_rConnection.sendBufferBlocking(l_pMemoryBuffer->getDirectPointer(), (uint32)l_pMemoryBuffer->getSize());
				delete l_pMemoryBuffer;

			}

			oLock.lock();

			// We're done, clean any possible pending buffers
			for(auto it = m_vClientPendingBuffer.begin(); it!=m_vClientPendingBuffer.end(); it++)
			{
				delete *it;
			}
			m_vClientPendingBuffer.clear();

			oLock.unlock();

			// The thread will exit here and can be joined
		}

		void scheduleBuffer(const IMemoryBuffer& rMemoryBuffer)
		{
			{
				std::lock_guard<std::mutex> oLock(m_oClientThreadMutex);
				if(!m_bPleaseQuit)
				{
					CMemoryBuffer* l_pMemoryBuffer=new CMemoryBuffer(rMemoryBuffer);
					m_vClientPendingBuffer.push_back(l_pMemoryBuffer);
				}
			}

			// No big harm notifying in any case, though if in 'quit' state, the quit request has already notified
			m_oPendingBufferCondition.notify_one();
		}

		CAcquisitionServer& m_rAcquisitionServer;
		Socket::IConnection& m_rConnection;

		std::deque < CMemoryBuffer* > m_vClientPendingBuffer;

		// Here we use a condition variable to avoid sleeping
		std::mutex m_oClientThreadMutex;
		std::condition_variable m_oPendingBufferCondition;
		
		// Should this thread quit?
		bool m_bPleaseQuit;

	};

	static void start_connection_client_handler_thread(CConnectionClientHandlerThread* pThread)
	{
		(*pThread)();
	}
}

//___________________________________________________________________//
//                                                                   //

CAcquisitionServer::CAcquisitionServer(const IKernelContext& rKernelContext)
	:m_pConnectionServerHandlerStdThread(NULL)
	,m_rKernelContext(rKernelContext)
	,m_pDriverContext(NULL)
	,m_pDriver(NULL)
	,m_pStreamEncoder(NULL)
	,m_pConnectionServer(NULL)
	,m_eNaNReplacementPolicy(NaNReplacementPolicy_LastCorrectValue)
	,m_bReplacementInProgress(false)
	,m_bInitialized(false)
	,m_bStarted(false)
	,m_ui64StartTime(0)
	,m_ui64LastDeliveryTime(0)
	,m_ui64SentSampleCount(0)
	,m_oDriftCorrection(rKernelContext)
{
	m_pDriverContext=new CDriverContext(rKernelContext, *this);

	m_pStreamEncoder=&m_rKernelContext.getAlgorithmManager().getAlgorithm(m_rKernelContext.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_MasterAcquisitionStreamEncoder));
	m_pStreamEncoder->initialize();

	ip_ui64SubjectIdentifier.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_SubjectIdentifier));
	ip_ui64SubjectAge.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_SubjectAge));
	ip_ui64SubjectGender.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_SubjectGender));
	ip_pSignalMatrix.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_SignalMatrix));
	ip_ui64SignalSamplingRate.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_SignalSamplingRate));
	ip_pStimulationSet.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_StimulationSet));
	ip_ui64BufferDuration.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_BufferDuration));
	ip_pChannelUnits.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_ChannelUnits));
	op_pEncodedMemoryBuffer.initialize(m_pStreamEncoder->getOutputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_OutputParameterId_EncodedMemoryBuffer));

	ip_bEncodeChannelLocalisationData.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_EncodeChannelLocalisationData));
	ip_bEncodeChannelUnitData.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputParameterId_EncodeChannelUnitData));

	CString l_sNaNReplacementPolicy=m_rKernelContext.getConfigurationManager().expand("${AcquisitionServer_NaNReplacementPolicy}");
	if(l_sNaNReplacementPolicy==CString("Disabled"))
	{
		this->setNaNReplacementPolicy(NaNReplacementPolicy_Disabled);
	}
	else if(l_sNaNReplacementPolicy==CString("Zero"))
	{
		this->setNaNReplacementPolicy(NaNReplacementPolicy_Zero);
	}
	else
	{
		this->setNaNReplacementPolicy(NaNReplacementPolicy_LastCorrectValue);
	}

	this->setOversamplingFactor(m_rKernelContext.getConfigurationManager().expandAsUInteger("${AcquisitionServer_OverSamplingFactor}", 1));
	this->setImpedanceCheckRequest(m_rKernelContext.getConfigurationManager().expandAsBoolean("${AcquisitionServer_CheckImpedance}", false));
	this->setChannelSelectionRequest(m_rKernelContext.getConfigurationManager().expandAsBoolean("${AcquisitionServer_ChannelSelection}", false));

	m_i64StartedDriverSleepDuration= m_rKernelContext.getConfigurationManager().expandAsInteger("${AcquisitionServer_StartedDriverSleepDuration}", 0);
	m_ui64StoppedDriverSleepDuration=m_rKernelContext.getConfigurationManager().expandAsUInteger("${AcquisitionServer_StoppedDriverSleepDuration}", 100);
	m_ui64DriverTimeoutDuration=m_rKernelContext.getConfigurationManager().expandAsUInteger("${AcquisitionServer_DriverTimeoutDuration}", 5000);

	for(auto itp = m_vPlugins.begin(); itp != m_vPlugins.end(); ++itp)
	{
		(*itp)->createHook();
	}
}

CAcquisitionServer::~CAcquisitionServer(void)
{
	if(m_bStarted)
	{
		m_pDriver->stop();
		m_bStarted=false;
	}

	if(m_bInitialized)
	{
		m_pDriver->uninitialize();
		m_bInitialized=false;
	}

	if(m_pConnectionServer)
	{
		m_pConnectionServer->release();
		m_pConnectionServer=NULL;
	}

	// n.b. We don't clear the connection list as the teardown order (even on window close)
	// will lead to AcquisitionServerGUI terminating the AcquisitionThread which will
	// in turn call the server's ::stop() that clears the list.

	ip_ui64SubjectIdentifier.uninitialize();
	ip_ui64SubjectAge.uninitialize();
	ip_ui64SubjectGender.uninitialize();
	ip_pSignalMatrix.uninitialize();
	ip_ui64SignalSamplingRate.uninitialize();
	ip_pStimulationSet.uninitialize();
	ip_ui64BufferDuration.uninitialize();
	op_pEncodedMemoryBuffer.uninitialize();
	ip_pChannelUnits.uninitialize();

	ip_bEncodeChannelLocalisationData.uninitialize();
	ip_bEncodeChannelUnitData.uninitialize();

	m_pStreamEncoder->uninitialize();
	m_rKernelContext.getAlgorithmManager().releaseAlgorithm(*m_pStreamEncoder);
	m_pStreamEncoder=NULL;

	delete m_pDriverContext;
	m_pDriverContext=NULL;
}

IDriverContext& CAcquisitionServer::getDriverContext(void)
{
	return *m_pDriverContext;
}

uint32 CAcquisitionServer::getClientCount(void)
{
	return uint32(m_vConnection.size());
}


float64 CAcquisitionServer::getImpedance(const uint32 ui32ChannelIndex)
{
	if(ui32ChannelIndex < m_vImpedance.size())
	{
		return m_vImpedance[ui32ChannelIndex];
	}
	return OVAS_Impedance_Unknown;
}

//___________________________________________________________________//
//                                                                   //

boolean CAcquisitionServer::loop(void)
{
	// m_rKernelContext.getLogManager() << LogLevel_Debug << "loop()\n";

	// Searches for new connection(s)
	if(m_pConnectionServer)
	{
		DoubleLock lock(&m_oPendingConnectionProtectionMutex, &m_oPendingConnectionExecutionMutex);

		for(auto itConnection=m_vPendingConnection.begin(); itConnection!=m_vPendingConnection.end(); itConnection++)
		{
			m_rKernelContext.getLogManager() << LogLevel_Info << "Received new connection...\n";

			Socket::IConnection* l_pConnection=itConnection->first;
			if(this->isStarted())
			{
				// When a new connection is found and the
				// acq server is started, send the header

				SConnectionInfo l_oInfo;
				l_oInfo.m_ui64ConnectionTime=0;
				l_oInfo.m_pConnectionClientHandlerThread=new CConnectionClientHandlerThread(*this, *l_pConnection);
				l_oInfo.m_pConnectionClientHandlerStdThread=new std::thread(std::bind(&start_connection_client_handler_thread, l_oInfo.m_pConnectionClientHandlerThread));
				l_oInfo.m_bChannelLocalisationSent = false;
				l_oInfo.m_bChannelUnitsSent = false;

				m_vConnection.push_back(pair < Socket::IConnection*, SConnectionInfo >(l_pConnection, l_oInfo));

#if DEBUG_STREAM
				m_rKernelContext.getLogManager() << LogLevel_Debug << "Creating header\n";
#endif

				op_pEncodedMemoryBuffer->setSize(0, true);
				m_pStreamEncoder->process(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputTriggerId_EncodeHeader);

#if 0
				uint64 l_ui64MemoryBufferSize=op_pEncodedMemoryBuffer->getSize();
				l_pConnection->sendBufferBlocking(&l_ui64MemoryBufferSize, sizeof(l_ui64MemoryBufferSize));
				l_pConnection->sendBufferBlocking(op_pEncodedMemoryBuffer->getDirectPointer(), (uint32)op_pEncodedMemoryBuffer->getSize());
#else
				l_oInfo.m_pConnectionClientHandlerThread->scheduleBuffer(*op_pEncodedMemoryBuffer);
#endif
			}
			else
			{
				// When a new connection is found and the
				// acq server is _not_ started, drop the
				// connection

				m_rKernelContext.getLogManager() << LogLevel_Warning << "Dropping connection - acquisition is not started\n";
				l_pConnection->release();
			}
		}
		m_vPendingConnection.clear();
	}

	// Cleans disconnected client(s)
	for(auto itConnection=m_vConnection.begin(); itConnection!=m_vConnection.end(); )
	{
		Socket::IConnection* l_pConnection=itConnection->first;
		if(!l_pConnection->isConnected())
		{
			if(itConnection->second.m_pConnectionClientHandlerStdThread)
			{
				requestClientThreadQuit(itConnection->second.m_pConnectionClientHandlerThread);

				itConnection->second.m_pConnectionClientHandlerStdThread->join();
				delete itConnection->second.m_pConnectionClientHandlerStdThread;
				delete itConnection->second.m_pConnectionClientHandlerThread;
			}
			l_pConnection->release();
			itConnection=m_vConnection.erase(itConnection);
			m_rKernelContext.getLogManager() << LogLevel_Info << "Closed connection...\n";
		}
		else
		{
			itConnection++;
		}
	}

	// Handles driver's main loop
	if(m_pDriver)
	{
		boolean l_bResult;
		boolean l_bTimeout;
		if(this->isStarted())
		{
			l_bResult=true;
			l_bTimeout=false;
			m_bGotData=false;
			const uint32 l_ui32TimeBeforeCall=System::Time::getTime();
			while(l_bResult && !m_bGotData && !l_bTimeout)
			{
				// Calls driver's loop
				l_bResult=m_pDriver->loop();
				if(!m_bGotData)
				{
					l_bTimeout=(System::Time::getTime()>l_ui32TimeBeforeCall+m_ui64DriverTimeoutDuration);

					if(m_i64StartedDriverSleepDuration>0) 
					{
						// This may cause jitter due to inaccuracies in sleep duration, but has the
						// benefit that it frees the CPU core for other tasks
						System::Time::sleep( static_cast<uint32_t>(m_i64StartedDriverSleepDuration) );
					} 
					else if(m_i64StartedDriverSleepDuration==0)
					{
						// Generally spins, but gives other threads a chance. Note that there is no guarantee when
						// the scheduler reschedules this thread.
						std::this_thread::yield();
					}
					else
					{
						// < 0 -> NOP, spins, doesn't offer to yield 
						// n.b. Unless the driver waits for samples (preferably with a hardware event), 
						// this choice will spin one core fully.
					}
				}
			}
			if(l_bTimeout)
			{
				m_rKernelContext.getLogManager() << LogLevel_ImportantWarning << "After " << m_ui64DriverTimeoutDuration << " milliseconds, did not receive anything from the driver - Timed out\n";
				return false;
			}
			if(m_bGotData && m_oDriftCorrection.getDriftCorrectionPolicy() == DriftCorrectionPolicy_Forced)
			{
				m_oDriftCorrection.correctDrift(m_oDriftCorrection.getSuggestedDriftCorrectionSampleCount(),
					m_ui64SampleCount, m_vPendingBuffer, m_oPendingStimulationSet, m_vSwapBuffer);
			}
		}
		else
		{
			// Calls driver's loop
			l_bResult=m_pDriver->loop();
			System::Time::sleep((uint32)m_ui64StoppedDriverSleepDuration);
		}

		// Calls driver's loop
		if(!l_bResult)
		{
			m_rKernelContext.getLogManager() << LogLevel_ImportantWarning << "Something bad happened in the loop callback, stopping the acquisition\n";
			return false;
		}
	}

	while(m_vPendingBuffer.size() >= m_ui32SampleCountPerSentBlock)
	{
		// We only send to clients when we have accumulated a full buffer

		// Queues a single buffer to connected client(s)
		// n.b. here we use arithmetic based on buffer duration so that we are in perfect agreement with
		// Acquisition Client box that sets the chunk starts and ends by steps of buffer duration.
		const uint64 l_ui64BufferDuration     = ip_ui64BufferDuration;
		const uint64 l_ui64SentBufferCount    = m_ui64SentSampleCount / m_ui32SampleCountPerSentBlock;
		const uint64 l_ui64BufferStartTime    = l_ui64SentBufferCount * l_ui64BufferDuration;
		const uint64 l_ui64BufferEndTime      = l_ui64BufferStartTime + l_ui64BufferDuration;

		// Note: if pendingbuffer has multiple chunks worth of data, the lastSampleTime will be the same for 
		// multiple calls of loopHook().
		const uint64 l_ui64LastSampleTime     = ITimeArithmetics::sampleCountToTime(m_ui32SamplingFrequency, m_ui64SampleCount); // theoretical time of the last sample from the device

		// Pass the stimuli and buffer to all plugins; note that they may modify them
		for(std::vector<IAcquisitionServerPlugin*>::iterator itp = m_vPlugins.begin(); itp != m_vPlugins.end(); ++itp)
		{
			// n.b. this potentially passes in more than 1 chunk; the receiver should nevertheless just act on the first one
			(*itp)->loopHook(m_vPendingBuffer, m_oPendingStimulationSet, l_ui64BufferStartTime, l_ui64BufferEndTime, l_ui64LastSampleTime);
		}

		// Prepare the buffer for all clients
		for(uint32 j=0; j<m_ui32ChannelCount; j++)
		{
			for(uint32 i=0; i<m_ui32SampleCountPerSentBlock; i++)
			{
				ip_pSignalMatrix->getBuffer()[j*m_ui32SampleCountPerSentBlock+i]=m_vPendingBuffer[i][j];
			}
		}

		// Verify stimulation dates
		for(size_t k = 0 ; k<m_oPendingStimulationSet.getStimulationCount();k++)
		{
			if(m_oPendingStimulationSet.getStimulationDate(k)<l_ui64BufferStartTime)
			{
				m_rKernelContext.getLogManager() << LogLevel_Warning << "Stimulation " << m_oPendingStimulationSet.getStimulationIdentifier(k)
					<< " at " << ITimeArithmetics::timeToSeconds(m_oPendingStimulationSet.getStimulationDate(k)) << "s is too old for buffer at ["
					<<  ITimeArithmetics::timeToSeconds(l_ui64BufferStartTime) << "," << ITimeArithmetics::timeToSeconds(l_ui64BufferEndTime) << "]s\n";
			}	
		}

		// Pass the buffer to each current connection 
		for(auto itConnection=m_vConnection.begin(); itConnection!=m_vConnection.end(); itConnection++)
		{
			SConnectionInfo& l_rInfo=itConnection->second;

			if(l_rInfo.m_ui64ConnectionTime==0)
			{
				l_rInfo.m_ui64ConnectionTime = l_ui64BufferStartTime;
			}

	#if DEBUG_STREAM
			m_rKernelContext.getLogManager() << LogLevel_Debug << "Creating buffer for connection " << uint64(l_pConnection) << "\n";
	#endif

			// Stimulation buffer
			IStimulationSet& l_oStimulationSet = *ip_pStimulationSet;
			l_oStimulationSet.clear();

			// Take the stimuli range valid for the buffer and adjust wrt connection time (stamp at connection = stamp at time 0 for the client)
			for(size_t k=0;k<m_oPendingStimulationSet.getStimulationCount();k++)
			{
				const uint64 lDate = m_oPendingStimulationSet.getStimulationDate(k); // this date is wrt the whole acquisition time in the server
				if(lDate <= l_ui64BufferEndTime)
				{
					// Note that we push a pending stim out even if it is old. This is because it likely signifies and error somewhere,
					// and it is better to expose the issue to the designer side than drop it.

					// The new date is wrt the specific connection time of the client (i.e. the chunk times on Designer side)
					const uint64 lNewDate = ( (lDate > l_rInfo.m_ui64ConnectionTime) ? (lDate - l_rInfo.m_ui64ConnectionTime) : 0 );
					
					l_oStimulationSet.appendStimulation(m_oPendingStimulationSet.getStimulationIdentifier(k), lNewDate, m_oPendingStimulationSet.getStimulationDuration(k));

					m_rKernelContext.getLogManager() << LogLevel_Info << "map: " << m_oPendingStimulationSet.getStimulationIdentifier(k) << " from " << ITimeArithmetics::timeToSeconds(lDate) << " to " << ITimeArithmetics::timeToSeconds(lNewDate) << "\n";
	//				m_rKernelContext.getLogManager() << LogLevel_Info << "map: " << m_oPendingStimulationSet.getStimulationIdentifier(k) << " from " << lDate << " to " << lNewDate << "\n";
				}
			}

			// Send a chunk of channel units? Note that we'll always send the units header.
			if(!l_rInfo.m_bChannelUnitsSent)
			{
				// If default values in channel units, don't bother sending unit data chunk
				ip_bEncodeChannelUnitData = m_pHeaderCopy->isChannelUnitSet();
				// std::cout << "Set " <<  ip_pChannelUnits->getBufferElementCount()  << "\n";
			}

			op_pEncodedMemoryBuffer->setSize(0, true);
			m_pStreamEncoder->process(OVP_GD_Algorithm_MasterAcquisitionStreamEncoder_InputTriggerId_EncodeBuffer);

			if(!l_rInfo.m_bChannelUnitsSent)
			{
				// Do not send again
				l_rInfo.m_bChannelUnitsSent = true;
				ip_bEncodeChannelUnitData = false;
			}

			l_rInfo.m_pConnectionClientHandlerThread->scheduleBuffer(*op_pEncodedMemoryBuffer);
		}

		// Clear stimulations that are now in the past
		OpenViBEToolkit::Tools::StimulationSet::removeRange(
					m_oPendingStimulationSet,
					0,
					l_ui64BufferEndTime
					);

		// Clear the buffer from the samples we sent
		m_vPendingBuffer.erase(m_vPendingBuffer.begin(), m_vPendingBuffer.begin()+m_ui32SampleCountPerSentBlock);

		m_ui64SentSampleCount += m_ui32SampleCountPerSentBlock;

	}

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CAcquisitionServer::connect(IDriver& rDriver, IHeader& rHeaderCopy, uint32 ui32SamplingCountPerSentBlock, uint32 ui32ConnectionPort)
{
	m_rKernelContext.getLogManager() << LogLevel_Debug << "connect\n";


	m_pDriver=&rDriver;
	m_pHeaderCopy=&rHeaderCopy;
	m_ui32SampleCountPerSentBlock=ui32SamplingCountPerSentBlock;

	m_rKernelContext.getLogManager() << LogLevel_Info << "Connecting to device [" << CString(m_pDriver->getName()) << "]...\n";

	// Initializes driver
	if(!m_pDriver->initialize(m_ui32SampleCountPerSentBlock, *this))
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Connection failed...\n";
		return false;
	}

	m_rKernelContext.getLogManager() << LogLevel_Info << "Connection succeeded !\n";

	const IHeader& l_rHeader=*rDriver.getHeader();
	IHeader::copy(rHeaderCopy, l_rHeader);

	m_ui32ChannelCount=rHeaderCopy.getChannelCount();
	m_ui32SamplingFrequency=(uint32)(rHeaderCopy.getSamplingFrequency()*m_ui64OverSamplingFactor);

	m_vSelectedChannels.clear();
	if (m_bIsChannelSelectionRequested)
	{
		for (OpenViBE::uint32 i=0,l=0;i<m_ui32ChannelCount;++i) {
			const std::string name = rHeaderCopy.getChannelName(i);
			if (name!="") {
				m_vSelectedChannels.push_back(i);
				rHeaderCopy.setChannelName(l,name.c_str());

				uint32 l_ui32Unit = 0, l_ui32Factor = 0;
				if(rHeaderCopy.isChannelUnitSet())
				{
					rHeaderCopy.getChannelUnits(i, l_ui32Unit, l_ui32Factor); // no need to check, will be defaults on failure
					rHeaderCopy.setChannelUnits(l, l_ui32Unit, l_ui32Factor);
				}
				l++;
			}
		}
		rHeaderCopy.setChannelCount(m_vSelectedChannels.size());
		m_ui32ChannelCount = rHeaderCopy.getChannelCount();

	} else {

		for (OpenViBE::uint32 i=0;i<m_ui32ChannelCount;++i)
			m_vSelectedChannels.push_back(i);
	}
	
	// These are passed to plugins
	m_vSelectedChannelNames.clear();
	for(uint32 i=0;i<rHeaderCopy.getChannelCount();i++) 
	{
		m_vSelectedChannelNames.push_back(CString(rHeaderCopy.getChannelName(i)));
	}

	if(m_ui32ChannelCount==0)
	{
		std::stringstream ss;
		ss << "Driver claimed to have " << uint32(0) << " channel";
		if (isChannelSelectionRequested())
			ss << "(check whether the property `Select only named channel' is set).\n";
		m_rKernelContext.getLogManager() << LogLevel_Error << ss.str().c_str();
		return false;
	}

	if(m_ui32SamplingFrequency==0)
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Driver claimed to have a sample frequency of " << uint32(0) << "\n";
		return false;
	}

	m_vImpedance.resize(m_ui32ChannelCount, OVAS_Impedance_NotAvailable);
	m_vSwapBuffer.resize(m_ui32ChannelCount);

	m_pConnectionServer=Socket::createConnectionServer();
	if(m_pConnectionServer->listen(ui32ConnectionPort))
	{
		m_bInitialized=true;

		m_rKernelContext.getLogManager() << LogLevel_Trace << "NaN value correction is set to ";
		switch(m_eNaNReplacementPolicy)
		{
			default:
			case NaNReplacementPolicy_LastCorrectValue: m_rKernelContext.getLogManager() << CString("LastCorrectValue") << "\n"; break;
			case NaNReplacementPolicy_Zero:             m_rKernelContext.getLogManager() << CString("Zero") << "\n"; break;
			case NaNReplacementPolicy_Disabled:         m_rKernelContext.getLogManager() << CString("Disabled") << "\n"; break;
		};

		m_rKernelContext.getLogManager() << LogLevel_Trace << "Oversampling factor set to " << m_ui64OverSamplingFactor << "\n";
		m_rKernelContext.getLogManager() << LogLevel_Trace << "Sampling frequency set to " << m_ui32SamplingFrequency << "Hz\n";
		m_rKernelContext.getLogManager() << LogLevel_Trace << "Started driver sleeping duration is " << m_i64StartedDriverSleepDuration << " milliseconds\n";
		m_rKernelContext.getLogManager() << LogLevel_Trace << "Stopped driver sleeping duration is " << m_ui64StoppedDriverSleepDuration << " milliseconds\n";
		m_rKernelContext.getLogManager() << LogLevel_Trace << "Driver timeout duration set to " << m_ui64DriverTimeoutDuration << " milliseconds\n";

		ip_ui64BufferDuration=ITimeArithmetics::sampleCountToTime(m_ui32SamplingFrequency, m_ui32SampleCountPerSentBlock);

		ip_ui64SubjectIdentifier=rHeaderCopy.getExperimentIdentifier();
		ip_ui64SubjectAge=rHeaderCopy.getSubjectAge();
		ip_ui64SubjectGender=rHeaderCopy.getSubjectGender();

		ip_ui64SignalSamplingRate=m_ui32SamplingFrequency;
		ip_pSignalMatrix->setDimensionCount(2);
		ip_pSignalMatrix->setDimensionSize(0, m_ui32ChannelCount);
		ip_pSignalMatrix->setDimensionSize(1, m_ui32SampleCountPerSentBlock);

		m_rKernelContext.getLogManager() << LogLevel_Trace << "Sampling rate     : " << m_ui32SamplingFrequency << "\n";
		m_rKernelContext.getLogManager() << LogLevel_Trace << "Samples per block : " << m_ui32SampleCountPerSentBlock << "\n";
		m_rKernelContext.getLogManager() << LogLevel_Trace << "Channel count     : " << m_ui32ChannelCount << "\n";

		for(uint32 i=0; i<m_ui32ChannelCount; i++)
		{
			const std::string l_sChannelName=rHeaderCopy.getChannelName(i);
			if(l_sChannelName!="")
			{
				ip_pSignalMatrix->setDimensionLabel(0, i, l_sChannelName.c_str());
			}
			else
			{
				std::stringstream ss;
				ss << "Channel " << i+1;
				ip_pSignalMatrix->setDimensionLabel(0, i, ss.str().c_str());
			}
			m_rKernelContext.getLogManager() << LogLevel_Trace << "Channel name      : " << CString(ip_pSignalMatrix->getDimensionLabel(0, i)) << "\n";
		}

		// Construct channel units stream header & matrix
		// Note: Channel units, although part of IHeader, will be sent as a matrix chunk during loop() once - if at all - to each client
		if(m_pHeaderCopy->isChannelUnitSet())
		{
			ip_pChannelUnits->setDimensionCount(2);
			ip_pChannelUnits->setDimensionSize(0, m_ui32ChannelCount);
			ip_pChannelUnits->setDimensionSize(1,2);                    // Units, Factor
			for(uint32 c=0;c<m_ui32ChannelCount;c++)
			{
				ip_pChannelUnits->setDimensionLabel(0, c, m_pHeaderCopy->getChannelName(c));
				if(m_pHeaderCopy->isChannelUnitSet())
				{
					uint32 l_ui32Unit = 0, l_ui32Factor = 0;
				
					m_pHeaderCopy->getChannelUnits(c, l_ui32Unit, l_ui32Factor); // no need to check, will be defaults on failure

					ip_pChannelUnits->getBuffer()[c*2+0] = static_cast<float64>(l_ui32Unit);
					ip_pChannelUnits->getBuffer()[c*2+1] = static_cast<float64>(l_ui32Factor);

					m_rKernelContext.getLogManager() << LogLevel_Trace << "Channel Unit      : " << l_ui32Unit << ", factor=" << OVTK_DECODE_FACTOR(l_ui32Factor) << "\n";
				}
			}
			ip_pChannelUnits->setDimensionLabel(1, 0, "Unit");
			ip_pChannelUnits->setDimensionLabel(1, 1, "Factor");
		}
		else
		{
			// Driver did not set units. Convention: send empty header matrix in this case.
			ip_pChannelUnits->setDimensionCount(2);
			ip_pChannelUnits->setDimensionSize(0,0);
			ip_pChannelUnits->setDimensionSize(0,0);
			m_rKernelContext.getLogManager() << LogLevel_Trace << "Driver did not set units, sending empty channel units matrix\n";
		}

		// @TODO Channel localisation
		{
			ip_bEncodeChannelLocalisationData = false; // never at the moment

		}

		// @TODO Gain is ignored
	}
	else
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Could not listen on TCP port (firewall problem ?)\n";
		return false;
	}

	m_pConnectionServerHandlerStdThread=new std::thread(CConnectionServerHandlerThread(*this, *m_pConnectionServer));

	return true;
}

boolean CAcquisitionServer::start(void)
{
	m_rKernelContext.getLogManager() << LogLevel_Debug << "buttonStartPressedCB\n";

	m_rKernelContext.getLogManager() << LogLevel_Info << "Starting the acquisition...\n";

	m_vPendingBuffer.clear();
	m_oPendingStimulationSet.clear();

	m_ui64SampleCount=0;
	m_ui64LastSampleCount=0;

	m_ui64SentSampleCount=0;

	// Starts driver
	if(!m_pDriver->start())
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Starting failed !\n";
		return false;
	}

	m_ui64StartTime = System::Time::zgetTime();
	m_ui64LastDeliveryTime = m_ui64StartTime;

	m_oDriftCorrection.start(m_ui32SamplingFrequency, m_ui64StartTime);

	for(auto itp = m_vPlugins.begin(); itp != m_vPlugins.end(); ++itp)
	{
		(*itp)->startHook(m_vSelectedChannelNames, m_ui32SamplingFrequency, m_ui32ChannelCount, m_ui32SampleCountPerSentBlock);
	}

	m_rKernelContext.getLogManager() << LogLevel_Info << "Now acquiring...\n";

	m_bStarted=true;

	return true;
}

bool CAcquisitionServer::requestClientThreadQuit(CConnectionClientHandlerThread* th)
{
	// Use a scoped lock before toggling a variable owned by the thread
	{
		std::lock_guard<std::mutex> oLock(th->m_oClientThreadMutex);
				
		// Tell the thread to quit
		th->m_bPleaseQuit = true;
	}

	// Wake up the thread in case it happens to be waiting on the cond var
	th->m_oPendingBufferCondition.notify_one();

	return true;
}

boolean CAcquisitionServer::stop(void)
{
	m_rKernelContext.getLogManager() << LogLevel_Debug << "buttonStopPressedCB\n";

	m_rKernelContext.getLogManager() << LogLevel_Info << "Stopping the acquisition.\n";

	m_oDriftCorrection.printStats();
	m_oDriftCorrection.stop();

	// Stops driver
	m_pDriver->stop();

	for(auto itConnection=m_vConnection.begin(); itConnection!=m_vConnection.end(); itConnection++)
	{
		if(itConnection->first->isConnected())
		{
			itConnection->first->close();
		}
		if(itConnection->second.m_pConnectionClientHandlerStdThread)
		{
			requestClientThreadQuit(itConnection->second.m_pConnectionClientHandlerThread);

			itConnection->second.m_pConnectionClientHandlerStdThread->join();
			delete itConnection->second.m_pConnectionClientHandlerStdThread;
			delete itConnection->second.m_pConnectionClientHandlerThread;
		}
		itConnection->first->release();
	}
	m_vConnection.clear();

	for(auto itp = m_vPlugins.begin(); itp != m_vPlugins.end(); ++itp)
	{
		(*itp)->stopHook();
	}


	m_bStarted=false;

	m_vPendingBuffer.clear();

	return true;
}

boolean CAcquisitionServer::disconnect(void)
{
	m_rKernelContext.getLogManager() << LogLevel_Info << "Disconnecting.\n";

	if(m_bInitialized)
	{
		m_pDriver->uninitialize();
	}

	m_vImpedance.clear();

	if(m_pConnectionServer)
	{
		m_pConnectionServer->close();
		m_pConnectionServer->release();
		m_pConnectionServer=NULL;
	}

	m_bInitialized=false;

	// Thread joining must be done after
	// switching m_bInitialized to false
	if(m_pConnectionServerHandlerStdThread)
	{
		m_pConnectionServerHandlerStdThread->join();
		delete m_pConnectionServerHandlerStdThread;
		m_pConnectionServerHandlerStdThread=NULL;
	}

	return true;
}

//___________________________________________________________________//
//                                                                   //

void CAcquisitionServer::setSamples(const float32* pSample)
{
	if (pSample==NULL) m_rKernelContext.getLogManager() << LogLevel_Warning << "Null data detected\n";
	this->setSamples(pSample, m_ui32SampleCountPerSentBlock);
}

void CAcquisitionServer::setSamples(const float32* pSample, const uint32 ui32SampleCount)
{

	if(m_bStarted)
	{
		for(uint32 i=0; i<ui32SampleCount; i++)
		{
			if(!m_bReplacementInProgress)
			{
				// otherwise NaN are propagating
				m_vOverSamplingSwapBuffer=m_vSwapBuffer;
			}
			for(uint32 k=0; k<m_ui64OverSamplingFactor; k++)
			{
				const float32 alpha=float32(k+1)/m_ui64OverSamplingFactor;

				bool l_bHadNaN = false;

				for(uint32 j=0; j<m_ui32ChannelCount; j++)
				{
					const uint32 l_ui32Channel = m_vSelectedChannels[j];

					if(std::isnan(pSample[l_ui32Channel*ui32SampleCount+i]) || !std::isfinite(pSample[l_ui32Channel*ui32SampleCount+i])) // NaN or infinite values
					{
						l_bHadNaN = true;

						switch(m_eNaNReplacementPolicy)
						{
							case NaNReplacementPolicy_Disabled:
								m_vSwapBuffer[j] = std::numeric_limits<float>::quiet_NaN();
								break;
							case NaNReplacementPolicy_Zero:
								m_vSwapBuffer[j] = 0;
								break;
							case NaNReplacementPolicy_LastCorrectValue:
								// we simply don't update the value
								break;
							default:
								break;
						}
					}
					else
					{
						m_vSwapBuffer[j]=alpha*pSample[l_ui32Channel*ui32SampleCount+i]+(1-alpha)*m_vOverSamplingSwapBuffer[j];
					}
				}
		
				const uint64 l_ui64CurrentSampleIndex = m_ui64SampleCount + i*m_ui64OverSamplingFactor + k;		// j is not included here as all channels have the equal sample time

				if(l_bHadNaN)
				{
					// When a NaN is encountered at time t1 on any channel, OVTK_GDF_Incorrect stimulus is sent. When a first good sample is encountered 
					// after the last bad sample t2, OVTK_GDF_Correct stimulus is sent, i.e. specifying a range of bad data : [t1,t2]. The stimuli are global 
					// and not specific to channels.

					if(!m_bReplacementInProgress)
					{
						const uint64 l_ui64IncorrectBlockStarts = ITimeArithmetics::sampleCountToTime(m_ui32SamplingFrequency, l_ui64CurrentSampleIndex);

						m_oPendingStimulationSet.appendStimulation(OVTK_GDF_Incorrect, l_ui64IncorrectBlockStarts, 0);
						m_bReplacementInProgress = true;
					}
				} 
				else 
				{
					if(m_bReplacementInProgress)
					{
						// @note -1 is used here because the incorrect-correct range is inclusive, [a,b]. So when sample is good at b+1, we set the end point at b.
						const uint64 l_ui64IncorrectBlockStops = ITimeArithmetics::sampleCountToTime(m_ui32SamplingFrequency, l_ui64CurrentSampleIndex - 1);

						m_oPendingStimulationSet.appendStimulation(OVTK_GDF_Correct, l_ui64IncorrectBlockStops, 0);
						m_bReplacementInProgress = false;
					}
				}

				m_vPendingBuffer.push_back(m_vSwapBuffer);
			}
		}

		m_ui64LastSampleCount=m_ui64SampleCount;
		m_ui64SampleCount+=ui32SampleCount*m_ui64OverSamplingFactor;

		m_oDriftCorrection.estimateDrift(ui32SampleCount*m_ui64OverSamplingFactor);

		m_ui64LastDeliveryTime=System::Time::zgetTime();
		m_bGotData=true;
	}
	else
	{
		m_rKernelContext.getLogManager() << LogLevel_Warning << "The acquisition is not started\n";
	}
}

void CAcquisitionServer::setStimulationSet(const IStimulationSet& rStimulationSet)
{
	if(m_bStarted)
	{
		uint64 l_ui64StimulationTime = ITimeArithmetics::sampleCountToTime(m_ui32SamplingFrequency, m_ui64LastSampleCount);
		OpenViBEToolkit::Tools::StimulationSet::append(m_oPendingStimulationSet, rStimulationSet, l_ui64StimulationTime);
	}
	else
	{
		m_rKernelContext.getLogManager() << LogLevel_Warning << "The acquisition is not started\n";
	}
}


boolean CAcquisitionServer::updateImpedance(const uint32 ui32ChannelIndex, const float64 f64Impedance)
{
	for (size_t i=0;i<m_vSelectedChannels.size();++i)
		if (ui32ChannelIndex==m_vSelectedChannels[i]) {
			m_vImpedance[i] = f64Impedance;
			return true;
		}
	return false;
}

// ____________________________________________________________________________
//

ENaNReplacementPolicy CAcquisitionServer::getNaNReplacementPolicy(void)
{
	return m_eNaNReplacementPolicy;
}

CString CAcquisitionServer::getNaNReplacementPolicyStr(void)
{
	switch (m_eNaNReplacementPolicy)
	{
		case NaNReplacementPolicy_Disabled:
			return CString("Disabled");
		case NaNReplacementPolicy_LastCorrectValue:
			return CString("LastCorrectValue");
		case NaNReplacementPolicy_Zero:
			return CString("Zero");
		default :
			return CString("N/A");
	}
}

uint64 CAcquisitionServer::getOversamplingFactor(void)
{
	return m_ui64OverSamplingFactor;
}

boolean CAcquisitionServer::setNaNReplacementPolicy(ENaNReplacementPolicy eNaNReplacementPolicy)
{
	m_eNaNReplacementPolicy=eNaNReplacementPolicy;
	return true;
}


boolean CAcquisitionServer::isImpedanceCheckRequested(void)
{
	if (m_pDriver)
	{
		return m_pDriver->getHeader()->isImpedanceCheckRequested();
	}
	return false;
}

boolean CAcquisitionServer::isChannelSelectionRequested(void)
{
	return m_bIsChannelSelectionRequested;
}

boolean CAcquisitionServer::setOversamplingFactor(uint64 ui64OversamplingFactor)
{
	m_ui64OverSamplingFactor=ui64OversamplingFactor;
	if(m_ui64OverSamplingFactor<1) m_ui64OverSamplingFactor=1;
	if(m_ui64OverSamplingFactor>16) m_ui64OverSamplingFactor=16;
	return true;
}

boolean CAcquisitionServer::setImpedanceCheckRequest(boolean bActive)
{
	m_bIsImpedanceCheckRequested=bActive;
	return true;
}

boolean CAcquisitionServer::setChannelSelectionRequest(const boolean bActive)
{
	m_bIsChannelSelectionRequested=bActive;
	return true;
}

// ____________________________________________________________________________
//

boolean CAcquisitionServer::acceptNewConnection(Socket::IConnection* pConnection)
{
	if(!pConnection)
	{
		return false;
	}

	const uint64 l_ui64Time=System::Time::zgetTime();

	DoubleLock lock(&m_oPendingConnectionProtectionMutex, &m_oPendingConnectionExecutionMutex);

	SConnectionInfo l_oInfo;
	l_oInfo.m_ui64ConnectionTime=l_ui64Time;
	l_oInfo.m_pConnectionClientHandlerThread=NULL; // not used
	l_oInfo.m_pConnectionClientHandlerStdThread=NULL; // not used
	l_oInfo.m_bChannelLocalisationSent = false;
	l_oInfo.m_bChannelUnitsSent = false;

	m_vPendingConnection.push_back(pair < Socket::IConnection*, SConnectionInfo > (pConnection, l_oInfo));

	for(auto itp = m_vPlugins.begin(); itp != m_vPlugins.end(); ++itp)
	{
		(*itp)->acceptNewConnectionHook();
	}

	return true;
}


