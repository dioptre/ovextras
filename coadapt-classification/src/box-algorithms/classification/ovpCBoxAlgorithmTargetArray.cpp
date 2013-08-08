#include "ovpCBoxAlgorithmTargetArray.h"

#include <system/Memory.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

boolean CBoxAlgorithmTargetArray::initialize(void)
{
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	m_ui64TrainStimulation=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

	//initalizing input (stimulations) decoder ------------------------------------
	for(uint32 i=0; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		IAlgorithmProxy* m_pStreamDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamDecoder));
		m_pStreamDecoder->initialize();
		m_vStimulationsDecoder.push_back(m_pStreamDecoder);

		TParameterHandler < const IMemoryBuffer* > m_pMemoryBuffer(m_vStimulationsDecoder[i]->getInputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_InputParameterId_MemoryBufferToDecode));
		TParameterHandler < IStimulationSet* > m_pStimulationSet(m_vStimulationsDecoder[i]->getOutputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_OutputParameterId_StimulationSet));
		ip_pEncodedMemoryBuffer.push_back(m_pMemoryBuffer);
		ip_pStimulationSet.push_back(m_pStimulationSet);
	}
	//-----------------------------------------------------------------------------
	//initalizing output (stimulations) encoder------------------------------------

	m_vStimulationsEncoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamEncoder));
	m_vStimulationsEncoder->initialize();
	//parameter handler of the encoder
	l_pStimulationSet.initialize(m_vStimulationsEncoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_InputParameterId_StimulationSet));
	op_pEncodedMemoryBuffer.initialize(m_vStimulationsEncoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_OutputParameterId_EncodedMemoryBuffer));
	//----------------------------------------------------------------------------
	m_ui64LastTime = 0;
	m_bIsLineChose=false;
	m_bIsColumnChose=false;
	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmTargetArray::uninitialize(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	//uninitalizing input (stimulations) decoder
	for(uint32 i=0; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		m_vStimulationsDecoder[i]->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_vStimulationsDecoder[i]);
	}
	m_vStimulationsDecoder.clear();

	//uninitalizing output (stimulations) encoder
	m_vStimulationsEncoder->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_vStimulationsEncoder);
	return true;
}

boolean CBoxAlgorithmTargetArray::processInput(uint32 ui32InputIndex)
{
	// some pre-processing code if needed...
	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmTargetArray::process(void)
{
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
	
	CStimulationSet l_oStimulationSet;
	//---------------------------------------------------------------------
	// Parses stimulations
	// for the k-th entry
	for (uint32 k=0; k<=l_rStaticBoxContext.getInputCount(); k++)
	{
		//for the i-th chunk 
		for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(k); i++)
		{
			//get the chunk to decode
			ip_pEncodedMemoryBuffer[k]= l_rDynamicBoxContext.getInputChunk(k, i);
			// decode
			m_vStimulationsDecoder[k]->process();
			if(m_vStimulationsDecoder[k]->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedHeader))
			{
				// Header received. This happens only once when pressing "play".
				// nothing to do...
			}
			if(m_vStimulationsDecoder[k]->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedBuffer))
			{
				// A buffer has been received, lets' check the stimulations inside
				for(uint32 j=0; j<ip_pStimulationSet[k]->getStimulationCount(); j++)
				{
					uint64 l_ui64StimulationCode = ip_pStimulationSet[k]->getStimulationIdentifier(j);
					uint64 l_ui64StimulationDate = ip_pStimulationSet[k]->getStimulationDate(j);
					//entry 0 is the input where the target will be received
					if (k==0)
					{
						if (j==0)
						{
							m_ui64TargetLineIdentifier = l_ui64StimulationCode;
							m_bIsLineChose=true;
						
						}
						if (j==1)
						{
							m_ui64TargetColumnIdentifier = l_ui64StimulationCode;
							m_bIsColumnChose=true;
						}
						
					}

				}
			}		
			if(m_vStimulationsDecoder[i]->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedEnd))
			{
				//this->getLogManager() << LogLevel_Warning <<"		end received\n ";
				//m_vStimulationsEncoder[0]->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeEnd);
			}		
			l_rDynamicBoxContext.markInputAsDeprecated(k, i);
		}
	}

	//determining the reward (good or bad) and sending the stimulation
	if ((m_bIsLineChose)&&(m_bIsColumnChose))
	{				

		//--------------------------------------------------sending the stimulation ----------------------------------------
		// we get the current time of the player
		uint64 l_ui64StimulationDate = this->getPlayerContext().getCurrentTime();
		l_oStimulationSet.clear();
		l_oStimulationSet.setStimulationCount(3);
		//--------------------------------------------------------------building the stimulation--------------------------------------------------------------
		//since we send the target, they are reached
		l_oStimulationSet.setStimulationIdentifier(0,OVTK_StimulationId_Target);
		l_oStimulationSet.setStimulationDate(0,l_ui64StimulationDate);
		l_oStimulationSet.setStimulationDuration(0,0);
		//send the chosen letter
		l_oStimulationSet.setStimulationIdentifier(1,m_ui64TargetLineIdentifier);
		l_oStimulationSet.setStimulationDate(1,l_ui64StimulationDate);
		l_oStimulationSet.setStimulationDuration(1,0);
								
		l_oStimulationSet.setStimulationIdentifier(2,m_ui64TargetColumnIdentifier);
		l_oStimulationSet.setStimulationDate(2,l_ui64StimulationDate);
		l_oStimulationSet.setStimulationDuration(2,0);
		//----------------------------------------------------------------------------------------------------------------------------				


		l_pStimulationSet=&l_oStimulationSet;
		// encode the stimulation
		op_pEncodedMemoryBuffer=l_rDynamicBoxContext.getOutputChunk(0);
		m_vStimulationsEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeBuffer);
		//send the stimulation
		l_rDynamicBoxContext.markOutputAsReadyToSend(0,m_ui64LastTime, l_ui64StimulationDate);
		//update the time
		m_ui64LastTime = l_ui64StimulationDate;
		//reset these boolean to send the stimulation only once
		m_bIsLineChose=false;
		m_bIsColumnChose=false;
	}

	return true;
}
