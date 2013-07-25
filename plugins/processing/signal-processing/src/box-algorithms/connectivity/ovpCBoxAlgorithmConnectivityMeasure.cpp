//#if defined(TARGET_HAS_ThirdPartyEIGEN)

#include "ovpCBoxAlgorithmConnectivityMeasure.h"
#include <system/Memory.h>
#include <cstdio>


using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

using namespace OpenViBEToolkit;

using namespace std;

/**********************************************************************************************************************
 * _find_channel_ and the method to split the channel list was taken from the channel selector created by Yann Renard *
 **********************************************************************************************************************/

namespace
{
	inline uint32 _find_channel_(const IMatrix& rMatrix, const CString& rChannel, const CIdentifier& rMatchMethodIdentifier, uint32 uiStart=0)
	{
		uint32 i, l_ui32Result=uint32(-1);

		if(rMatchMethodIdentifier==OVP_TypeId_MatchMethod_Name)
		{
			for(i=uiStart; i<rMatrix.getDimensionSize(0); i++)
			{
				if(Tools::String::isAlmostEqual(rMatrix.getDimensionLabel(0, i), rChannel, false))
				{
					l_ui32Result=i;
				}
			}
		}
		else if(rMatchMethodIdentifier==OVP_TypeId_MatchMethod_Index)
		{
			unsigned int value;
			if(::sscanf(rChannel.toASCIIString(), "%u", &value)==1)
			{
				value--; // => makes it 0-indexed !
				if(uiStart <= uint32(value) && uint32(value) < rMatrix.getDimensionSize(0))
				{
					l_ui32Result=uint32(value);
				}
			}
		}
		else if(rMatchMethodIdentifier==OVP_TypeId_MatchMethod_Smart)
		{
			if(l_ui32Result==uint32(-1)) l_ui32Result=_find_channel_(rMatrix, rChannel, OVP_TypeId_MatchMethod_Name, uiStart);
			if(l_ui32Result==uint32(-1)) l_ui32Result=_find_channel_(rMatrix, rChannel, OVP_TypeId_MatchMethod_Index, uiStart);
		}

		return l_ui32Result;
	}
};



