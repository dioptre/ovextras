#include "ovpCBoxLSLExport.h"

#include <system/Memory.h>

#include <itpp/base/algebra/eigen.h>
#include <itpp/base/algebra/inv.h>
#include <itpp/base/converters.h>
#include <itpp/stat/misc_stat.h>
#include <itpp/itsignal.h>
#include <itpp/signal/resampling.h>
#include <itpp/base/factory.h>
using namespace itpp;

#include <openvibe/ovITimeArithmetics.h>
#include <array>


using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Streaming;


CBoxAlgorithmLSLExport::CBoxAlgorithmLSLExport()
: m_oCInputChannel1(0)
{
}

boolean CBoxAlgorithmLSLExport::initialize(void)
{
	const IBox* l_pStaticBoxContext=getBoxAlgorithmContext()->getStaticBoxContext();

	m_oCInputChannel1.initialize(this);

	l_pStaticBoxContext->getSettingValue(0, m_sStreamName);
	l_pStaticBoxContext->getSettingValue(1, m_sStreamType);

	m_outlet = NULL;
	m_stims.clear();

	return true;
}

boolean CBoxAlgorithmLSLExport::uninitialize(void)
{
	m_oCInputChannel1.uninitialize();

	m_stims.clear();

	if (m_outlet!=NULL)
		delete m_outlet;

	return true;
}

