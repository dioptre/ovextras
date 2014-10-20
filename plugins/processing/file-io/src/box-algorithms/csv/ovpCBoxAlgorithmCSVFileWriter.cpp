#include "ovpCBoxAlgorithmCSVFileWriter.h"

#include <string>
#include <iostream>

#include "openvibe/ovITimeArithmetics.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;

CBoxAlgorithmCSVFileWriter::CBoxAlgorithmCSVFileWriter(void)
	:
	m_fpRealProcess(NULL)
	,m_pStreamDecoder(NULL)
	,m_pMatrix(NULL)
	,m_bDeleteMatrix(false)
{
}

boolean CBoxAlgorithmCSVFileWriter::initialize(void)
{
	this->getStaticBoxContext().getInputType(0, m_oTypeIdentifier);

	const CString l_sFilename=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_sSeparator=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	const uint64 l_ui64Precision=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

	m_oFileStream.open(l_sFilename.toASCIIString(), std::ios::trunc);
	if(!m_oFileStream)
	{
		this->getLogManager() << LogLevel_ImportantWarning << "Could not open file [" << l_sFilename << "] for writing\n";
		return false;
	}

	m_oFileStream << std::scientific;
	m_oFileStream.precision(static_cast<std::streamsize>(l_ui64Precision));

	if(this->getTypeManager().isDerivedFromStream(m_oTypeIdentifier, OV_TypeId_StreamedMatrix))
	{
		if(m_oTypeIdentifier==OV_TypeId_Signal)
		{
			m_pStreamDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamDecoder));
			m_pStreamDecoder->initialize();
			op_ui64SamplingFrequency.initialize(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_SamplingRate));
		}
		else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
		{
			m_pStreamDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumStreamDecoder));
			m_pStreamDecoder->initialize();
			op_pMinMaxFrequencyBand.initialize(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_SpectrumStreamDecoder_OutputParameterId_MinMaxFrequencyBands));
		}
		else if(m_oTypeIdentifier==OV_TypeId_FeatureVector)
		{
			m_pStreamDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorStreamDecoder));
			m_pStreamDecoder->initialize();
		}
		else
		{
			if(m_oTypeIdentifier!=OV_TypeId_StreamedMatrix)
			{
				this->getLogManager() << LogLevel_Info << "Input is a type derived from matrix that the box doesn't recognize, decoding as Streamed Matrix\n";
			}
			m_pStreamDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixStreamDecoder));
			m_pStreamDecoder->initialize();
		}

		ip_pMemoryBuffer.initialize(m_pStreamDecoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_InputParameterId_MemoryBufferToDecode));
		op_pMatrix.initialize(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_OutputParameterId_Matrix));
		m_fpRealProcess=&CBoxAlgorithmCSVFileWriter::process_streamedMatrix;
	}
	else if(m_oTypeIdentifier==OV_TypeId_Stimulations)
	{
		m_pStreamDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamDecoder));
		m_pStreamDecoder->initialize();
		ip_pMemoryBuffer.initialize(m_pStreamDecoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_InputParameterId_MemoryBufferToDecode));
		op_pStimulationSet.initialize(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_OutputParameterId_StimulationSet));
		m_fpRealProcess=&CBoxAlgorithmCSVFileWriter::process_stimulation;
	}
	else
	{
		this->getLogManager() << LogLevel_ImportantWarning << "Invalid input type identifier " << this->getTypeManager().getTypeName(m_oTypeIdentifier) << "\n";
		return false;
	}

	m_ui64SampleCount = 0;

	m_bFirstBuffer=true;
	return true;
}

boolean CBoxAlgorithmCSVFileWriter::uninitialize(void)
{
	if(m_oFileStream.is_open())
	{
		m_oFileStream.close();
	}

	if(m_bDeleteMatrix)
	{
		delete m_pMatrix;
		m_pMatrix = NULL;
	}
	op_pStimulationSet.uninitialize();
	op_pMatrix.uninitialize();
	ip_pMemoryBuffer.uninitialize();

	if(m_pStreamDecoder)
	{
		m_pStreamDecoder->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pStreamDecoder);
		m_pStreamDecoder=NULL;
	}

	return true;
}

boolean CBoxAlgorithmCSVFileWriter::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CBoxAlgorithmCSVFileWriter::process(void)
{
	return (this->*m_fpRealProcess)();
}