boolean CBoxAlgorithmConnectivityMeasure::initialize(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	m_ui32InputCount = l_rStaticBoxContext.getInputCount();

	// Retrieve algorithm chosen by the user
	CIdentifier l_oConnectivityAlgorithmClassIdentifier;
	CString l_sConnectivityAlgorithmClassIdentifier = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	l_oConnectivityAlgorithmClassIdentifier=this->getTypeManager().getEnumerationEntryValueFromName(OVTK_ClassId_ConnectivityAlgorithm, l_sConnectivityAlgorithmClassIdentifier);

	// Display an error message if the algorithm is not recognized
	if(l_oConnectivityAlgorithmClassIdentifier==OV_UndefinedIdentifier)
	{
		this->getLogManager() << LogLevel_ImportantWarning << "Unknown connectivity algorithm [" << l_sConnectivityAlgorithmClassIdentifier << "]\n";
		return false;
	}

	// Create algorithm instance of type ConnectivityAlgorithm
	m_pConnectivityMethod = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(l_oConnectivityAlgorithmClassIdentifier));
	m_pConnectivityMethod->initialize();

	// Initialize
	ip_pMatrix1.initialize(m_pConnectivityMethod->getInputParameter(OVTK_Algorithm_Connectivity_InputParameterId_InputMatrix1));
	ip_pMatrix2.initialize(m_pConnectivityMethod->getInputParameter(OVTK_Algorithm_Connectivity_InputParameterId_InputMatrix2));
	ip_ui64SamplingRate1.initialize(m_pConnectivityMethod->getInputParameter(OVTK_Algorithm_Connectivity_InputParameterId_ui64SamplingRate1));
	ip_ui64SamplingRate2.initialize(m_pConnectivityMethod->getInputParameter(OVTK_Algorithm_Connectivity_InputParameterId_ui64SamplingRate2));
	ip_pChannelTable.initialize(m_pConnectivityMethod->getInputParameter(OVTK_Algorithm_Connectivity_InputParameterId_LookupMatrix));
	op_pMatrix.initialize(m_pConnectivityMethod->getInputParameter(OVTK_Algorithm_Connectivity_OutputParameterId_OutputMatrix));

	// Signal stream decoder and encoder initialization
	m_oAlgo0_SignalDecoder.initialize(*this);
	m_oAlgo1_SignalEncoder.initialize(*this);
	
	// if an input was added, creation of the corresponding decoder
	if(m_ui32InputCount==2)
	{
		m_oAlgo2_SignalDecoder.initialize(*this);
	}
	// The box can't process more than two input
	else if(m_ui32InputCount !=1 && m_ui32InputCount !=2)
	{
		this->getLogManager() << LogLevel_ImportantWarning << "Incorrect number of input";
				return false;
	}


	//______________________________________________________________________________________________________________________________________
	//
	// Splits the channel list in order to identify channel pairs to process
	//_______________________________________________________________________________________________________________________________________
	//

	// Retrieve string setting giving channel pairs
	CString l_sChannelList=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	uint64 l_ui64MatchMethodIdentifier=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

	m_vChannelTable.clear();

	// Parsing chain to identify channel name or index
	std::vector < CString > l_sPairs;
	uint32 l_ui32PairsCount = Tools::String::split(l_sChannelList, Tools::String::TSplitCallback < std::vector < CString > >(l_sPairs), OV_Value_EnumeratedStringSeparator);
	for(uint32 pair=0; pair<l_ui32PairsCount; pair++)
	{
		std::vector < CString > l_sChannel;
		uint32 l_ui32ChannelCount = Tools::String::split(l_sPairs[pair], Tools::String::TSplitCallback < std::vector < CString > >(l_sChannel), OVP_Value_CoupledStringSeparator);

		for(uint32 chan=0; chan<l_ui32ChannelCount; chan=chan+2)
		{
			std::vector < CString > l_sSubChannel;
			std::vector < CString > l_sSubChannel2;
			// Checks if the channel designation is a range
			if(OpenViBEToolkit::Tools::String::split(l_sChannel[chan], OpenViBEToolkit::Tools::String::TSplitCallback < std::vector < CString > >(l_sSubChannel), OV_Value_RangeStringSeparator)==2)
			{
				//if yes, check if the next designation is also a range
				if(OpenViBEToolkit::Tools::String::split(l_sChannel[chan+1], OpenViBEToolkit::Tools::String::TSplitCallback < std::vector < CString > >(l_sSubChannel2), OV_Value_RangeStringSeparator)==2)
				{
					// Finds the first & second part of both range (only index based)
					uint32 l_ui32RangeStartIndex=::_find_channel_(*ip_pMatrix1, l_sSubChannel[0], OVP_TypeId_MatchMethod_Index);
					uint32 l_ui32RangeEndIndex=::_find_channel_(*ip_pMatrix1, l_sSubChannel[1], OVP_TypeId_MatchMethod_Index);

					uint32 l_ui32RangeStartIndex2 =::_find_channel_(*ip_pMatrix1, l_sSubChannel2[0], OVP_TypeId_MatchMethod_Index);
					uint32 l_ui32RangeEndIndex2=::_find_channel_(*ip_pMatrix1, l_sSubChannel2[1], OVP_TypeId_MatchMethod_Index);

					// When first or second part is not found but associated token is empty, don't consider this as an error
					if(l_ui32RangeStartIndex==uint32(-1) && l_sSubChannel[0]==CString("")) l_ui32RangeStartIndex=0;
					if(l_ui32RangeEndIndex  ==uint32(-1) && l_sSubChannel[1]==CString("")) l_ui32RangeEndIndex=ip_pMatrix1->getDimensionSize(0)-1;

					if(l_ui32RangeStartIndex2==uint32(-1) && l_sSubChannel2[0]==CString("")) l_ui32RangeStartIndex2=0;
					if(l_ui32RangeEndIndex2  ==uint32(-1) && l_sSubChannel2[1]==CString("")) l_ui32RangeEndIndex2=ip_pMatrix1->getDimensionSize(0)-1;

					// After these corrections, if either first or second token were not found, or if start index is greater than start index, consider this an error and invalid range
					if(l_ui32RangeStartIndex==uint32(-1) || l_ui32RangeEndIndex  ==uint32(-1) || l_ui32RangeStartIndex>l_ui32RangeEndIndex)
					{
						this->getLogManager() << LogLevel_Warning << "Invalid channel range [" << l_sChannel[chan] << "] - splitted as [" << l_sSubChannel[0] << "][" << l_sSubChannel[1] << "]\n";
					}
					else if(l_ui32RangeStartIndex2==uint32(-1) || l_ui32RangeEndIndex2  ==uint32(-1) || l_ui32RangeStartIndex2>l_ui32RangeEndIndex2)
					{
						this->getLogManager() << LogLevel_Warning << "Invalid channel range [" << l_sChannel[chan+1] << "] - splitted as [" << l_sSubChannel2[0] << "][" << l_sSubChannel2[1] << "]\n";
					}
					else
					{
						// The ranges are valid so selects all the channels in those range
						this->getLogManager() << LogLevel_Trace << "For range [" << l_sChannel[chan] << "] :\n";
						for(uint32 k=l_ui32RangeStartIndex; k<=l_ui32RangeEndIndex; k++)
						{
							for(uint32 j=l_ui32RangeStartIndex2; j<=l_ui32RangeEndIndex2; j++)
							{
								m_vChannelTable.push_back(k);
								m_vChannelTable.push_back(j);

								this->getLogManager() << LogLevel_Trace << "  Selected channels [" << k+1 << ","<< j+1 <<"]\n";
							}

						}
					}

				}

				else
				{
					//First, identify the channel to compare to the range
					uint32 l_ui32Index=uint32(-1);
					// Looks for the channel with this name
					l_ui32Index=::_find_channel_(*ip_pMatrix1, l_sChannel[chan+1], l_ui64MatchMethodIdentifier, l_ui32Index+1);
					// Finds the first & second part of the range (only index based)
					uint32 l_ui32RangeStartIndex=::_find_channel_(*ip_pMatrix1, l_sSubChannel[0], OVP_TypeId_MatchMethod_Index);
					uint32 l_ui32RangeEndIndex=::_find_channel_(*ip_pMatrix1, l_sSubChannel[1], OVP_TypeId_MatchMethod_Index);

					// When first or second part is not found but associated token is empty, don't consider this as an error
					if(l_ui32RangeStartIndex==uint32(-1) && l_sSubChannel[0]==CString("")) l_ui32RangeStartIndex=0;
					if(l_ui32RangeEndIndex  ==uint32(-1) && l_sSubChannel[1]==CString("")) l_ui32RangeEndIndex=ip_pMatrix1->getDimensionSize(0)-1;

					// After these corrections, if either first or second token were not found, or if start index is greater than start index, consider this an error and invalid range
					if(l_ui32RangeStartIndex==uint32(-1) || l_ui32RangeEndIndex  ==uint32(-1) || l_ui32RangeStartIndex>l_ui32RangeEndIndex)
					{
						this->getLogManager() << LogLevel_Warning << "Invalid channel range [" << l_sChannel[chan] << "] - splitted as [" << l_sSubChannel[0] << "][" << l_sSubChannel[1] << "]\n";
					}
					else
					{
						// The range is valid so selects all the channels in this range
						this->getLogManager() << LogLevel_Trace << "For range [" << l_sChannel[chan] << "] :\n";
						for(uint32 k=l_ui32RangeStartIndex; k<=l_ui32RangeEndIndex; k++)
						{
							m_vChannelTable.push_back(k);
							m_vChannelTable.push_back(l_ui32Index);
							this->getLogManager() << LogLevel_Trace << "  Selected channel [" << k+1 << ","<<l_ui32Index+1<<"]\n";
						}
					}
				}
			}
			//else, check if the next designation is a range
			else if(OpenViBEToolkit::Tools::String::split(l_sChannel[chan+1], OpenViBEToolkit::Tools::String::TSplitCallback < std::vector < CString > >(l_sSubChannel), OV_Value_RangeStringSeparator)==2)
			{
				//First, identify the channel to compare to the range
				uint32 l_ui32Index=uint32(-1);
				// Looks for the channel with this name
				l_ui32Index=::_find_channel_(*ip_pMatrix1, l_sChannel[chan], l_ui64MatchMethodIdentifier, l_ui32Index+1);
				// Finds the first & second part of the range (only index based)
				uint32 l_ui32RangeStartIndex=::_find_channel_(*ip_pMatrix1, l_sSubChannel[0], OVP_TypeId_MatchMethod_Index);
				uint32 l_ui32RangeEndIndex=::_find_channel_(*ip_pMatrix1, l_sSubChannel[1], OVP_TypeId_MatchMethod_Index);

				// When first or second part is not found but associated token is empty, don't consider this as an error
				if(l_ui32RangeStartIndex==uint32(-1) && l_sSubChannel[0]==CString("")) l_ui32RangeStartIndex=0;
				if(l_ui32RangeEndIndex  ==uint32(-1) && l_sSubChannel[1]==CString("")) l_ui32RangeEndIndex=ip_pMatrix1->getDimensionSize(0)-1;

				// After these corrections, if either first or second token were not found, or if start index is greater than start index, consider this an error and invalid range
				if(l_ui32RangeStartIndex==uint32(-1) || l_ui32RangeEndIndex  ==uint32(-1) || l_ui32RangeStartIndex>l_ui32RangeEndIndex)
				{
					this->getLogManager() << LogLevel_Warning << "Invalid channel range [" << l_sChannel[chan+1] << "] - splitted as [" << l_sSubChannel[0] << "][" << l_sSubChannel[1] << "]\n";
				}
				else
				{
					// The range is valid so selects all the channels in this range
					this->getLogManager() << LogLevel_Trace << "For range [" << l_sChannel[chan+1] << "] :\n";
					for(uint32 k=l_ui32RangeStartIndex; k<=l_ui32RangeEndIndex; k++)
					{
						m_vChannelTable.push_back(l_ui32Index);
						m_vChannelTable.push_back(k);
						this->getLogManager() << LogLevel_Trace << "  Selected channel [" << k+1 << "]\n";
					}
				}
			}
			// This is not a range, identify the channel
			else
			{
				uint32 l_bFound=false;
				uint32 l_ui32Index=uint32(-1);

				// Looks for all the channels with this name
				while((l_ui32Index=::_find_channel_(*ip_pMatrix1, l_sChannel[chan], l_ui64MatchMethodIdentifier, l_ui32Index+1))!=uint32(-1))
				{
					l_bFound=true;
					m_vChannelTable.push_back(l_ui32Index);
					this->getLogManager() << LogLevel_Trace << "Selected channel [" << l_ui32Index+1 << "]\n";
				}

				while((l_ui32Index=::_find_channel_(*ip_pMatrix1, l_sChannel[chan+1], l_ui64MatchMethodIdentifier, l_ui32Index+1))!=uint32(-1))
				{
					l_bFound=true;
					m_vChannelTable.push_back(l_ui32Index);
					this->getLogManager() << LogLevel_Trace << "Selected channel [" << l_ui32Index+1 << "]\n";
				}

				// When no channel was found, consider it a missing channel
				if(!l_bFound)
				{
					this->getLogManager() << LogLevel_Warning << "Invalid channel [" << l_sPairs[pair] << "]\n";
					m_vChannelTable.push_back(uint32(-1));
				}
			}


		}
	}




	// Copy the look up vector into the parameterHandler in order to pass it to the algorithm
	for(uint32 i=0;i<m_vChannelTable.size();i++)
	{
		ip_pChannelTable->getBuffer()[i] = m_vChannelTable[i];
	}

	// Set reference target
	ip_pMatrix1.setReferenceTarget(m_oAlgo0_SignalDecoder.getOutputMatrix());
	ip_ui64SamplingRate1.setReferenceTarget(m_oAlgo0_SignalDecoder.getOutputSamplingRate());
	if( m_ui32InputCount == 2)
	{
		ip_pMatrix2.setReferenceTarget(m_oAlgo2_SignalDecoder.getOutputMatrix());
		ip_ui64SamplingRate2.setReferenceTarget(m_oAlgo2_SignalDecoder.getOutputSamplingRate());
	}
	else
	{
		ip_pMatrix2.setReferenceTarget(m_oAlgo0_SignalDecoder.getOutputMatrix());
		ip_ui64SamplingRate2.setReferenceTarget(m_oAlgo0_SignalDecoder.getOutputSamplingRate());
	}

	m_oAlgo1_SignalEncoder.getInputMatrix().setReferenceTarget(op_pMatrix);

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmConnectivityMeasure::uninitialize(void)
{
	m_pConnectivityMethod->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_pConnectivityMethod);
	m_oAlgo0_SignalDecoder.uninitialize();
	m_oAlgo1_SignalEncoder.uninitialize();
	// if a second decoder algorithm was created
	if(m_ui32InputCount==2)
		{
			m_oAlgo2_SignalDecoder.uninitialize();
		}
	return true;
}