boolean CBoxAlgorithmLSLExport::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CBoxAlgorithmLSLExport::process(void)
{
	if(!m_oCInputChannel1.isWorking()) 
	{
		m_oCInputChannel1.waitForSignalHeader();

		if (m_oCInputChannel1.isWorking())
		{
			 try 
			 {
				//if it fails here then most likely you are using the wrong dll - e.x debug instead of release or vice-versa
				lsl::stream_info info(m_sStreamName.toASCIIString(),m_sStreamType.toASCIIString(),m_oCInputChannel1.getNbOfChannels() + 1 ,m_oCInputChannel1.getSamplingRate(),lsl::cf_float32);

				lsl::xml_element channels = info.desc().append_child("channels");

				for(uint32 i=0; i<m_oCInputChannel1.getNbOfChannels(); i++)
					channels.append_child("channel")
						.append_child_value("label",m_oCInputChannel1.getChannelName(i))
						.append_child_value("type","EEG")
						.append_child_value("unit","microvolts");

				channels.append_child("channel")
						.append_child_value("label","Stimulations")
						.append_child_value("type","marker");

				if (m_outlet!=NULL)
					this->getLogManager() << LogLevel_Error << "Possible double initializatioin!\n";

				m_outlet =  new lsl::stream_outlet(info); //here the length of the buffered signal can be specified	
			 }
			 catch(std::exception &e) 
			 {
				 this->getLogManager() << LogLevel_Error << "Could not initialize LSL library: " << e.what() << "\n";
				 return false;
             }
		}
	}
	else
	{
		//stimulations
		for (uint32 i=0;i < m_oCInputChannel1.getNbOfStimulationBuffers(); i++)
		{
			OpenViBE::uint64 l_u64ChunkStartTimestamp, l_u64ChunkEndTimestamp;
			IStimulationSet* set = m_oCInputChannel1.getStimulation(l_u64ChunkStartTimestamp,l_u64ChunkEndTimestamp,i);
			
			for (uint32 j=0;j < set->getStimulationCount(); j++)
			{
				uint64 time = m_oCInputChannel1.getStartTimestamp() + set->getStimulationDate(j);
				uint64 identifier = set->getStimulationIdentifier(j);

				
				if(m_stims.size() == 0)
				{
					m_stims.push_back(std::pair<float32,uint64>((float32)identifier,time));
				    //std::cout<< "added: " << m_stims[m_stims.size()-1].first << " " << m_stims[m_stims.size()-1].second<< "\n";
				}
				else
				{
					std::pair<float32,uint64> last = m_stims[m_stims.size()-1];
					if (last.first!=identifier && last.second!=time)
					{
						m_stims.push_back(std::pair<float32,uint64>((float32)identifier,time));
						//std::cout<< "added: " << m_stims[m_stims.size()-1].first << " " << m_stims[m_stims.size()-1].second<< "\n";
					}
					else
					{
						//std::cout<< "duplicate: " << m_stims[m_stims.size()-1].first << " " << m_stims[m_stims.size()-1].second<< "\n";
					}
				}
			}
		}

		//signal
		for (uint32 i=0;i < m_oCInputChannel1.getNbOfSignalBuffers(); i++)
		{
		    OpenViBE::uint64 l_u64StartTimestamp, l_u64EndTimestamp; 
			OpenViBE::float64* l_pInputBuffer = m_oCInputChannel1.getSignal(l_u64StartTimestamp, l_u64EndTimestamp, i);

			if(l_pInputBuffer)
			{

		        uint32 l_ui32SamplesPerChannelInput = m_oCInputChannel1.getNbOfSamples();
				std::vector<std::vector<float32>> mychunk(l_ui32SamplesPerChannelInput);

				for(uint32 k=0;k<l_ui32SamplesPerChannelInput;k++)
				{
					mychunk[k] = std::vector<float32>(m_oCInputChannel1.getNbOfChannels()+1);
				}

				//Fill a matrix - OpenVibe provides the data ch1 (all values from all samples), ch2(all values from all samples) ... chN, 
				//In the generated chunk every row is a single sample (containing the data from all channels) and every column number is the number of the channel
				for (uint32 k=0;k < m_oCInputChannel1.getNbOfChannels(); k++) 
				{
					for (int j=0;j <l_ui32SamplesPerChannelInput; j++)
					{
						int index = (k * l_ui32SamplesPerChannelInput) + j;
						mychunk[j][k] = l_pInputBuffer[index];
					}
				}

				//Process stimulations and add them to the output in a dedicated channel
				std::vector<float32> stim_chan = std::vector<float32>(l_ui32SamplesPerChannelInput);

				std::vector<std::pair<OpenViBE::float32,OpenViBE::uint64>>::iterator it; 

				std::vector<std::pair<OpenViBE::float32,OpenViBE::uint64>>::iterator selected = std::remove_if
					        (m_stims.begin(),  
							 m_stims.end(),
							[l_u64StartTimestamp,l_u64EndTimestamp](std::pair<OpenViBE::float32,OpenViBE::uint64> v) { return (v.second >= l_u64StartTimestamp && v.second <= l_u64EndTimestamp); }
				            );

				for (it = selected ; it < m_stims.end(); it++)
				{
					std::pair<float32,uint64> current = *it;
					
					uint64 posCurrent = ITimeArithmetics::timeToSampleCount(m_oCInputChannel1.getSamplingRate(), current.second);
					//uint64 posEnd = ITimeArithmetics::timeToSampleCount(m_oCInputChannel1.getSamplingRate(), l_u64EndTimestamp);
					uint64 posStart = ITimeArithmetics::timeToSampleCount(m_oCInputChannel1.getSamplingRate(), l_u64StartTimestamp);

					uint32 pos = (uint32)posCurrent - (uint32)posStart;
					if (pos<0) pos = 0; //fix position
					if (pos == stim_chan.size()) pos = stim_chan.size() - 1; //fix position

					if (pos>=0 && pos<stim_chan.size())
					{
					  stim_chan[pos] = (float32)current.first;
					  //std::cout<< "pos relative: " << pos << " value: " << stim_chan[pos] << " time:" << ITimeArithmetics::timeToSeconds(current.second)<< "\n";
					}
					else this->getLogManager() << LogLevel_Warning << "Bad stimulation position: " << pos << "stim code: " << current.first << "\n";
				}

				//remove the processed ones
				m_stims.erase(selected,m_stims.end());

				//add the stim channel at the end of the matrix
				uint32 k = m_oCInputChannel1.getNbOfChannels();
                for (uint32 j=0;j <l_ui32SamplesPerChannelInput; j++)
				{
					mychunk[j][k] = stim_chan[j];
				}

				//send all channels
				m_outlet->push_chunk(mychunk);			
	        }
		}
	}
	
	return true;
}
