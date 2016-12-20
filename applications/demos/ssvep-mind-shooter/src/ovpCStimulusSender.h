
// @TODO refactor ALL instances of this class to a single one in a specific place

#include <openvibe/ov_types.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

using boost::asio::ip::tcp;

namespace OpenViBEPlugins
{
	namespace SimpleVisualisation
	{
		/*
		 * \class StimulusSender
		 * \author Jussi T. Lindgren / Inria
		 * \brief Simple client to send stimuli to Acquisition Server's TCP Tagging
		 */
		class StimulusSender {
		public:

			StimulusSender(void)
				: m_oStimulusSocket(m_ioService), m_bConnectedOnce(false) { }

			~StimulusSender();

			// Connect to the TCP Tagging plugin of the Acquisition Server
			OpenViBE::boolean connect(const char* sAddress, const char* sStimulusPort);

			// Send a stimulation
			OpenViBE::boolean sendStimulation(OpenViBE::uint64 ui64Stimulation) ;

		protected:

			boost::asio::io_service m_ioService;

			tcp::socket m_oStimulusSocket;

			OpenViBE::boolean m_bConnectedOnce;
		};
	}
}
