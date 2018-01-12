#ifndef __OpenViBE_AcquisitionServer_CAcquisitionServer_H__
#define __OpenViBE_AcquisitionServer_CAcquisitionServer_H__

#include "ovas_base.h"
#include "ovasIDriver.h"
#include "ovasIHeader.h"
#include "ovasCDriftCorrection.h"

#include <socket/IConnectionServer.h>

#include <mutex>
#include <thread>

#include <string>
#include <vector>
#include <list>
#include <deque>


namespace OpenViBEAcquisitionServer
{
	class CConnectionServerHandlerThread;
	class CConnectionClientHandlerThread;
	class IAcquisitionServerPlugin;

	typedef struct
	{
		OpenViBE::uint64 m_ui64ConnectionTime;                              // Time the client connected
		OpenViBE::uint64 m_ui64StimulationTimeOffset;                       // Time offset wrt acquisition start
		OpenViBE::uint64 m_ui64SignalSampleCountToSkip;                     // How many samples to skip wrt current buffer start. n.b. not a constant.
		CConnectionClientHandlerThread* m_pConnectionClientHandlerThread;  // Ptr to the class object that is executed by the client connection handler thread
		std::thread* m_pConnectionClientHandlerStdThread;                  // The actual thread handle
		bool m_bChannelUnitsSent; 
		bool m_bChannelLocalisationSent;
	} SConnectionInfo;

	typedef enum
	{
		NaNReplacementPolicy_LastCorrectValue=0,
		NaNReplacementPolicy_Zero,
		NaNReplacementPolicy_Disabled,
	} ENaNReplacementPolicy;

	// Concurrency handling
	class DoubleLock {
		// Implements
		//   lock(mutex1);
		//   lock(mutex2);
		//   unlock(mutex1);
		// mutex2 lock is released when the object goes out of scope
		//
		// @details The two mutex 'pattern' is used to avoid thread starving which can happen e.g.
		// on Linux if just a single mutex is used; apparently the main loop just takes the 
		// mutex repeatedly without the gui thread sitting on the mutex being unlocked.
		// n.b. potentially calls for redesign
	public:
		DoubleLock(std::mutex* m1, std::mutex* m2) : lock2(*m2, std::defer_lock)
		{
			std::lock_guard<std::mutex> lock1(*m1);
			lock2.lock();
		}
	private:
		std::unique_lock<std::mutex> lock2;
	};

	class CDriverContext;
	class CAcquisitionServer : public OpenViBEAcquisitionServer::IDriverCallback
	{
	public:

		CAcquisitionServer(const OpenViBE::Kernel::IKernelContext& rKernelContext);
		virtual ~CAcquisitionServer(void);

		virtual OpenViBEAcquisitionServer::IDriverContext& getDriverContext();

		OpenViBE::uint32 getClientCount(void);
		OpenViBE::float64 getImpedance(const OpenViBE::uint32 ui32ChannelIndex);

		OpenViBE::boolean loop(void);

		OpenViBE::boolean connect(OpenViBEAcquisitionServer::IDriver& rDriver, OpenViBEAcquisitionServer::IHeader& rHeaderCopy, OpenViBE::uint32 ui32SamplingCountPerSentBlock, OpenViBE::uint32 ui32ConnectionPort);
		OpenViBE::boolean start(void);
		OpenViBE::boolean stop(void);
		OpenViBE::boolean disconnect(void);

		// Driver samples information callback
		virtual void setSamples(const OpenViBE::float32* pSample);
		virtual void setSamples(const OpenViBE::float32* pSample, const OpenViBE::uint32 ui32SampleCount);
		virtual void setStimulationSet(const OpenViBE::IStimulationSet& rStimulationSet);

		// Driver context callback
		virtual OpenViBE::boolean isConnected(void) const { return m_bInitialized; }
		virtual OpenViBE::boolean isStarted(void) const { return m_bStarted; }
		virtual OpenViBE::boolean updateImpedance(const OpenViBE::uint32 ui32ChannelIndex, const OpenViBE::float64 f64Impedance);

		// General parameters configurable from the GUI
		OpenViBEAcquisitionServer::ENaNReplacementPolicy getNaNReplacementPolicy(void);
		OpenViBE::CString getNaNReplacementPolicyStr(void);
		OpenViBE::uint64 getOversamplingFactor(void);
		OpenViBE::boolean setNaNReplacementPolicy(OpenViBEAcquisitionServer::ENaNReplacementPolicy eNaNReplacementPolicy);
		OpenViBE::boolean isImpedanceCheckRequested(void);
		OpenViBE::boolean isChannelSelectionRequested(void);
		OpenViBE::boolean setOversamplingFactor(OpenViBE::uint64 ui64OversamplingFactor);
		OpenViBE::boolean setImpedanceCheckRequest(OpenViBE::boolean bActive);
		OpenViBE::boolean setChannelSelectionRequest(OpenViBE::boolean bActive);

