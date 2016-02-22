#include "ovpCSignChangeDetector.h"
#include <cstdlib>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Stimulation;

boolean CSignChangeDetector::initialize(void)
{
	CString l_sSettingValue;

	// we read the settings:
	// The stimulations names:
	getStaticBoxContext().getSettingValue(0, l_sSettingValue);
	m_ui64OnStimulationId=getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, l_sSettingValue);
	getStaticBoxContext().getSettingValue(1, l_sSettingValue);
	m_ui64OffStimulationId=getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, l_sSettingValue);

	m_f64LastSample=0;
	m_bFirstSample=true;
	
	m_oStreamedMatrixDecoder.initialize(*this, 0);
	m_oStimulationEncoder.initialize(*this, 0);

	m_ui64ChannelIndex=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	if (m_ui64ChannelIndex == 0)
	{
		this->getLogManager() << LogLevel_Info << "Channel Index is 0. The channel indexing convention starts from 1.\n";
		return false;
	}
	m_ui64ChannelIndex--; // Convert from [1,n] indexing to [0,n-1] indexing used later

	return true;
}

boolean CSignChangeDetector::uninitialize(void)
{
	m_oStimulationEncoder.uninitialize();
	m_oStreamedMatrixDecoder.uninitialize();

	return true;
}

boolean CSignChangeDetector::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CSignChangeDetector::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	// Get a few convenience handles
	const OpenViBE::IMatrix* l_pMatrix = m_oStreamedMatrixDecoder.getOutputMatrix();
	OpenViBE::IStimulationSet* l_pStimulationSet = m_oStimulationEncoder.getInputStimulationSet();

	// We decode the stream matrix
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		const uint64 l_ui64StartTime=l_rDynamicBoxContext.getInputChunkStartTime(0, i);
		const uint64 l_ui64EndTime=l_rDynamicBoxContext.getInputChunkEndTime(0, i);

		m_oStreamedMatrixDecoder.decode(i);

		// if  we received the header
		if(m_oStreamedMatrixDecoder.isHeaderReceived())
		{
			//we analyse the header (meaning the input matrix size)
			if (l_pMatrix->getDimensionCount() != 2)
			{
				this->getLogManager() << LogLevel_ImportantWarning << "Streamed matrix must have exactly 2 dimensions\n";
				return false;
			}
			else
			{

				if (m_ui64ChannelIndex >= l_pMatrix->getDimensionSize(0))
				{
					this->getLogManager() << LogLevel_Info << "Channel Index out of bounds. Incoming matrix has fewer channels than specified index.\n";
					return false;
				}

				m_ui64SamplesPerChannel = l_pMatrix->getDimensionSize(1);
			}

			// we send a header on the stimulation output:
			m_oStimulationEncoder.encodeHeader();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64StartTime,l_ui64EndTime );
		}


		// if we received a buffer
		if(m_oStreamedMatrixDecoder.isBufferReceived())
		{
			l_pStimulationSet->clear();
			const float64* l_pData = l_pMatrix->getBuffer();
			// for each data sample of the buffer we look for sign change

			for (uint32 j=0;j< l_pMatrix->getDimensionSize(1);j++)
			{
				const float64 l_f64CurrentSample = l_pData[(m_ui64ChannelIndex * m_ui64SamplesPerChannel) + j];
				if(m_bFirstSample) 
				{
					m_f64LastSample = l_f64CurrentSample;
					m_bFirstSample = false;
				}

				// Change from positive to negative
				if(m_f64LastSample >= 0 && l_f64CurrentSample < 0)
				{
					const uint64 l_ui64Time = l_ui64StartTime + (l_ui64EndTime-l_ui64StartTime)*j/m_ui64SamplesPerChannel;

					l_pStimulationSet->appendStimulation(m_ui64OffStimulationId, l_ui64Time, 0);
				}

				// Change from negative to positive
				if(m_f64LastSample < 0 && l_f64CurrentSample >= 0)
				{
					const uint64 l_ui64Time = l_ui64StartTime + (l_ui64EndTime-l_ui64StartTime)*j/m_ui64SamplesPerChannel;

					l_pStimulationSet->appendStimulation(m_ui64OnStimulationId, l_ui64Time, 0);
				}

				m_f64LastSample = l_f64CurrentSample;

			}

			m_oStimulationEncoder.encodeBuffer();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);
		}

		// if we received the End
		if(m_oStreamedMatrixDecoder.isEndReceived())
		{
			// we send the End signal to the stimulation output:
			m_oStimulationEncoder.encodeEnd();
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);
		}

		// The stream matrix chunk i has been processed
		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}

	return true;
}
