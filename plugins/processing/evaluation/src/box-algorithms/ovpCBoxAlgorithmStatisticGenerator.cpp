#include "ovpCBoxAlgorithmStatisticGenerator.h"

#include <sstream>

#include <xml/IXMLHandler.h>
#include <xml/IXMLNode.h>
#include <limits>

namespace{
	const char* const c_sStatisticRootNodeName = "Statistic";
	const char* const c_sStimulationListNodeName = "Stimulations-list";
	const char* const c_sStimulationNodeName = "Stimulation";
	const char* const c_sIdentifierCodeNodeName = "Identifier";
	const char* const c_sIdentifierLabelNodeName = "Label";
	const char* const c_sAmountNodeName = "Count";

	const char* const c_sChannelListNodeName = "Channel-list";
	const char* const c_sChannelNodeName = "Channel";
	const char* const c_sChannelLabelNodeName = "Name";
	const char* const c_sChannelMinNodeName = "Minimum";
	const char* const c_sChannelMaxNodeName = "Maximum";
	const char* const c_sChannelMeanNodeName = "Mean";
}


using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Measurement;

boolean CBoxAlgorithmStatisticGenerator::initialize(void)
{
	m_oSignalDecoder.initialize(*this, 0);
	m_oStimulationDecoder.initialize(*this, 1);

	m_oStimulationMap.clear();

	m_bHasBeenStreamed = false;

	m_oFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

	return true;
}

boolean CBoxAlgorithmStatisticGenerator::uninitialize(void)
{
	m_oSignalDecoder.uninitialize();
	m_oStimulationDecoder.uninitialize();

	if(m_bHasBeenStreamed)
	{

		XML::IXMLNode* l_pRootNode = XML::createNode(c_sStatisticRootNodeName);


		XML::IXMLNode* l_pStimulationsNode = XML::createNode(c_sStimulationListNodeName);
		for(std::map< CIdentifier,uint32 >::iterator iter = m_oStimulationMap.begin(); iter != m_oStimulationMap.end(); ++iter)
		{
			XML::IXMLNode* l_pTempNode = XML::createNode(c_sStimulationNodeName);

			XML::IXMLNode* l_pIdNode = XML::createNode(c_sIdentifierCodeNodeName);
			CIdentifier l_oIdentifier =  iter->first;
			l_pIdNode->setPCData(l_oIdentifier.toString());

			XML::IXMLNode* l_pLabelNode = XML::createNode(c_sIdentifierLabelNodeName);
			l_pLabelNode->setPCData(this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, l_oIdentifier.toUInteger()).toASCIIString());

			XML::IXMLNode* l_pAmountNode = XML::createNode(c_sAmountNodeName);
			std::stringstream l_sAmount;
			l_sAmount << m_oStimulationMap[l_oIdentifier];

			l_pAmountNode->setPCData(l_sAmount.str().c_str());

			l_pTempNode->addChild(l_pIdNode);
			l_pTempNode->addChild(l_pLabelNode);
			l_pTempNode->addChild(l_pAmountNode);

			l_pStimulationsNode->addChild(l_pTempNode);

		}
		l_pRootNode->addChild(l_pStimulationsNode);


		XML::IXMLNode* l_pChannelsNode = XML::createNode(c_sChannelListNodeName);
		for(uint32 i = 0; i < m_oSignalInfoList.size(); ++i)
		{
			SSignalInfo& l_oSignalInfo = m_oSignalInfoList[i];
			XML::IXMLNode* l_pChannelNode = XML::createNode(c_sChannelNodeName);

			XML::IXMLNode* l_pNodeName = XML::createNode(c_sChannelLabelNodeName);
			l_pNodeName->setPCData(l_oSignalInfo.m_sChannelName.toASCIIString());
			l_pChannelNode->addChild(l_pNodeName);


			l_pChannelNode->addChild(getFloat64Node(c_sChannelMaxNodeName, l_oSignalInfo.m_f64Max));
			l_pChannelNode->addChild(getFloat64Node(c_sChannelMinNodeName, l_oSignalInfo.m_f64Min));
			l_pChannelNode->addChild(getFloat64Node(c_sChannelMeanNodeName, l_oSignalInfo.m_f64SampleSum/l_oSignalInfo.m_ui32SampleCount));

			l_pChannelsNode->addChild(l_pChannelNode);
		}
		l_pRootNode->addChild(l_pChannelsNode);

		XML::IXMLHandler *l_pHandler = XML::createXMLHandler();
		l_pHandler->writeXMLInFile(*l_pRootNode, m_oFilename.toASCIIString());

		l_pHandler->release();
		l_pRootNode->release();
	}
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
			uint32 l_ui32AmountChannel = m_oSignalDecoder.getOutputMatrix()->getDimensionSize(0);
			m_bHasBeenStreamed = true;
			for(size_t j = 0; j < l_ui32AmountChannel; ++j)
			{
				SSignalInfo l_oInfo = {m_oSignalDecoder.getOutputMatrix()->getDimensionLabel(0, j),
									   std::numeric_limits<float64>::max(), - std::numeric_limits<float64>::max(), 0, 0};
				m_oSignalInfoList.push_back(l_oInfo);
			}
		}
		if(m_oSignalDecoder.isBufferReceived())
		{
			uint32 l_ui32SampleCount = m_oSignalDecoder.getOutputMatrix()->getDimensionSize(1);
			float64* l_pBuffer = m_oSignalDecoder.getOutputMatrix()->getBuffer();
			for(size_t j = 0; j < m_oSignalInfoList.size(); ++j)
			{
				SSignalInfo& l_oInfo = m_oSignalInfoList[j];
				for(size_t k = 0; k < l_ui32SampleCount; ++k)
				{
					float64 l_f64Sample = l_pBuffer[j*l_ui32SampleCount + k];
					l_oInfo.m_f64SampleSum += l_f64Sample;

					if(l_f64Sample < l_oInfo.m_f64Min)
					{
						l_oInfo.m_f64Min = l_f64Sample;
					}
					if(l_f64Sample > l_oInfo.m_f64Max)
					{
						l_oInfo.m_f64Max = l_f64Sample;
					}
				}
				l_oInfo.m_ui32SampleCount += l_ui32SampleCount;
			}
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
			m_bHasBeenStreamed=true;
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

XML::IXMLNode *CBoxAlgorithmStatisticGenerator::getFloat64Node(const char *const sNodeName, float64 f64Value)
{
	std::stringstream l_sString;
	XML::IXMLNode* l_pTempNode = XML::createNode(sNodeName);
	l_sString << f64Value;
	l_pTempNode->setPCData(l_sString.str().c_str());
	return l_pTempNode;
}
