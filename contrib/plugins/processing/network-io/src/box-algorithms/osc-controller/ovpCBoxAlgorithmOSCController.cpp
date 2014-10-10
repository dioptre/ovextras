#include "ovpCBoxAlgorithmOSCController.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::NetworkIO;

OpenViBE::boolean CBoxAlgorithmOSCController::initialize(void)
{
	CString l_sServerAddress = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	uint64 l_ui64ServerPort = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext = this->getStaticBoxContext();

	// Number of inputs
	l_ui32InputCount = l_rStaticBoxContext.getInputCount();

	// Save the names of inputs
	for (uint32 i = 0; i < l_ui32InputCount; ++i) {
	    CString l_sInputName;
	    l_rStaticBoxContext.getInputName(i, l_sInputName);
	    m_pInputNames.push_back(l_sInputName.toASCIIString());
    }
	
	// Connect the socket (FIXME: Is the cast correct?)
	m_oUdpSocket.connectTo(static_cast<std::string>(l_sServerAddress.toASCIIString()), l_ui64ServerPort);

	if (!m_oUdpSocket.isOk()) {
		return false;
	}

	// Signal stream decoder
	m_oSignalDecoder.initialize(*this);
	
	return true;
}

OpenViBE::boolean CBoxAlgorithmOSCController::uninitialize(void)
{
	if(m_oUdpSocket.isOk())
	{
		m_oUdpSocket.close();
	}

	m_oSignalDecoder.uninitialize();
	l_ui32InputCount = 0;
	m_pInputNames.clear();
	m_pInputValues.clear();

	return true;
}

OpenViBE::boolean CBoxAlgorithmOSCController::processInput(uint32 ui32InputIndex)
{
	// FIXME: Here we may check whether the inputs have changed compared
	// to their previous values.
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

OpenViBE::boolean CBoxAlgorithmOSCController::process(void)
{
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext = this->getDynamicBoxContext();

	oscpkt::PacketWriter pw;
	oscpkt::Message msg;
	bool have_data = false;

	//iterate over all chunk on input 0
	for (uint32 i = 0; i < l_ui32InputCount; ++i) {
		uint32 chunkCount = l_rDynamicBoxContext.getInputChunkCount(i);
		if (chunkCount > 0) {
			if (!have_data) {
				have_data = true;
				pw.startBundle();
			}
			m_oSignalDecoder.decode(i, 0);
			if (m_oSignalDecoder.isBufferReceived())
			{
				IMatrix* l_pMatrix = m_oSignalDecoder.getOutputMatrix();

				uint32 l_pInputVal = static_cast<uint32>(l_pMatrix->getBuffer()[0]);
				// FIXME: We need different types as well */
				pw.addMessage(msg.init(m_pInputNames[i]).pushInt32(l_pInputVal));
			}
		}
	}

	if (have_data) {
		pw.endBundle();
		m_oUdpSocket.sendPacket(pw.packetData(), pw.packetSize());
	}
	return true;
}
