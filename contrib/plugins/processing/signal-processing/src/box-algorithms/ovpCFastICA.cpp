#if defined TARGET_HAS_ThirdPartyITPP

#include "ovpCFastICA.h"

#include <iostream>
#include <sstream>

#include <itpp/itstat.h>
#include <itpp/itsignal.h>

using namespace itpp;

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

	const uint32 l_ui32NumOfICs =  m_oEncoder.getInputMatrix()->getDimensionSize(0);

	mat sources(l_ui32ChannelCount, l_ui32SampleCount);		  // current block (for decomposing)
	mat Buffer_sources(l_ui32ChannelCount, m_ui32Buff_Size);  // accumulated blocks (for training)
	mat ICs(l_ui32NumOfICs, l_ui32SampleCount);
	//mat Mix_mat(l_ui32ChannelCount, l_ui32ChannelCount);
	mat Sep_mat(l_ui32NumOfICs, l_ui32ChannelCount);
	//mat Dewhite(l_ui32ChannelCount, l_ui32ChannelCount);

	// Append the data to a FIFO buffer
	for (uint32 i=0;  i < l_ui32ChannelCount; i++)
	{
		for(uint32 j=0 ; j<m_ui32Buff_Size ; j++)
		{
			if(j<m_ui32Buff_Size-l_ui32SampleCount)
			{
				m_pFifoBuffer[i*m_ui32Buff_Size+j+l_ui32SampleCount]=m_pFifoBuffer[i*m_ui32Buff_Size+j]; // memory shift
				if(j<l_ui32SampleCount)
				{
					m_pFifoBuffer[i*m_ui32Buff_Size+j] = (double)l_pInputBuffer[i*l_ui32SampleCount+l_ui32SampleCount-1-j];
					sources((int)i, (int)j) =  (double)l_pInputBuffer[i*l_ui32SampleCount+j];
				}			
			}

			Buffer_sources((int)i, (int)(m_ui32Buff_Size-1-j)) = m_pFifoBuffer[i*m_ui32Buff_Size+j];
		}
	}

	m_ui32Samp_Nb += l_ui32SampleCount;
	if((m_ui32Samp_Nb >= m_ui32Buff_Size)&&(m_bTrained==false))
	{
		this->getLogManager() << LogLevel_Trace << "Instanciating the Fast_ICA object with " << m_ui32Samp_Nb << " samples.\n";
		Fast_ICA fastica(Buffer_sources);
		this->getLogManager() << LogLevel_Trace << "Setting the number of ICs to extract to " << l_ui32NumOfICs << " and configuring FastICA...\n";
		
		if(m_ui64Mode == OVP_TypeId_FastICA_OperatingMode_PCA || m_ui64Mode == OVP_TypeId_FastICA_OperatingMode_Whiten)
		{
			fastica.set_pca_only(true);
		}
		else
		{
			fastica.set_approach(static_cast<int>(m_ui64Type));
			fastica.set_non_linearity(static_cast<int>(m_ui64Non_Lin));
			fastica.set_max_num_iterations(m_ui32NbRep_max);
			fastica.set_fine_tune(m_bSetFineTune);
			fastica.set_max_fine_tune(m_ui32NbTune_max);
			fastica.set_mu(m_ui64Set_Mu);
			fastica.set_epsilon(m_ui64Epsilon);
		}

		fastica.set_nrof_independent_components(l_ui32NumOfICs); 

		//if(m_ui32Samp_Nb>l_ui32SampleCount) fastica.set_init_guess((Dewhite * Dewhite.T()) * Sep_mat.T());	
		this->getLogManager() << LogLevel_Trace << "Explicit launch of the Fast_ICA algorithm. Can occasionally take time.\n";
		fastica.separate();
		this->getLogManager() << LogLevel_Trace << "Retrieving separating matrix from fastica .\n";
		if(m_ui64Mode == OVP_TypeId_FastICA_OperatingMode_PCA)
		{
			Sep_mat = fastica.get_principal_eigenvectors().transpose();
		} 
		else if(m_ui64Mode == OVP_TypeId_FastICA_OperatingMode_Whiten)
		{
			Sep_mat = fastica.get_whitening_matrix();
		}
		else
		{
			Sep_mat = fastica.get_separating_matrix();
		}

		m_bTrained = true;
		float64 *l_pDemixer = m_oDemixer.getBuffer();
		for(uint32 i=0;  i < l_ui32NumOfICs; i++)
		{
			for(uint32 j=0;  j < l_ui32ChannelCount; j++)
			{
				l_pDemixer[i*l_ui32ChannelCount+j] = Sep_mat((int)i,(int)j);
			}
		}
	}
	else
	{
		// Use the previously stored matrix
		const float64* l_pDemixer = m_oDemixer.getBuffer();
		for(uint32 i=0;i<l_ui32NumOfICs;i++) 
		{
			for(uint32 j=0;j<l_ui32ChannelCount;j++)
			{
				Sep_mat((int)i, (int)j) = l_pDemixer[i*l_ui32ChannelCount+j];
			}
		}
	}

	// Effective demixing (ICA after m_ui32Duration sec)
	ICs = Sep_mat * sources;

	float64 *l_pOutputBuffer = m_oEncoder.getInputMatrix()->getBuffer();
	//this->getLogManager() << LogLevel_Trace << "Filling output buffer with ICs .\n";
	for (uint32 i=0;  i < l_ui32NumOfICs; i++)
	{
		for(uint32 j=0 ; j < l_ui32SampleCount ; j++)
		{
			l_pOutputBuffer[i*l_ui32SampleCount+j] = ICs((int)i,(int)j);
		}
	}
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

	m_ui32Nb_ICs             = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_ui64Mode               = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_ui32Duration           = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	m_ui64Type               = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
	m_ui32NbRep_max          = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
	m_bSetFineTune           = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
	m_ui32NbTune_max         = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);
	m_ui64Non_Lin            = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);
	m_ui64Set_Mu             = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8);
	m_ui64Epsilon            = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 9);
	m_sSpatialFilterFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 10);
	m_bSaveAsFile            = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 11);

	m_pFifoBuffer = NULL;
	m_bFileSaved = false;

	if(m_bSaveAsFile && m_sSpatialFilterFilename==CString(""))
	{
		this->getLogManager() << "If save is enabled, filename must be provided\n";
		return false;
	}

	return true;
}

