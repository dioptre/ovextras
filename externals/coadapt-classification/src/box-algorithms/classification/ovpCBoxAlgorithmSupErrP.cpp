#include "ovpCBoxAlgorithmSupErrP.h"

#include <system/Memory.h>
#include <iostream>


using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

boolean CBoxAlgorithmSupErrP::initialize(void)
{
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

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
	//since I use only one encoder, I will go back to a single encoder instead of a vector
	/*for(uint32 i=0; i<l_rStaticBoxContext.getOutputCount(); i++)
	{
		IAlgorithmProxy* m_pStreamEncoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamEncoder));
		m_pStreamEncoder->initialize();
		m_vStimulationsEncoder.push_back(m_pStreamEncoder);
		//this->getLogManager() << LogLevel_Warning <<i<<"\n";
	}*/
	m_vStimulationsEncoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamEncoder));
	m_vStimulationsEncoder->initialize();
	//parameter handler of the encoder
	l_pStimulationSet.initialize(m_vStimulationsEncoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_InputParameterId_StimulationSet));
	op_pEncodedMemoryBuffer.initialize(m_vStimulationsEncoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_OutputParameterId_EncodedMemoryBuffer));
	//----------------------------------------------------------------------------
	//we can already encode the header (see Voting Classifier)
	m_ui64LastTime = 0;
	//m_vStimulationsEncoder[0]->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeHeader);
	//l_rDynamicBoxContext.markOutputAsReadyToSend(0, m_ui64LastTime, this->getPlayerContext().getCurrentTime());
	//false at the beginning, by default
	m_bIsLineChose=false;
	m_bIsColumnChose=false;
	

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmSupErrP::uninitialize(void)
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
	/*for(uint32 i=0; i<l_rStaticBoxContext.getOutputCount(); i++)
	{
		m_vStimulationsEncoder[i]->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_vStimulationsEncoder[i]);
	}
	m_vStimulationsEncoder.clear();*/
	m_vStimulationsEncoder->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_vStimulationsEncoder);
	return true;
}

boolean CBoxAlgorithmSupErrP::processInput(uint32 ui32InputIndex)
{
	// some pre-processing code if needed...
	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmSupErrP::process(void)
{
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
	
	CStimulationSet l_oStimulationSet;
	// some local variables ------------------------------------------------------------
	uint64 l_ui64StimulationToSend;
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
					//entry 0 is the experimentation parameter
					if (k==0)
					{
						//the two stimulation are on the same channel
						//the first is the line, the second the column
						if (j==0)
						{
							m_ui64TargetLineIdentifier = l_ui64StimulationCode;
							std::cout << "SupErrP: Target Line: " << m_ui64TargetLineIdentifier-33025 << "\n";
						}
						if (j==1)
						{
							m_ui64TargetColumnIdentifier = l_ui64StimulationCode;
							std::cout << "SupErrP: Target Column: " << m_ui64TargetColumnIdentifier-33025 << "\n";
						}
						//when a new letter is targeted, we must wait to compute the reward, so ...
						m_bIsLineChose=false;
						m_bIsColumnChose=false;
					}
					//the answer of the classifier, entry 1 is the line, entry 2 is the column
					if (k==1)
					{
						m_ui64ChosenLineIdentifier = l_ui64StimulationCode;
						m_bIsLineChose=true;
						//l_ui64StartTime = l_rDynamicBoxContext.getInputChunkStartTime(k, i); 
						//l_ui64EndTime = l_rDynamicBoxContext.getInputChunkEndTime(k, i);
						//uint32 l = l_oStimulationSet.appendStimulation(m_ui64ChosenLineIdentifier, this->getPlayerContext().getCurrentTime(), 0);
						
					}
					if (k==2)
					{
						m_ui64ChosenColumnIdentifier = l_ui64StimulationCode;
						m_bIsColumnChose=true;
						//l_ui64StartTime = l_rDynamicBoxContext.getInputChunkStartTime(k, i); 
						//l_ui64EndTime = l_rDynamicBoxContext.getInputChunkEndTime(k, i);
						
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
		this->getLogManager() << LogLevel_Fatal <<"Target line : "<<m_ui64TargetLineIdentifier<<" chosen line "<< m_ui64ChosenLineIdentifier <<"\n";
		this->getLogManager() << LogLevel_Fatal <<"Target column : "<<m_ui64TargetColumnIdentifier<<" chosen column "<< m_ui64ChosenColumnIdentifier <<"\n";
		//determination of the reward ------------------------------------------------------------------------------------------
		if ((m_ui64ChosenColumnIdentifier==m_ui64TargetColumnIdentifier)&&(m_ui64ChosenLineIdentifier==m_ui64TargetLineIdentifier))
		{
			//this->getLogManager() << LogLevel_Warning <<"good reward\n ";
			l_ui64StimulationToSend = OVTK_StimulationId_Target;
		}
		else 
		{
			
			l_ui64StimulationToSend = OVTK_StimulationId_NonTarget;
			//this->getLogManager() << LogLevel_Warning <<"bad reward\n ";
		}

		//--------------------------------------------------sending the stimulation ----------------------------------------
		// we get the current time of the player
		uint64 l_ui64StimulationDate = this->getPlayerContext().getCurrentTime();
		l_oStimulationSet.clear();
		l_oStimulationSet.setStimulationCount(3);
		//--------------------------------------------------------------building the stimulation--------------------------------------------------------------
		l_oStimulationSet.setStimulationIdentifier(0,l_ui64StimulationToSend);
		l_oStimulationSet.setStimulationDate(0,l_ui64StimulationDate);
		l_oStimulationSet.setStimulationDuration(0,0);
		
		//send the chosen letter
		l_oStimulationSet.setStimulationIdentifier(1,m_ui64ChosenLineIdentifier);
		l_oStimulationSet.setStimulationDate(1,l_ui64StimulationDate);
		l_oStimulationSet.setStimulationDuration(1,0);
								
		l_oStimulationSet.setStimulationIdentifier(2,m_ui64ChosenColumnIdentifier);
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
