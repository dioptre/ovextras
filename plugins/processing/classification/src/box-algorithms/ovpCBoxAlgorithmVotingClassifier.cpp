#include "ovpCBoxAlgorithmVotingClassifier.h"

#include <system/ovCMemory.h>

#include <list>
#include <vector>
#include <string>
#include <algorithm>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

boolean CBoxAlgorithmVotingClassifier::initialize(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	m_oClassificationChoiceEncoder.initialize(*this, 0);

	CIdentifier l_oTypeIdentifier;
	l_rStaticBoxContext.getInputType(0, l_oTypeIdentifier);
	m_bMatrixBased=(l_oTypeIdentifier==OV_TypeId_StreamedMatrix);

	for(uint32 i=0; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		SInput& l_rInput=m_vClassificationResults[i];
		if(m_bMatrixBased)
		{
			OpenViBEToolkit::TStreamedMatrixDecoder<CBoxAlgorithmVotingClassifier> *l_pDecoder = new OpenViBEToolkit::TStreamedMatrixDecoder<CBoxAlgorithmVotingClassifier>();
			l_pDecoder->initialize(*this, i);
			l_rInput.m_pDecoder= l_pDecoder;

			l_rInput.op_pMatrix = l_pDecoder->getOutputMatrix();

			l_rInput.m_bTwoValueInput = false;
		}
		else
		{
			OpenViBEToolkit::TStimulationDecoder<CBoxAlgorithmVotingClassifier> *l_pDecoder = new OpenViBEToolkit::TStimulationDecoder<CBoxAlgorithmVotingClassifier>();
			l_pDecoder->initialize(*this, i);
			l_rInput.m_pDecoder= l_pDecoder;

			l_rInput.op_pStimulationSet = l_pDecoder->getOutputStimulationSet();

			l_rInput.m_bTwoValueInput = false;
		}
	}

	m_ui64NumberOfRepetitions  =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_ui64TargetClassLabel     =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_ui64NonTargetClassLabel  =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	m_ui64RejectClassLabel     =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
	m_ui64ResultClassLabelBase =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
	m_bChooseOneIfExAequo      =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

	m_ui64LastTime=0;

	m_oClassificationChoiceEncoder.encodeHeader();
	this->getDynamicBoxContext().markOutputAsReadyToSend(0, m_ui64LastTime, this->getPlayerContext().getCurrentTime());

	return true;
}

boolean CBoxAlgorithmVotingClassifier::uninitialize(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
	uint32 i;

	for(i=0; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		SInput& l_rInput=m_vClassificationResults[i];
		l_rInput.m_pDecoder->uninitialize();
		delete l_rInput.m_pDecoder;
	}

	m_oClassificationChoiceEncoder.uninitialize();

	return true;
}

