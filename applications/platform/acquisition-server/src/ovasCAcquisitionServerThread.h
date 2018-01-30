#ifndef __OpenViBE_AcquisitionServer_CAcquisitionServerThread_H__
#define __OpenViBE_AcquisitionServer_CAcquisitionServerThread_H__

#include "ovasCAcquisitionServerGUI.h"
#include "ovasCAcquisitionServer.h"

#include <system/ovCTime.h>
#include <cmath> // std::abs() for Fedora

// fwd declare of gtk idle callbacks
static gboolean idle_updateimpedance_cb(void* pUserData);
static gboolean idle_updatedrift_cb(void* pUserData);
static gboolean idle_updatedisconnect_cb(void* pUserData);
static gboolean idle_updatestatus_cb(void* pUserData);

namespace OpenViBEAcquisitionServer
{
	class CAcquisitionServerThread
	{
	public:

		enum
		{
			Status_Idle,
			Status_Connected,
			Status_Started,
			Status_Finished
		};

		CAcquisitionServerThread(const OpenViBE::Kernel::IKernelContext& rKernelContext, CAcquisitionServerGUI& rGUI, CAcquisitionServer& rAcquisitionServer)
			:m_rKernelContext(rKernelContext)
			, m_rGUI(rGUI)
			, m_rAcquisitionServer(rAcquisitionServer)
			, m_ui32Status(Status_Idle)
			, m_ui32LastStatus(-1)	// Always call update on start
			, m_ui32ClientCount(0)
			, m_f64LastDriftMs(-1)
		{
		}

		void main(void)
		{
			OpenViBE::boolean l_bFinished = false;
			OpenViBE::uint64 l_ui64LastGUIUpdate = 0;

			while (!l_bFinished)
			{
				OpenViBE::boolean l_bShouldSleep = false;
				OpenViBE::boolean l_bShouldDisconnect = false;
				OpenViBE::uint32 i, l_ui32ClientCount;
				OpenViBE::float64 l_f64DriftMs;

				{
					DoubleLock lock(&m_rAcquisitionServer.m_oProtectionMutex, &m_rAcquisitionServer.m_oExecutionMutex);

					l_ui32ClientCount = m_rAcquisitionServer.getClientCount();
					l_f64DriftMs = (m_ui32Status == Status_Started ? m_rAcquisitionServer.m_oDriftCorrection.getDriftMs() : 0);


					switch (m_ui32Status)
					{
						case Status_Idle:
							l_bShouldSleep = true;
							break;

						case Status_Connected:
						case Status_Started:
							{
								if (!m_rAcquisitionServer.loop())
								{
									l_bShouldDisconnect = true;
								}
								else
								{
									for (i = 0; i < m_vImpedance.size(); i++)
									{
										m_vImpedance[i] = m_rAcquisitionServer.getImpedance(i);
									}
								}
							}
							break;

						case Status_Finished:
							l_bFinished = true;
							break;

						default:
							break;
					}
				}

				if (!l_bFinished)
				{	
					if(m_rAcquisitionServer.isImpedanceCheckRequested())
					{
						// In the impedance check mode, we need to yield the thread to get reliable redraw
						l_bShouldSleep = true;
					}

					// Update the GUI if the variables have changed. In order to avoid 
					// gdk_threads_enter()/gdk_threads_exit() calls that may not work on all 
					// backends (esp. Windows), delegate the work to g_idle_add() functions.
					// As a result, we need to protect access to the variables that the callbacks use;
					// this protection is done inside the callbacks.
					if( !std::equal(m_vImpedance.begin(), m_vImpedance.end(), m_vImpedanceLast.begin(), 
						[](OpenViBE::float64 a, OpenViBE::float64 b) { return (std::abs(a-b)<0.01); }) ) 
					{
						m_vImpedanceLast = m_vImpedance;

						gdk_threads_add_idle(idle_updateimpedance_cb, (void *)this);
					}

					if (m_ui32LastStatus != m_ui32Status || l_ui32ClientCount != m_ui32ClientCount)
					{
						m_ui32LastStatus = m_ui32Status;
						m_ui32ClientCount = l_ui32ClientCount;
						gdk_threads_add_idle(idle_updatestatus_cb, (void *)this);
					}

					if (l_bShouldDisconnect)
					{
						gdk_threads_add_idle(idle_updatedisconnect_cb, (void *)this);
					}

					// update fast changing variables at most every 0.25 sec to avoid hammering the gui
					const OpenViBE::uint64 l_ui64TimeNow = System::Time::zgetTime();
					if(l_ui64TimeNow - l_ui64LastGUIUpdate > (1LL<<32)/4LL ) 
					{
						if (std::abs(l_f64DriftMs-m_f64LastDriftMs) > 0)
						{
							m_f64LastDriftMs = l_f64DriftMs;

							 gdk_threads_add_idle(idle_updatedrift_cb, (void *)this);
						}

						l_ui64LastGUIUpdate = l_ui64TimeNow;
					}

					if (l_bShouldSleep)
					{
						System::Time::sleep(100);
					}
				}
			}

			m_rKernelContext.getLogManager() << OpenViBE::Kernel::LogLevel_Trace << "CAcquisitionServerThread:: thread finished\n";
		}