boolean CBoxAlgorithmConnectivityMeasure::processInput(uint32 ui32InputIndex)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmConnectivityMeasure::process(void)
{
	
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	// we decode the input signal chunks
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{

		m_oAlgo0_SignalDecoder.decode(0,i);
//		if(m_ui32InputCount==2){m_oAlgo2_SignalDecoder.decode(1,i)};

		// If header is received
		if(m_oAlgo0_SignalDecoder.isHeaderReceived())
		{
			// Start the initialization process
			m_pConnectivityMethod->process(OVTK_Algorithm_Connectivity_InputTriggerId_Initialize);
			// Make sure the algo initialization was successful
			if(!m_pConnectivityMethod->process(OVTK_Algorithm_Connectivity_InputTriggerId_Initialize))
		    {
				this->getLogManager() << LogLevel_Trace << "initialization was unsuccessful";
				return false;
		    }

			// Pass the header to the next boxes, by encoding a header on the output 0:
			m_oAlgo1_SignalEncoder.encodeHeader(0);
//			if(m_ui32InputCount==2){m_oAlgo2_SignalDecoder.encodeHeader(0)};
			// send the output chunk containing the header. The dates are the same as the input chunk:
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
		if(m_oAlgo0_SignalDecoder.isBufferReceived())
		{
			// Buffer received. For example the signal values
			// Access to the buffer can be done thanks to :
//			IMatrix* l_pMatrix = m_oAlgo0_SignalDecoder.getOutputMatrix(); // the StreamedMatrix of samples.
//			uint64 l_uiSamplingFrequency = m_oAlgo0_SignalDecoder.getOutputSamplingRate(); // the sampling rate of the signal
			
			m_pConnectivityMethod->process(OVTK_Algorithm_Connectivity_InputTriggerId_Process);

			if(m_pConnectivityMethod->isOutputTriggerActive(OVTK_Algorithm_Connectivity_OutputTriggerId_ProcessDone))
			{
				// Encode the output buffer :
				m_oAlgo1_SignalEncoder.encodeBuffer(0);
				// and send it to the next boxes :
				l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			}

		}
		if(m_oAlgo0_SignalDecoder.isEndReceived())
		{
			// End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
			m_oAlgo1_SignalEncoder.encodeEnd(0);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

		// The current input chunk has been processed, and automatically discarded
	}

	return true;
}

//#endif //#TARGET_HAS_ThirdPartyEIGEN