boolean CFastICA::uninitialize()
{
	m_oEncoder.uninitialize();
	m_oDecoder.uninitialize();
	if(m_pFifoBuffer)
	{
		delete[] m_pFifoBuffer;
		m_pFifoBuffer = NULL;
	}

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
			if(m_oDecoder.getOutputMatrix()->getDimensionCount()!=2)
			{
				this->getLogManager() << LogLevel_Error << "Needs a 2 dimensional (rows x cols) matrix as input\n";
				return false;
			}
			m_ui32Buff_Size = static_cast<uint32>(m_oDecoder.getOutputSamplingRate())*m_ui32Duration;
			const uint32 l_ui32ChannelCount = m_oDecoder.getOutputMatrix()->getDimensionSize(0);
			const uint32 l_ui32SampleCount = m_oDecoder.getOutputMatrix()->getDimensionSize(1);

			if(m_pFifoBuffer)
			{
				delete[] m_pFifoBuffer;
			}
			m_pFifoBuffer = new OpenViBE::float64[l_ui32ChannelCount*m_ui32Buff_Size];
			this->getLogManager() << LogLevel_Trace << "FIFO buffer initialized with " << l_ui32ChannelCount*m_ui32Buff_Size << ".\n";

			if(m_ui32Nb_ICs > l_ui32ChannelCount)
			{
				this->getLogManager() << LogLevel_Warning << "Trying to estimate more components than channels, truncating\n";
				m_ui32Nb_ICs = l_ui32ChannelCount;
			}

			for(uint32 j=0 ; j<l_ui32ChannelCount*m_ui32Buff_Size ; j++)
			{
				m_pFifoBuffer[j]=0.0;
			}
			m_ui32Samp_Nb = 0;
			m_bTrained = false;

			IMatrix* l_pEncoderMatrix = m_oEncoder.getInputMatrix();
			l_pEncoderMatrix->setDimensionCount(2);
			l_pEncoderMatrix->setDimensionSize(0, m_ui32Nb_ICs);
			l_pEncoderMatrix->setDimensionSize(1, l_ui32SampleCount);

			m_oEncoder.getInputSamplingRate() = m_oDecoder.getOutputSamplingRate();

			char l_sPrefix[32];
			if(m_ui64Mode == OVP_TypeId_FastICA_OperatingMode_PCA)
				sprintf(l_sPrefix, "PC");
			else if(m_ui64Mode == OVP_TypeId_FastICA_OperatingMode_Whiten)
				sprintf(l_sPrefix, "Wh");
			else
				sprintf(l_sPrefix, "IC");

			for(uint32 c=0 ; c < m_ui32Nb_ICs ; c++)
			{
				char l_sBuffer[64];
				sprintf(l_sBuffer, "%s %d", l_sPrefix, c+1);
				l_pEncoderMatrix->setDimensionLabel(0,c, l_sBuffer);
			}

			m_oEncoder.encodeHeader();

			m_oDemixer.setDimensionCount(2);
			m_oDemixer.setDimensionSize(0,m_ui32Nb_ICs);
			m_oDemixer.setDimensionSize(1,l_ui32ChannelCount);

			// Set the demixer to (partial) identity matrix to start with
			OpenViBEToolkit::Tools::Matrix::clearContent(m_oDemixer);
			float64* l_pDemixer = m_oDemixer.getBuffer();
			
			for(uint32 c=0;c<m_ui32Nb_ICs;c++)
			{
				l_pDemixer[c*l_ui32ChannelCount+c] = 1.0;
			}

			getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, 0, 0);
		}

		if(m_oDecoder.isBufferReceived()) 
		{
			const uint64 l_ui64LastChunkStartTime = l_pDynamicBoxContext->getInputChunkStartTime(0,i);
			const uint64 l_ui64LastChunkEndTime   = l_pDynamicBoxContext->getInputChunkEndTime(0,i);
			
			computeICA();

			if((m_bSaveAsFile) && (m_bTrained) && (m_bFileSaved==false)) 
			{
				if(!OpenViBEToolkit::Tools::Matrix::saveToTextFile(m_oDemixer, m_sSpatialFilterFilename))
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