		OpenViBE::boolean connect(void)
		{
			DoubleLock lock(&m_rAcquisitionServer.m_oProtectionMutex, &m_rAcquisitionServer.m_oExecutionMutex);

			m_rKernelContext.getLogManager() << OpenViBE::Kernel::LogLevel_Trace << "CAcquisitionServerThread::connect()\n";

			if(!m_rAcquisitionServer.connect(m_rGUI.getDriver(), m_rGUI.getHeaderCopy(), m_rGUI.getSampleCountPerBuffer(), m_rGUI.getTCPPort()))
			{
				return false;
			}
			else
			{
				m_ui32Status=Status_Connected;
			}

			{
				m_vImpedance.resize(m_rGUI.getHeaderCopy().getChannelCount(), OVAS_Impedance_NotAvailable);
				m_vImpedanceLast.resize(m_rGUI.getHeaderCopy().getChannelCount(), OVAS_Impedance_NotAvailable);
			}
			return true;
		}

		OpenViBE::boolean start(void)
		{
			DoubleLock lock(&m_rAcquisitionServer.m_oProtectionMutex, &m_rAcquisitionServer.m_oExecutionMutex);

			m_rKernelContext.getLogManager() << OpenViBE::Kernel::LogLevel_Trace << "CAcquisitionServerThread::start()\n";
			if(!m_rAcquisitionServer.start())
			{
				m_rAcquisitionServer.stop();
				return false;
			}
			else
			{
				m_ui32Status=Status_Started;
			}
			return true;
		}

		OpenViBE::boolean stop(void)
		{
			DoubleLock lock(&m_rAcquisitionServer.m_oProtectionMutex, &m_rAcquisitionServer.m_oExecutionMutex);

			m_rKernelContext.getLogManager() << OpenViBE::Kernel::LogLevel_Trace << "CAcquisitionServerThread::stop()\n";
			m_rAcquisitionServer.stop();
			m_ui32Status=Status_Connected;
			return true;
		}

		OpenViBE::boolean disconnect(void)
		{
			DoubleLock lock(&m_rAcquisitionServer.m_oProtectionMutex, &m_rAcquisitionServer.m_oExecutionMutex);

			m_rKernelContext.getLogManager() << OpenViBE::Kernel::LogLevel_Trace << "CAcquisitionServerThread::disconnect()\n";

			if(m_ui32Status==Status_Started)
			{
				m_rAcquisitionServer.stop();
			}

			m_rAcquisitionServer.disconnect();

			m_vImpedance.clear();
			m_vImpedanceLast.clear();

			m_ui32Status=Status_Idle;
			return true;
		}

		OpenViBE::boolean terminate(void)
		{
			DoubleLock lock(&m_rAcquisitionServer.m_oProtectionMutex, &m_rAcquisitionServer.m_oExecutionMutex);

			m_rKernelContext.getLogManager() << OpenViBE::Kernel::LogLevel_Trace << "CAcquisitionServerThread::terminate()\n";

			switch(m_ui32Status)
			{
				case Status_Started:
					m_rKernelContext.getLogManager() << OpenViBE::Kernel::LogLevel_Trace << "CAcquisitionServerThread::stop()\n";
					m_rAcquisitionServer.stop();
				case Status_Connected:
					m_rKernelContext.getLogManager() << OpenViBE::Kernel::LogLevel_Trace << "CAcquisitionServerThread::disconnect()\n";
					m_rAcquisitionServer.disconnect();
			}

			m_ui32Status=Status_Finished;
			return true;
		}

		// GTK C callbacks call these from the main thread to update the GUI
		void updateGUIImpedance(void)
		{
			DoubleLock lock(&m_rAcquisitionServer.m_oProtectionMutex, &m_rAcquisitionServer.m_oExecutionMutex);

			for (size_t i = 0; i < m_vImpedanceLast.size(); i++)
			{
				m_rGUI.setImpedance(i, m_vImpedanceLast[i]);
			}
		}