boolean CBoxAlgorithmVotingClassifier::processInput(uint32 ui32Index)
{
	this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmVotingClassifier::process(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
	uint32 i, j, k;

	boolean l_bCanChoose=true;

	for(i=0; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		SInput& l_rInput=m_vClassificationResults[i];
		for(j=0; j<l_rDynamicBoxContext.getInputChunkCount(i); j++)
		{
			l_rInput.m_pDecoder->decode(j);

			if(l_rInput.m_pDecoder->isHeaderReceived())
			{
				if(m_bMatrixBased)
				{
					if(l_rInput.op_pMatrix->getBufferElementCount() != 1)
					{
						if(l_rInput.op_pMatrix->getBufferElementCount() == 2)
						{
							this->getLogManager() << LogLevel_Trace << "Input got two dimensions, the value use for the vote will be the difference between the two values\n";
							l_rInput.m_bTwoValueInput = true;
						}
						else
						{
							this->getLogManager() << LogLevel_ImportantWarning << "Input matrix should have one or two values\n";
							return false;
						}
					}
				}
			}
			if(l_rInput.m_pDecoder->isBufferReceived())
			{
				if(m_bMatrixBased)
				{
					float64 l_f64Value;
					if(l_rInput.m_bTwoValueInput)
					{
						l_f64Value = l_rInput.op_pMatrix->getBuffer()[1] - l_rInput.op_pMatrix->getBuffer()[0];
					}
					else
					{
						l_f64Value = l_rInput.op_pMatrix->getBuffer()[0];
					}
					l_rInput.m_vScore.push_back(std::pair<float64, uint64>(-l_f64Value, l_rDynamicBoxContext.getInputChunkEndTime(i, j)));
				}
				else
				{
					for(k=0; k<l_rInput.op_pStimulationSet->getStimulationCount(); k++)
					{
						uint64 l_ui64StimulationIdentifier=l_rInput.op_pStimulationSet->getStimulationIdentifier(k);
						if(l_ui64StimulationIdentifier == m_ui64TargetClassLabel || l_ui64StimulationIdentifier == m_ui64NonTargetClassLabel || l_ui64StimulationIdentifier == m_ui64RejectClassLabel)
						{
							l_rInput.m_vScore.push_back(std::pair<float64, uint64>(l_ui64StimulationIdentifier == m_ui64TargetClassLabel ? 1 : 0, l_rInput.op_pStimulationSet->getStimulationDate(k)));
						}
					}
				}
			}
			if(l_rInput.m_pDecoder->isEndReceived())
			{
				m_oClassificationChoiceEncoder.encodeEnd();
				l_rDynamicBoxContext.markOutputAsReadyToSend(0, m_ui64LastTime, this->getPlayerContext().getCurrentTime());
			}
		}

		if(l_rInput.m_vScore.size() < m_ui64NumberOfRepetitions)
		{
			l_bCanChoose=false;
		}
	}

	if(l_bCanChoose)
	{
		float64 l_f64ResultScore=-1E100;
		uint64 l_ui64ResultClassLabel = m_ui64RejectClassLabel;
		uint64 l_ui64Time = 0;

		std::map < uint32, float64 > l_vScore;
		for(i=0; i<l_rStaticBoxContext.getInputCount(); i++)
		{
			SInput& l_rInput=m_vClassificationResults[i];
			l_vScore[i]=0;
			for(j=0; j<m_ui64NumberOfRepetitions; j++)
			{
				l_vScore[i]+=l_rInput.m_vScore[j].first;
			}

			if(l_vScore[i] > l_f64ResultScore)
			{
				l_f64ResultScore = l_vScore[i];
				l_ui64ResultClassLabel = m_ui64ResultClassLabelBase + i;
				l_ui64Time = l_rInput.m_vScore[(unsigned int)(m_ui64NumberOfRepetitions-1)].second;
			}
			else if(l_vScore[i] == l_f64ResultScore)
			{
				if(!m_bChooseOneIfExAequo)
				{
					l_f64ResultScore = l_vScore[i];
					l_ui64ResultClassLabel = m_ui64RejectClassLabel;
					l_ui64Time = l_rInput.m_vScore[(unsigned int)(m_ui64NumberOfRepetitions-1)].second;
				}
			}

			l_rInput.m_vScore.erase(l_rInput.m_vScore.begin(), l_rInput.m_vScore.begin()+(int)m_ui64NumberOfRepetitions);

			this->getLogManager() << LogLevel_Trace << "Input " << i << " got score " << l_vScore[i] << "\n";
		}

		if(l_ui64ResultClassLabel != m_ui64RejectClassLabel)
		{
			this->getLogManager() << LogLevel_Trace << "Chosed " << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, l_ui64ResultClassLabel) << " with score " << l_f64ResultScore << "\n";
		}
		else
		{
			this->getLogManager() << LogLevel_Trace << "Chosed rejection " << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, l_ui64ResultClassLabel) << "\n";
		}
		m_oClassificationChoiceEncoder.getInputStimulationSet()->clear();
		m_oClassificationChoiceEncoder.getInputStimulationSet()->appendStimulation(l_ui64ResultClassLabel, l_ui64Time, 0);

		m_oClassificationChoiceEncoder.encodeBuffer();
		l_rDynamicBoxContext.markOutputAsReadyToSend(0, m_ui64LastTime, l_ui64Time);
		m_ui64LastTime=l_ui64Time;
	}

	return true;
}