boolean CBoxAlgorithmCSVFileWriter::process_streamedMatrix(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		const uint64 l_ui64StartTime=l_rDynamicBoxContext.getInputChunkStartTime(0, i);
		const uint64 l_ui64EndTime=l_rDynamicBoxContext.getInputChunkEndTime(0, i);
		ip_pMemoryBuffer=l_rDynamicBoxContext.getInputChunk(0, i);
		m_pStreamDecoder->process();
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			if(op_pMatrix->getDimensionCount() > 2 || op_pMatrix->getDimensionCount() < 1)
			{
				this->getLogManager() << LogLevel_ImportantWarning << "Input matrix does not have 1 or 2 dimensions - Cannot write content in CSV file...\n";
				return false;
			}

			// The matrix is a vector, make a [n x 1] matrix to represent it
			if( op_pMatrix->getDimensionCount() == 1 )
			{
				m_pMatrix = new CMatrix();
				m_bDeleteMatrix = true;
				m_pMatrix->setDimensionCount(2);
				if(m_oTypeIdentifier==OV_TypeId_FeatureVector)
				{
					// Flip, [n channels X 1 sample]
					m_pMatrix->setDimensionSize(0,op_pMatrix->getDimensionSize(0));
					m_pMatrix->setDimensionSize(1,1);
					for(uint32 i=0;i<op_pMatrix->getDimensionSize(0);i++)
					{
						m_pMatrix->setDimensionLabel(0,i,op_pMatrix->getDimensionLabel(0,i));
					}
				} 
				else
				{
					// As-is, [1 channel X n samples]
					m_pMatrix->setDimensionSize(0,1);
					m_pMatrix->setDimensionSize(1,op_pMatrix->getDimensionSize(0));
					for(uint32 i=0;i<op_pMatrix->getDimensionSize(0);i++)
					{
						m_pMatrix->setDimensionLabel(1,i,op_pMatrix->getDimensionLabel(0,i));
					}
				}

			}
			else
			{
				m_pMatrix=op_pMatrix;
			}
//			std::cout<<&m_pMatrix<<" "<<&op_pMatrix<<"\n";
			m_oFileStream << "Time (s)";
			for(uint32 c=0; c<m_pMatrix->getDimensionSize(0); c++)
			{
				std::string l_sLabel(m_pMatrix->getDimensionLabel(0, c));
				while(l_sLabel.length()>0 && l_sLabel[l_sLabel.length()-1]==' ')
				{
					l_sLabel.erase(l_sLabel.length()-1);
				}
				m_oFileStream << m_sSeparator.toASCIIString() << l_sLabel.c_str();
			}

			if(m_oTypeIdentifier==OV_TypeId_Signal)
			{
				m_oFileStream << m_sSeparator.toASCIIString() << "Sampling Rate";
			}
			else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
			{
				m_oFileStream << m_sSeparator << "Min frequency band";
				m_oFileStream << m_sSeparator << "Max frequency band";
			}
			else
			{
			}

			m_oFileStream << "\n";
		}
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_OutputTriggerId_ReceivedBuffer))
		{
			const uint32 l_ui32NumChannels = m_pMatrix->getDimensionSize(0);
			const uint32 l_ui32NumSamples = m_pMatrix->getDimensionSize(1);

			for(uint32 s=0; s<l_ui32NumSamples; s++)
			{
				if(m_oTypeIdentifier==OV_TypeId_StreamedMatrix || m_oTypeIdentifier==OV_TypeId_FeatureVector)
				{
					m_oFileStream << ITimeArithmetics::timeToSeconds(l_ui64StartTime);
				}
				else if(m_oTypeIdentifier==OV_TypeId_Signal)
				{
					m_oFileStream << ITimeArithmetics::timeToSeconds(ITimeArithmetics::sampleCountToTime(static_cast<uint64>(op_ui64SamplingFrequency), m_ui64SampleCount+s));
				}
				else if(m_oTypeIdentifier==OV_TypeId_Spectrum) 
				{
					m_oFileStream << ITimeArithmetics::timeToSeconds(l_ui64EndTime);
				}
				for(uint32 c=0; c<l_ui32NumChannels; c++)
				{
					m_oFileStream << m_sSeparator.toASCIIString() << op_pMatrix->getBuffer()[c*l_ui32NumSamples+s];
				}

				if(m_bFirstBuffer)
				{
					if(m_oTypeIdentifier==OV_TypeId_Signal)
					{
						m_oFileStream << m_sSeparator.toASCIIString() << (uint64)op_ui64SamplingFrequency;

						m_bFirstBuffer=false;
					}
					else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
					{
						m_oFileStream << m_sSeparator.toASCIIString() << op_pMinMaxFrequencyBand->getBuffer()[s*2+0];
						m_oFileStream << m_sSeparator.toASCIIString() << op_pMinMaxFrequencyBand->getBuffer()[s*2+1];
					}
					else
					{
					}
				}
				else
				{
					if(m_oTypeIdentifier==OV_TypeId_Signal)
					{
						m_oFileStream << m_sSeparator.toASCIIString();
					}
					else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
					{
						m_oFileStream << m_sSeparator.toASCIIString() << m_sSeparator.toASCIIString();
					}
					else
					{
					}
				}

				m_oFileStream << "\n";
			}
			m_ui64SampleCount += l_ui32NumSamples;

			m_bFirstBuffer=false;
		}
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_OutputTriggerId_ReceivedEnd))
		{
		}
		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}

	return true;
}

boolean CBoxAlgorithmCSVFileWriter::process_stimulation(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		ip_pMemoryBuffer=l_rDynamicBoxContext.getInputChunk(0, i);
		m_pStreamDecoder->process();
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			m_oFileStream << "Time (s)" << m_sSeparator.toASCIIString() << "Identifier" << m_sSeparator.toASCIIString() << "Duration\n";
		}
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedBuffer))
		{
			for(uint32 j=0; j<op_pStimulationSet->getStimulationCount(); j++)
			{
				m_oFileStream << ITimeArithmetics::timeToSeconds(op_pStimulationSet->getStimulationDate(j))
					<< m_sSeparator.toASCIIString() 
					<< op_pStimulationSet->getStimulationIdentifier(j)
					<< m_sSeparator.toASCIIString() 
					<< ITimeArithmetics::timeToSeconds(op_pStimulationSet->getStimulationDuration(j))
					<< "\n";
			}
		}
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedEnd))
		{
		}
		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}

	return true;
}