		void updateGUIDrift(void)
		{
			DoubleLock lock(&m_rAcquisitionServer.m_oProtectionMutex, &m_rAcquisitionServer.m_oExecutionMutex);

			m_rGUI.setDriftMs(m_f64LastDriftMs);
		}

		void updateGUIStatus(void)
		{
			DoubleLock lock(&m_rAcquisitionServer.m_oProtectionMutex, &m_rAcquisitionServer.m_oExecutionMutex);

			std::string l_sState;
			std::string l_sClientText;

			switch(m_ui32Status)
			{
				case Status_Started:
					if(m_ui32ClientCount==0)
					{
						l_sState = "Receiving...";
					}
					else
					{
						l_sState = "Receiving and sending...";
					}
					char l_sLabel[1024];
					::sprintf(l_sLabel, "%u client%s connected", static_cast<unsigned int>(m_ui32ClientCount), (m_ui32ClientCount!=1 ? "s" : ""));
					l_sClientText = l_sLabel;
					break;
				case Status_Idle:
					l_sState = "Disconnected";
					break;
				case Status_Connected:
					l_sState = "Connected to device!";
					l_sClientText = "Press play to start...";

					break;
				default:
					l_sState = "Unknown";
					break;
			}
			m_rGUI.setStateText(l_sState.c_str());
			m_rGUI.setClientText(l_sClientText.c_str());
		}

		void updateGUIDisconnect(void)
		{
			DoubleLock lock(&m_rAcquisitionServer.m_oProtectionMutex, &m_rAcquisitionServer.m_oExecutionMutex);

			m_rGUI.disconnect();
		}

/*
		uint32 getStatus(void)
		{
			DoubleLock lock(&m_rAcquisitionServer.m_oProtectionMutex, &m_rAcquisitionServer.m_oExecutionMutex);	

			return m_ui32Status;
		}
*/
	protected:

		const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
		OpenViBEAcquisitionServer::CAcquisitionServerGUI& m_rGUI;
		OpenViBEAcquisitionServer::CAcquisitionServer& m_rAcquisitionServer;
		OpenViBE::uint32 m_ui32Status;
		OpenViBE::uint32 m_ui32LastStatus;

		OpenViBE::uint32 m_ui32ClientCount;
		OpenViBE::float64 m_f64LastDriftMs;
		std::vector < OpenViBE::float64 > m_vImpedanceLast;
		std::vector < OpenViBE::float64 > m_vImpedance;
	};

	class CAcquisitionServerThreadHandle
	{
	public:

		CAcquisitionServerThreadHandle(OpenViBEAcquisitionServer::CAcquisitionServerThread& rAcquisitionServerThread)
			:m_rAcquisitionServerThread(rAcquisitionServerThread)
		{
		}

		void operator() (void)
		{
			m_rAcquisitionServerThread.main();
		}

	protected:

		OpenViBEAcquisitionServer::CAcquisitionServerThread& m_rAcquisitionServerThread;
	};
}

static gboolean idle_updateimpedance_cb(void* pUserData)
{
	OpenViBEAcquisitionServer::CAcquisitionServerThread* l_pPtr = static_cast<OpenViBEAcquisitionServer::CAcquisitionServerThread*>(pUserData);

	l_pPtr->updateGUIImpedance();

	return FALSE; // don't call again
}

static gboolean idle_updatedrift_cb(void* pUserData)
{
	OpenViBEAcquisitionServer::CAcquisitionServerThread* l_pPtr = static_cast<OpenViBEAcquisitionServer::CAcquisitionServerThread*>(pUserData);

	l_pPtr->updateGUIDrift();

	return FALSE; // don't call again
}

static gboolean idle_updatestatus_cb(void* pUserData)
{
	OpenViBEAcquisitionServer::CAcquisitionServerThread* l_pPtr = static_cast<OpenViBEAcquisitionServer::CAcquisitionServerThread*>(pUserData);

	l_pPtr->updateGUIStatus();

	return FALSE; // don't call again
}

static gboolean idle_updatedisconnect_cb(void* pUserData)
{
	OpenViBEAcquisitionServer::CAcquisitionServerThread* l_pPtr = static_cast<OpenViBEAcquisitionServer::CAcquisitionServerThread*>(pUserData);

	l_pPtr->updateGUIDisconnect();

	return FALSE; // don't call again
}


#endif // __OpenViBE_AcquisitionServer_CAcquisitionServerThread_H__
