#if defined TARGET_HAS_ThirdPartyITPP

#include "ovpCFastICA.h"

#include <iostream>
#include <sstream>
/*
#include <itpp/itstat.h>
#include <itpp/itsignal.h>

using namespace itpp;
*/
using namespace OpenViBE;
using namespace OpenViBE::Plugins;
using namespace OpenViBE::Kernel;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;
using namespace OpenViBEToolkit;
using namespace std;

void CFastICA::computeICA(void)
{
	const uint32 l_ui32ChannelCount = m_oDecoder.getOutputMatrix()->getDimensionSize(0);
	const uint32 l_ui32SampleCount = m_oDecoder.getOutputMatrix()->getDimensionSize(1);
	const float64 *l_pInputBuffer = m_oDecoder.getOutputMatrix()->getBuffer();
	// Jeff modif
	IMatrix* l_pEncoderMatrix = m_oEncoder.getInputMatrix();
	l_pEncoderMatrix->setDimensionCount(2);
	l_pEncoderMatrix->setDimensionSize(0, l_ui32ChannelCount);
	l_pEncoderMatrix->setDimensionSize(1, l_ui32SampleCount);
	//
	float64 *l_pOutputBuffer = m_oEncoder.getInputMatrix()->getBuffer();

	mat sources(l_ui32ChannelCount, l_ui32SampleCount);
	mat Buffer_sources(l_ui32ChannelCount, m_ui32Buff_Size);
	mat ICs(l_ui32ChannelCount, l_ui32SampleCount);
	//mat ICs(l_ui32ChannelCount, m_ui32Buff_Size);
	//mat Mix_mat(l_ui32ChannelCount, l_ui32ChannelCount);
	mat Sep_mat(l_ui32ChannelCount, l_ui32ChannelCount);
	//mat Dewhite(l_ui32ChannelCount, l_ui32ChannelCount);
	for (uint32 i=0;  i < l_ui32ChannelCount; i++)
	{
		for(uint32 j=0 ; j<m_ui32Buff_Size ; j++)
		{
			if(j<m_ui32Buff_Size-l_ui32SampleCount)
			{
				fifo_buffer[i*m_ui32Buff_Size+j+l_ui32SampleCount]=fifo_buffer[i*m_ui32Buff_Size+j]; // memory shift
				if(j<l_ui32SampleCount)
				{
					fifo_buffer[i*m_ui32Buff_Size+j] = (double)l_pInputBuffer[i*l_ui32SampleCount+l_ui32SampleCount-1-j];
					sources((int)i, (int)j) =  (double)l_pInputBuffer[i*l_ui32SampleCount+j];
				}			
			}
			if(!m_bTrained)  // Sep_mat is set to identity
			{
				if(i==j)
					Sep_mat((int)i, (int)j) = 1.0;
				else if(j<l_ui32ChannelCount)
					Sep_mat((int)i, (int)j) = 0.0;
			}
			else			// Sep_mat is set to the saved fastica demixing matrix
			{
				if(j<l_ui32ChannelCount)
					Sep_mat((int)i, (int)j) = demixer[i*l_ui32ChannelCount+j];
			}
			Buffer_sources((int)i, (int)(m_ui32Buff_Size-1-j)) = fifo_buffer[i*m_ui32Buff_Size+j];
		}
	}
	m_ui32Samp_Nb += l_ui32SampleCount;
	if((m_ui32Samp_Nb >= m_ui32Buff_Size)&&(m_bTrained==false))
	{
		this->getLogManager() << LogLevel_Trace << "Instanciating the Fast_ICA object with " << m_ui32Samp_Nb << " samples.\n";
		Fast_ICA fastica(Buffer_sources);
		this->getLogManager() << LogLevel_Trace << "Setting the number of ICs to extract to " << sources.rows() << " and configuring FastICA...\n";
		
		fastica.set_nrof_independent_components(Buffer_sources.rows()); // WARNING: m_ui32Nb_ICs forced to l_ui32ChannelCount
		fastica.set_non_linearity(FICA_NONLIN_TANH);
		fastica.set_approach(FICA_APPROACH_DEFL);				//int in_maxNumIterations = 100000;
		fastica.set_max_num_iterations(m_ui32NbRep_max);		//int in_maxNumFine = 100;
		fastica.set_fine_tune(m_bSetFineTune);
		fastica.set_max_fine_tune(m_ui32NbTune_max);
		fastica.set_non_linearity(m_ui32Non_Lin);
		fastica.set_mu(m_ui64Set_Mu);
		fastica.set_epsilon(m_ui64Epsilon);
		
		//if(m_ui32Samp_Nb>l_ui32SampleCount) fastica.set_init_guess((Dewhite * Dewhite.T()) * Sep_mat.T());	
		this->getLogManager() << LogLevel_Trace << "Explicit launch of the Fast_ICA algorithm. Can occasionnally take time.\n";
		fastica.separate();
		this->getLogManager() << LogLevel_Trace << "Retrieving ICs from fastica .\n";
		//ICs = fastica.get_independent_components();
		//this->getLogManager() << LogLevel_Trace << "Retrieving mixing matrix from fastica .\n";
		//Mix_mat = fastica.get_mixing_matrix();
		this->getLogManager() << LogLevel_Trace << "Retrieving separating matrix from fastica .\n";
		Sep_mat = fastica.get_separating_matrix();
		//Dewhite = fastica.get_dewhitening_matrix();
		m_bTrained = true;
		for(uint32 i=0;  i < l_ui32ChannelCount; i++)
			for(uint32 j=0;  j < l_ui32ChannelCount; j++)
				demixer[i*l_ui32ChannelCount+j] = Sep_mat((int)i,(int)j);
	}
	// Effective demixing (ICA after m_ui32Duration sec)
	ICs = Sep_mat * sources;

	//this->getLogManager() << LogLevel_Trace << "Filling output buffer with ICs .\n";
	for (uint32 i=0;  i < l_ui32ChannelCount; i++)
		for(uint32 j=0 ; j < l_ui32SampleCount ; j++)
			l_pOutputBuffer[i*l_ui32SampleCount+j] = ICs((int)i,(int)j);
}

