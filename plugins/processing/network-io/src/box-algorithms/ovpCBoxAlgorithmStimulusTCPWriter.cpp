#include "ovpCBoxAlgorithmStimulusTCPWriter.h"

#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::NetworkIO;

using boost::asio::ip::tcp;

void CBoxAlgorithmStimulusTCPWriter::startAccept() { 
	boost::asio::ip::tcp::socket* l_pSocket = new boost::asio::ip::tcp::socket(m_pAcceptor->get_io_service());
	
	// Since startAccept will only be called inside ioService.poll(), there is no need to access control m_vSockets
	m_vSockets.push_back(l_pSocket);

	this->getLogManager() << LogLevel_Debug << "We are now using " << m_vSockets.size() << " socket(s)\n";

	m_pAcceptor->async_accept(*l_pSocket, 
		boost::bind(&CBoxAlgorithmStimulusTCPWriter::handleAccept, this, boost::asio::placeholders::error));
}

void CBoxAlgorithmStimulusTCPWriter::handleAccept(const boost::system::error_code& ec) { 
	if(!ec) {
		this->getLogManager() << LogLevel_Debug << "Handling a new incoming connection\n";
	} else {
		// @fixme should the socket be closed in this case?
		this->getLogManager() << LogLevel_Warning << "Issue '" << ec.message().c_str() << "' with accepting a connection.\n";
	}
	// Already schedule the accepting of the next connection
	startAccept();
} 

boolean CBoxAlgorithmStimulusTCPWriter::initialize(void)
{
	// Stimulation stream decoder
	m_oAlgo0_StimulationDecoder.initialize(*this);
	
	uint64 l_ui64Port = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

	m_ui64OutputStyle = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

	m_pAcceptor = new tcp::acceptor(m_oIOService, tcp::endpoint(tcp::v4(), (uint32)l_ui64Port));

	startAccept();

	m_oIOService.poll();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmStimulusTCPWriter::uninitialize(void)
{
	m_oAlgo0_StimulationDecoder.uninitialize();

	m_oIOService.stop();

	for(uint32 i=0;i<m_vSockets.size();i++) {
		delete m_vSockets[i];
	}
	m_vSockets.clear();

	delete m_pAcceptor;
	m_pAcceptor = NULL;
	
	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmStimulusTCPWriter::processInput(uint32 ui32InputIndex)
{
	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmStimulusTCPWriter::process(void)
{
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	// Process the asio loop once (e.g. see if there's new connections)
	m_oIOService.poll();

	// Collect the received stimuli
	std::vector<uint64> l_sStims;
	for(uint32 j=0; j<l_rDynamicBoxContext.getInputChunkCount(0); j++)
	{
		m_oAlgo0_StimulationDecoder.decode(0,j);
		if(m_oAlgo0_StimulationDecoder.isHeaderReceived())
		{
		}
		if(m_oAlgo0_StimulationDecoder.isBufferReceived()) 
		{
			IStimulationSet* l_pStimulations = m_oAlgo0_StimulationDecoder.getOutputStimulationSet();
			for(uint32 j=0; j<l_pStimulations->getStimulationCount(); j++)
			{
				uint64 l_ui64StimulationCode = l_pStimulations->getStimulationIdentifier(j);
				// uint64 l_ui64StimulationDate = l_pStimulations->getStimulationDate(j);
				l_sStims.push_back(l_ui64StimulationCode);
				
			}
		}
		if(m_oAlgo0_StimulationDecoder.isEndReceived())
		{
		}
	}

	// Write the stimuli to all the listeners. 
	// @note This uses blocking writes, so if any connection is slow, this will cause the box to lag. 
	// An alternative would be to keep stim buffers per connection and use async writes.
	std::vector<boost::asio::ip::tcp::socket*>::iterator it = m_vSockets.begin();
	while(it!=m_vSockets.end()) {
		boost::asio::ip::tcp::socket* tmpSock = (*it);
		bool hadError = false;
		if(tmpSock->is_open()) {
			for(uint32 i=0;i<l_sStims.size() && !hadError;i++) 
			{
				boost::system::error_code ec;
				
				switch(m_ui64OutputStyle) {
					case TCPWRITER_RAW:
						boost::asio::write(*tmpSock, boost::asio::buffer((void*)&l_sStims[i],sizeof(l_sStims[i])), ec);
						break;
					case TCPWRITER_HEX:
						{
						CString  l_sTmp = CIdentifier(l_sStims[i]).toString() + CString("\n");
						const char *ptr = l_sTmp.toASCIIString();
						boost::asio::write(*tmpSock, boost::asio::buffer(ptr,strlen(ptr)), ec);
						}
						break;
					case TCPWRITER_STRING: 
						{
						CString l_sTmp = this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, l_sStims[i]) + CString("\n");
						const char *ptr = l_sTmp.toASCIIString();
						boost::asio::write(*tmpSock, boost::asio::buffer(ptr,strlen(ptr)), ec);
						}
						break;
					default:
						this->getLogManager() << LogLevel_Error << "Unknown output type\n";
						break;
				}

				if(ec) {
					this->getLogManager() << LogLevel_Warning << "Got error '" << ec.message().c_str() << "' while trying to write to socket\n";
					hadError = true;
				}
			}
		} 
		if(hadError) {
			// Close the socket
			this->getLogManager() << LogLevel_Debug << "Closing the socket\n";
			tmpSock->shutdown(boost::asio::socket_base::shutdown_both);
			tmpSock->close();
			m_oIOService.poll();
			delete tmpSock;
			it = m_vSockets.erase(it);
		} else {
			it++;
		}
	}

	return true;
}
