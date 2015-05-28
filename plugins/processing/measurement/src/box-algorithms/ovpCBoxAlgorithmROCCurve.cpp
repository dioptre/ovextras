#include "ovpCBoxAlgorithmROCCurve.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Measurement;

boolean CBoxAlgorithmROCCurve::initialize(void)
{
	m_oTriggerDecoder.initialize(*this, 0);
	m_oClassLabelDecoder.initialize(*this, 1);
	m_oClassificationValueDecoder.initialize(*this, 2);
	
	return true;
}

boolean CBoxAlgorithmROCCurve::uninitialize(void)
{
	m_oTriggerDecoder.uninitialize();
	m_oClassLabelDecoder.uninitialize();
	m_oClassificationValueDecoder.uninitialize();

	return true;
}


boolean CBoxAlgorithmROCCurve::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}


boolean CBoxAlgorithmROCCurve::process(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	return true;
}