		std::vector<IAcquisitionServerPlugin*> getPlugins() { return m_vPlugins; }

		//
		virtual OpenViBE::boolean acceptNewConnection(Socket::IConnection* pConnection);

	protected:

		bool requestClientThreadQuit(CConnectionClientHandlerThread* th);

	public:

		// See class DoubleLock
		std::mutex m_oProtectionMutex;
		std::mutex m_oExecutionMutex;

		std::mutex m_oPendingConnectionProtectionMutex;
		std::mutex m_oPendingConnectionExecutionMutex;

		std::thread* m_pConnectionServerHandlerStdThread;

	public:

		const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		OpenViBEAcquisitionServer::CDriverContext* m_pDriverContext;
		OpenViBEAcquisitionServer::IDriver* m_pDriver;
		const OpenViBEAcquisitionServer::IHeader* m_pHeaderCopy;

		OpenViBE::Kernel::IAlgorithmProxy* m_pStreamEncoder;
		OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64SubjectIdentifier;
		OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64SubjectAge;
		OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64SubjectGender;
		OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > ip_pSignalMatrix;
		OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > ip_pChannelUnits;
		OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64SignalSamplingRate;
		OpenViBE::Kernel::TParameterHandler < OpenViBE::IStimulationSet* > ip_pStimulationSet;
		OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > ip_ui64BufferDuration;
		OpenViBE::Kernel::TParameterHandler < OpenViBE::IMemoryBuffer* > op_pEncodedMemoryBuffer;

		OpenViBE::Kernel::TParameterHandler < OpenViBE::boolean > ip_bEncodeChannelLocalisationData;
		OpenViBE::Kernel::TParameterHandler < OpenViBE::boolean > ip_bEncodeChannelUnitData;

		std::list < std::pair < Socket::IConnection*, SConnectionInfo > > m_vConnection;
		std::list < std::pair < Socket::IConnection*, SConnectionInfo > > m_vPendingConnection;
		std::deque < std::vector < OpenViBE::float32 > > m_vPendingBuffer;
		std::vector < OpenViBE::float32 > m_vSwapBuffer;
		std::vector < OpenViBE::float32 > m_vOverSamplingSwapBuffer;
		std::vector < OpenViBE::float64 > m_vImpedance;
		std::vector < OpenViBE::uint32 >  m_vSelectedChannels;
		std::vector < OpenViBE::CString >  m_vSelectedChannelNames;
		Socket::IConnectionServer* m_pConnectionServer;

		OpenViBEAcquisitionServer::ENaNReplacementPolicy m_eNaNReplacementPolicy;
		OpenViBE::boolean m_bReplacementInProgress;

		OpenViBE::boolean m_bInitialized;
		OpenViBE::boolean m_bStarted;
		OpenViBE::boolean m_bIsImpedanceCheckRequested;
		OpenViBE::boolean m_bIsChannelSelectionRequested;
		OpenViBE::boolean m_bGotData;
		OpenViBE::uint64 m_ui64OverSamplingFactor;
		OpenViBE::uint32 m_ui32ChannelCount;
		OpenViBE::uint32 m_ui32SamplingFrequency;
		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;
		OpenViBE::uint64 m_ui64SampleCount;
		OpenViBE::uint64 m_ui64LastSampleCount;
		OpenViBE::uint64 m_ui64StartTime;
		OpenViBE::uint64 m_ui64LastDeliveryTime;

		CDriftCorrection m_oDriftCorrection;

		OpenViBE::uint64 m_ui64JitterEstimationCountForDrift;
		OpenViBE::uint64 m_ui64DriverTimeoutDuration;               // ms after which the driver is considered having time-outed
		OpenViBE::int64 m_i64StartedDriverSleepDuration;            // ms, <0 == spin, 0 == yield thread, >0 sleep. Used when driver does not return samples.
		OpenViBE::uint64 m_ui64StoppedDriverSleepDuration;          // ms to sleep when driver is not running

		OpenViBE::uint8* m_pSampleBuffer;
		OpenViBE::CStimulationSet m_oPendingStimulationSet;

		std::vector<IAcquisitionServerPlugin*> m_vPlugins;
	};
};

#endif // __OpenViBE_AcquisitionServer_CAcquisitionServer_H__
