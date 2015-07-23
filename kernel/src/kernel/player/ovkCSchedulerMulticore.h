#ifndef __OpenViBEKernel_Kernel_Scheduler_CSchedulerMulticore_H__
#define __OpenViBEKernel_Kernel_Scheduler_CSchedulerMulticore_H__

#include "ovkCScheduler.h"

#include <map>
#include <list>
#include <deque>

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/version.hpp>

namespace OpenViBE
{
	namespace Kernel
	{
		class CSimulatedBox;
		class CChunk;
		class CPlayer;

		// typedef std::pair<boolean (*)(void*), void*> jobInfo;
		typedef boost::function<void()> jobCall;

		class JobContext {
		public:
			JobContext() : m_ui32JobsPending(0), m_bQuit(false) { };

			boost::mutex m_oJobMutex;
		//	boost::mutex m_oJobsPendingMutex;

			std::deque<jobCall> m_vJobList;
			// std::deque<jobInfo> m_vJobList;
			OpenViBE::uint32 m_ui32JobsPending;

			boost::condition_variable m_oHaveWork;
			boost::condition_variable m_oJobDone;
	
			OpenViBE::boolean m_bQuit;
		};

		class CWorkerThread {
		public:
			CWorkerThread(JobContext& ctx) : m_ctx(ctx) { };
	
			JobContext& m_ctx;

			// void operator()(void)
			void run(void)
			{

				while(true)
				{
					jobCall job;
				//	jobInfo job((boolean (*)(void*))NULL,(void*)NULL);

					// Wait until we get a job or are told to quit
					{ // scope for lock
						boost::unique_lock<boost::mutex> lock(m_ctx.m_oJobMutex);

						while(!m_ctx.m_bQuit && m_ctx.m_vJobList.size()==0)
						{
							m_ctx.m_oHaveWork.wait(lock);
						}

						if(m_ctx.m_bQuit) {
							return;
						}

						// Ok, we have a job
						job = m_ctx.m_vJobList.front();
						m_ctx.m_vJobList.pop_front();
					}

					// do job
					// std::cout << "Hello " << job << " ( " << boost::this_thread::get_id() << ")\n";
			
//					std::cout << "Thread " << boost::this_thread::get_id() << " enter job\n";

					// @todo pass job failed to caller
					job();
					// (*job.first)(job.second);

//					std::cout << "Thread " << boost::this_thread::get_id() << " exit job\n";

					// job done, mark as done
					{ // scope for lock
						boost::unique_lock<boost::mutex> lock(m_ctx.m_oJobMutex);	
						m_ctx.m_ui32JobsPending--;
					}

					m_ctx.m_oJobDone.notify_one();
				}
			}

			static void startWorkerThread(CWorkerThread* pThread)
			{
				pThread->run();
				// (*pThread)();
			}

		};


		class parallelExecutor
		{
		public:
			parallelExecutor() { };	
			boolean initialize(const uint32 nThreads);
			boolean push(const jobCall& someJob);							// add job, pass to threads
			boolean pushList(const std::deque<jobCall>& vJobList);			// add jobs
			boolean waitForAll(void);						// wait until all pushed jobs are done
			boolean uninitialize();

		private:

			JobContext m_oJobContext;

			std::vector<CWorkerThread*> m_vWorkerThread;
			std::vector<boost::thread*> m_vThread;

		};



		class CSchedulerMulticore : public OpenViBE::Kernel::CScheduler
		{
		public:

			CSchedulerMulticore(const OpenViBE::Kernel::IKernelContext& rKernelContext, OpenViBE::Kernel::CPlayer& rPlayer);
			virtual ~CSchedulerMulticore(void);

			virtual OpenViBE::boolean setScenario(
				const OpenViBE::CIdentifier& rScenarioIdentifier);
			virtual OpenViBE::boolean setFrequency(
				const OpenViBE::uint64 ui64Frequency);

			virtual SchedulerInitializationCode initialize(void);
			virtual OpenViBE::boolean uninitialize(void);

			virtual OpenViBE::boolean loop(void);

			virtual OpenViBE::boolean sendInput(const OpenViBE::Kernel::CChunk& rChunk, const OpenViBE::CIdentifier& rBoxIdentifier, const OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::uint64 getCurrentTime(void) const;
			virtual OpenViBE::uint64 getFrequency(void) const;
			virtual OpenViBE::float64 getCPUUsage(void) const;

			virtual OpenViBE::boolean sendMessage(const IMessageWithData &msg, CIdentifier targetBox, uint32 inputIndex);

			static OpenViBE::boolean job(CSchedulerMulticore *context, CIdentifier id, CSimulatedBox* box);
			OpenViBE::boolean runBox(CIdentifier id, CSimulatedBox* box);

			_IsDerivedFromClass_Final_(OpenViBE::Kernel::TKernelObject < OpenViBE::Kernel::IKernelObject >, OVK_ClassId_Kernel_Player_Scheduler);

			CPlayer& getPlayer(void)
			{
				return m_rPlayer;
			}

		protected:

			OpenViBE::Kernel::CPlayer& m_rPlayer;
			OpenViBE::CIdentifier m_oScenarioIdentifier;
			OpenViBE::Kernel::IScenario* m_pScenario;
			OpenViBE::uint64 m_ui64Steps;
			OpenViBE::uint64 m_ui64Frequency;
			OpenViBE::uint64 m_ui64CurrentTime;
			OpenViBE::boolean m_bIsInitialized;
			OpenViBE::Kernel::parallelExecutor m_oExecutor;

			std::map < std::pair < OpenViBE::int32, OpenViBE::CIdentifier>, OpenViBE::Kernel::CSimulatedBox* > m_vSimulatedBox;
			std::map < OpenViBE::CIdentifier, System::CChrono > m_vSimulatedBoxChrono;
			std::map < OpenViBE::CIdentifier, std::map < OpenViBE::uint32, std::list < OpenViBE::Kernel::CChunk > > > m_vSimulatedBoxInput;
			std::map < OpenViBE::CIdentifier, boost::mutex* > m_vSimulatedBoxInputMutex;

		private:

			System::CChrono m_oBenchmarkChrono;
		};
	};
};

#endif // __OpenViBEKernel_Kernel_Scheduler_CSchedulerMulticore_H__
