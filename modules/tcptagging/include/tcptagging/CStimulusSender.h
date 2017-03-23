#ifndef __TCPTagging_CStimulusSender_H__
#define __TCPTagging_CStimulusSender_H__

#include "defines.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "IStimulusSender.h"

using boost::asio::ip::tcp;

namespace TCPTagging
{
	/*
	* \class CStimulusSender
	* \author Jussi T. Lindgren / Inria
	* \brief Simple client to send stimuli to Acquisition Server's TCP Tagging
	*/
	class CStimulusSender : public IStimulusSender {
	public:

		CStimulusSender(void)
			: m_oStimulusSocket(m_ioService), m_bConnectedOnce(false) { }
		virtual ~CStimulusSender();

		// Connect to the TCP Tagging plugin of the Acquisition Server
		virtual TCPTagging::boolean connect(const char* sAddress, const char* sStimulusPort);

		// Send a stimulation. Set Timestamp to 0 for immediate tagging (also the default).
		virtual TCPTagging::boolean sendStimulation(TCPTagging::uint64 ui64Stimulation, TCPTagging::uint64 ui64Timestamp = 0) ;

	protected:

		boost::asio::io_service m_ioService;

		tcp::socket m_oStimulusSocket;

		TCPTagging::boolean m_bConnectedOnce;

	};

	// Tentative name
	class CStimulusMultiSender : public IStimulusMultiSender {
	public :
		CStimulusMultiSender(void)
			: m_uiGtkExecutionIndex(0)
		{
		}
		virtual ~CStimulusMultiSender() {}

		virtual TCPTagging::boolean connect(const char* sAddress, const char* sStimulusPort)
		{
			return m_sender.connect(sAddress, sStimulusPort);
		}

		virtual void addStimulationToWaitingList(TCPTagging::uint64 ui64Stimulation, TCPTagging::uint64 ui64Timestamp = 0)
		{
			m_vStimuliQueue.push_back(ui64Stimulation);
		}

		virtual TCPTagging::boolean sendStimulations(void)
		{
			TCPTagging::boolean res = true;
			for (const TCPTagging::uint64& stim : m_vStimuliQueue)
			{
				res &= m_sender.sendStimulation(stim);
			}
			m_vStimuliQueue.clear();
			m_uiGtkExecutionIndex = 0;
			return res;
		}

		virtual TCPTagging::boolean sendStimulationNow(TCPTagging::uint64 ui64Stimulation, TCPTagging::uint64 ui64Timestamp = 0)
		{
			return m_sender.sendStimulation(ui64Stimulation, ui64Timestamp);
		}

		virtual bool isCurrentlySending(void)
		{
			return m_uiGtkExecutionIndex != 0;
		}

		virtual unsigned int getExecutionIndex(void)
		{
			return m_uiGtkExecutionIndex;
		}

		virtual void setExecutionIndex(unsigned int index) 
		{ 
			m_uiGtkExecutionIndex = index; 
		}

	protected :
		std::vector< TCPTagging::uint64 > m_vStimuliQueue;
	    unsigned int m_uiGtkExecutionIndex;
		CStimulusSender m_sender;
	};
}

#endif
