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
}

#endif
