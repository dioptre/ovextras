#include "ovpCBoxAlgorithmStatisticGenerator.h"

#include <iostream>
#include <sstream>

#include <xml/IXMLHandler.h>
#include <xml/IXMLNode.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Measurement;

boolean CBoxAlgorithmStatisticGenerator::initialize(void)
{
	m_oSignalDecoder.initialize(*this, 0);
	m_oStimulationDecoder.initialize(*this, 1);

	m_ui32AmountChannel = 0;
	m_oStimulationMap.clear();

	m_oFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

	return true;
}

boolean CBoxAlgorithmStatisticGenerator::uninitialize(void)
{
	XML::IXMLNode* l_pRootNode = XML::createNode("Statistic");


	XML::IXMLNode* l_pStimulationsNode = XML::createNode("Stimulations-list");
	for(std::map< CIdentifier,uint32 >::iterator iter = m_oStimulationMap.begin(); iter != m_oStimulationMap.end(); ++iter)
	{
		XML::IXMLNode* l_pTempNode = XML::createNode("Stimulation");

		XML::IXMLNode* l_pIdNode = XML::createNode("Identifier");
		CIdentifier l_oIdentifier =  iter->first;
		l_pIdNode->setPCData(l_oIdentifier.toString());

		XML::IXMLNode* l_pAmountNode = XML::createNode("Amount");
		std::stringstream l_sAmount;
		l_sAmount << m_oStimulationMap[l_oIdentifier];

		l_pAmountNode->setPCData(l_sAmount.str().c_str());

		l_pTempNode->addChild(l_pIdNode);
		l_pTempNode->addChild(l_pAmountNode);

		l_pStimulationsNode->addChild(l_pTempNode);

	}
	l_pRootNode->addChild(l_pStimulationsNode);

	XML::IXMLHandler *l_pHandler = XML::createXMLHandler();
	l_pHandler->writeXMLInFile(*l_pRootNode, m_oFilename.toASCIIString());

	//We need to compute and store data
	m_oSignalDecoder.uninitialize();
	m_oStimulationDecoder.uninitialize();

	l_pHandler->release();
	l_pRootNode->release();

	return true;
}


boolean CBoxAlgorithmStatisticGenerator::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}


boolean CBoxAlgorithmStatisticGenerator::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		m_oSignalDecoder.decode(i);
		if(m_oSignalDecoder.isHeaderReceived())
		{
			m_ui32AmountChannel = m_oSignalDecoder.getOutputMatrix()->getDimensionSize(0);

		}
		if(m_oSignalDecoder.isBufferReceived())
		{

		}
		if(m_oSignalDecoder.isEndReceived())
		{

		}
	}

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(1); i++)
	{
		m_oStimulationDecoder.decode(i);
		if(m_oStimulationDecoder.isHeaderReceived())
		{

		}
		if(m_oStimulationDecoder.isBufferReceived())
		{
			IStimulationSet& l_rStimulationSet = *(m_oStimulationDecoder.getOutputStimulationSet());
			for(uint64 j=0; j<l_rStimulationSet.getStimulationCount(); j++)
			{
				m_oStimulationMap[l_rStimulationSet.getStimulationIdentifier(j)]++;
			}
		}
		if(m_oStimulationDecoder.isEndReceived())
		{

		}
	}

	return true;
}