CFastICA::CFastICA(void)
{
}

void CFastICA::release(void)
{
	delete this;
}

boolean CFastICA::initialize()
{
	m_oDecoder.initialize(*this, 0);
	m_oEncoder.initialize(*this, 0);

	m_ui32Nb_ICs    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_ui32Duration  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_ui32NbRep_max = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	m_bSetFineTune  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
	m_ui32NbTune_max= FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
	m_ui32Non_Lin   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
	m_ui64Set_Mu    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);
	m_ui64Epsilon   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);
	m_sSpatialFilterFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8);
	m_bSaveAsFile   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 9);

	return true;
}

boolean CFastICA::uninitialize()
{
	m_oEncoder.uninitialize();
	m_oDecoder.uninitialize();
	// m_vBuffer_sources = NULL;  // Jeff
	delete[] fifo_buffer;
	delete[] demixer;
	return true;
}

boolean CFastICA::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CFastICA::process()
{
	IDynamicBoxContext* l_pDynamicBoxContext=getBoxAlgorithmContext()->getDynamicBoxContext();

	// Process input data
	for(uint32 i=0; i<l_pDynamicBoxContext->getInputChunkCount(0); i++)
	{
		m_oDecoder.decode(i);

		if(m_oDecoder.isHeaderReceived()) 
		{
			// Set the output (encoder) matrix prorperties from the input (decoder)
			IMatrix* l_pEncoderMatrix = m_oEncoder.getInputMatrix();
			// Jeff (initializing the buffer)
			m_ui32Buff_Size = m_oDecoder.getOutputSamplingRate()*m_ui32Duration;
			const uint32 l_ui32ChannelCount = m_oDecoder.getOutputMatrix()->getDimensionSize(0);
			this->getLogManager() << LogLevel_Trace << "FIFO buffer initialized with " << l_ui32ChannelCount*m_ui32Buff_Size << ".\n";
			fifo_buffer = new OpenViBE::float64[l_ui32ChannelCount*m_ui32Buff_Size];
			demixer     = new OpenViBE::float64[l_ui32ChannelCount*l_ui32ChannelCount];
			for(uint32 i=0 ; i<l_ui32ChannelCount*m_ui32Buff_Size ; i++)
				fifo_buffer[i]=0.0;
			m_ui32Samp_Nb = 0;
			m_bTrained = false;
			//
			OpenViBEToolkit::Tools::Matrix::copyDescription(*l_pEncoderMatrix, *m_oDecoder.getOutputMatrix());
			m_oEncoder.getInputSamplingRate() = m_oDecoder.getOutputSamplingRate();

			for(uint32 i=0 ; i<l_pEncoderMatrix->getDimensionSize(0) ; i++)
			{
				char l_sBuffer[64];
				sprintf(l_sBuffer, "IC %d", i+1);
				l_pEncoderMatrix->setDimensionLabel(0,i, l_sBuffer);
			}

			m_oEncoder.encodeHeader();

			getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, 0, 0);
		}

		if(m_oDecoder.isBufferReceived()) 
		{
			const uint64 l_ui64LastChunkStartTime = l_pDynamicBoxContext->getInputChunkStartTime(0,i);
			const uint64 l_ui64LastChunkEndTime = l_pDynamicBoxContext->getInputChunkEndTime(0,i);
			
			const uint32 l_ui32ChannelCount = m_oDecoder.getOutputMatrix()->getDimensionSize(0);

			computeICA();

			if((m_bSaveAsFile)&&(m_bTrained)&&(m_bFileSaved==false)) 
			{
				OpenViBE::CMatrix l_oDemixer;
				l_oDemixer.setDimensionCount(2);
				l_oDemixer.setDimensionSize(0,l_ui32ChannelCount);
				l_oDemixer.setDimensionSize(1,l_ui32ChannelCount);
				float64* l_pDemixer = l_oDemixer.getBuffer();
				for(uint32 i=0;i<l_ui32ChannelCount;i++)
					for(uint32 j=0; j<l_ui32ChannelCount; j++)
						l_pDemixer[i*l_ui32ChannelCount+j] = demixer[i*l_ui32ChannelCount+j];
				if(!OpenViBEToolkit::Tools::Matrix::saveToTextFile(l_oDemixer, m_sSpatialFilterFilename))
					this->getLogManager() << LogLevel_Warning << "Unable to save to [" << m_sSpatialFilterFilename << "\n";			
				m_bFileSaved=true;
			} 

			m_oEncoder.encodeBuffer();

			getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, l_ui64LastChunkStartTime, l_ui64LastChunkEndTime);
		}

		if(m_oDecoder.isEndReceived()) 
		{
			// NOP
		}
	}

	return true;
}

#endif // TARGET_HAS_ThirdPartyITPP
