#include "ovpCBoxAlgorithmMessageReceiver.h"
#include <sstream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Samples;

boolean CBoxAlgorithmMessageReceiver::initialize(void)
{
	l_bMessageReceived = false;
	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmMessageReceiver::uninitialize(void)
{

	return true;
}
/*******************************************************************************/


boolean CBoxAlgorithmMessageReceiver::processClock(IMessageClock& rMessageClock)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/


uint64 CBoxAlgorithmMessageReceiver::getClockFrequency(void)
{
	// Note that the time is coded on a 64 bits unsigned integer, fixed decimal point (32:32)
	return 1LL<<32; // the box clock frequency
}
/*******************************************************************************/

/*
boolean CBoxAlgorithmMessageReceiver::processInput(uint32 ui32InputIndex)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

OpenViBE::boolean CBoxAlgorithmMessageReceiver::processMessage(const IMyMessage& msg, uint32 inputIndex)
{
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//create the message
	IMyMessage& MMM = this->getPlayerContext().createMessage();
	//test that the sending is actually impossible
	this->getPlayerContext().sendMessage(MMM, 0);


	getLogManager() << OpenViBE::Kernel::LogLevel_Info << "on message input " << inputIndex << "\n";
	bool success;
	CString uiKey = CString("meaning of life");
	CString floatKey = CString("float");
	CString strKey = CString("string");
	uint64 uinteger = msg.getValueUint64(uiKey, success);
	getLogManager() << OpenViBE::Kernel::LogLevel_Info << uinteger << " " << success << "\n";
	float64 flt = msg.getValueFloat64( floatKey, success);
	getLogManager() << OpenViBE::Kernel::LogLevel_Info << flt << " " << success <<"\n";
	const CString* cstr =  msg.getValueCString(strKey, success);
	getLogManager() << OpenViBE::Kernel::LogLevel_Info << *cstr  << " " << success <<"\n";

	CString matKey = CString("matrix");
	const IMatrix * l_oMatrix = msg.getValueCMatrix( matKey, success);
	getLogManager() << OpenViBE::Kernel::LogLevel_Info << l_oMatrix->getBufferElementCount() << " " << success <<"\n";
	//*
	const float64* l_f64Buffer = l_oMatrix->getBuffer();
	std::stringstream l_sstream;
	for (uint64 i=0; i<l_oMatrix->getBufferElementCount(); i++)
	{
		l_sstream << l_f64Buffer[i] << " ";
	}
	getLogManager() << OpenViBE::Kernel::LogLevel_Info << l_sstream.str().c_str() << "\n";


	getLogManager() << OpenViBE::Kernel::LogLevel_Info << "testing copy matrix\n";
	//m_oMatrix.getFullCopy(msg.getCopyValueCMatrix( "matrix", success));
	OpenViBEToolkit::Tools::Matrix::copy(m_oMatrix, *(msg.getValueCMatrix( "matrix", success)) );

	getLogManager() << OpenViBE::Kernel::LogLevel_Info <<  "matrix " << success <<" (by copy)\n";

	const float64* l_f64CopyBuffer = m_oMatrix.getBuffer();
	std::stringstream l_sCopyStream;
	for (uint64 i=0; i<m_oMatrix.getBufferElementCount(); i++)
	{
		l_sCopyStream << l_f64CopyBuffer[i] << " ";
	}
	getLogManager() << OpenViBE::Kernel::LogLevel_Info << l_sCopyStream.str().c_str() << "\n";

	l_bMessageReceived = true;
	return true;
}




boolean CBoxAlgorithmMessageReceiver::process(void)
{
	
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();


	if (l_bMessageReceived)
	{
		const float64* l_f64Buffer = m_oMatrix.getBuffer();
		std::stringstream l_sstream;
		for (uint64 i=0; i<m_oMatrix.getBufferElementCount(); i++)
		{
			l_sstream << l_f64Buffer[i] << " ";
		}
		getLogManager() << OpenViBE::Kernel::LogLevel_Info << "in process (test copy worked) " << l_sstream.str().c_str() << "\n";

	}
	return true;
}
