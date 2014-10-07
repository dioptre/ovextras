#include "ovpCBoxAlgorithmCSVFileWriter.h"

#include <string>
#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;

CBoxAlgorithmCSVFileWriter::CBoxAlgorithmCSVFileWriter(void)
	:m_pFile(NULL)
	,m_fpRealProcess(NULL)
	,m_pStreamDecoder(NULL)
	,m_pMatrix(NULL)
	,m_bDeleteMatrix(false)
{
}

boolean CBoxAlgorithmCSVFileWriter::initialize(void)
{
	this->getStaticBoxContext().getInputType(0, m_oTypeIdentifier);

	CString l_sFilename=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_sSeparator=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_bUseCompression=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

	m_pFile=::fopen(l_sFilename.toASCIIString(), "wb");
	if(!m_pFile)
	{
		this->getLogManager() << LogLevel_ImportantWarning << "Could not open file [" << l_sFilename << "]\n";
		return false;
	}

	if(this->getTypeManager().isDerivedFromStream(m_oTypeIdentifier, OV_TypeId_StreamedMatrix))
	{
		if(m_oTypeIdentifier==OV_TypeId_Signal)
		{
			m_pStreamDecoder=new OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmCSVFileWriter >();
			m_pStreamDecoder->initialize(*this,0);
		}
		else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
		{
			m_pStreamDecoder=new OpenViBEToolkit::TSpectrumDecoder < CBoxAlgorithmCSVFileWriter >();
			m_pStreamDecoder->initialize(*this,0);
		}
		else
		{
			m_pStreamDecoder=new OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmCSVFileWriter >();
			m_pStreamDecoder->initialize(*this,0);
		}
		m_fpRealProcess=&CBoxAlgorithmCSVFileWriter::process_streamedMatrix;
	}
	else if(m_oTypeIdentifier==OV_TypeId_Stimulations)
	{
		m_pStreamDecoder=new OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmCSVFileWriter >();
		m_pStreamDecoder->initialize(*this,0);
		m_fpRealProcess=&CBoxAlgorithmCSVFileWriter::process_stimulation;
	}
	else
	{
		this->getLogManager() << LogLevel_ImportantWarning << "Invalid input type identifier " << this->getTypeManager().getTypeName(m_oTypeIdentifier) << "\n";
		return false;
	}

	if(m_bUseCompression)
	{
		this->getLogManager() << LogLevel_Warning << "Compression flag not used yet, the file will be flagged uncompressed and stored as is\n";
	}

	m_bFirstBuffer=true;
	return true;
}

boolean CBoxAlgorithmCSVFileWriter::uninitialize(void)
{
	if(m_pFile)
	{
		::fclose(m_pFile);
		m_pFile=NULL;
	}
	if(m_bDeleteMatrix)
	{
		delete m_pMatrix;
	}

	if(m_pStreamDecoder)
	{
		m_pStreamDecoder->uninitialize();
		delete m_pStreamDecoder;
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
		uint64 l_ui64StartTime=l_rDynamicBoxContext.getInputChunkStartTime(0, i);
		uint64 l_ui64EndTime=l_rDynamicBoxContext.getInputChunkEndTime(0, i);
		m_pStreamDecoder->decode(i);
		IMatrix* l_pMatrix = ((OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmCSVFileWriter >*)m_pStreamDecoder)->getOutputMatrix();
		if(m_pStreamDecoder->isHeaderReceived())
		{
			if(l_pMatrix->getDimensionCount() > 2 || l_pMatrix->getDimensionCount() < 1)
			{
				this->getLogManager() << LogLevel_ImportantWarning << "Input matrix does not have 1 or 2 dimensions - Could not write content in CSV file...\n";
				return false;
			}

			if( l_pMatrix->getDimensionCount() == 1 )
			{
				m_pMatrix = new CMatrix();
				m_bDeleteMatrix = true;
				m_pMatrix->setDimensionCount(2);
				m_pMatrix->setDimensionSize(0,1);
				m_pMatrix->setDimensionSize(1,l_pMatrix->getDimensionSize(0));
				for(uint32 i=0;i<l_pMatrix->getDimensionSize(0);i++)
				{
					m_pMatrix->setDimensionLabel(1,i,l_pMatrix->getDimensionLabel(0,i));
				}
			}
			else
			{
				m_pMatrix=l_pMatrix;
			}
//			std::cout<<&m_pMatrix<<" "<<&op_pMatrix<<"\n";
			::fprintf(m_pFile, "Time (s)");
			for(uint32 c=0; c<m_pMatrix->getDimensionSize(0); c++)
			{
				std::string l_sLabel(m_pMatrix->getDimensionLabel(0, c));
				while(l_sLabel.length()>0 && l_sLabel[l_sLabel.length()-1]==' ')
				{
					l_sLabel.erase(l_sLabel.length()-1);
				}
				::fprintf(m_pFile,
					"%s%s",
					m_sSeparator.toASCIIString(),
					l_sLabel.c_str());
			}

			if(m_oTypeIdentifier==OV_TypeId_Signal)
			{
				::fprintf(m_pFile,
					"%sSampling Rate",
					m_sSeparator.toASCIIString());
			}
			else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
			{
				::fprintf(m_pFile,
					"%sMin frequency band",
					m_sSeparator.toASCIIString());
				::fprintf(m_pFile,
					"%sMax frequency band",
					m_sSeparator.toASCIIString());
			}
			else
			{
			}

			::fprintf(m_pFile, "\n");
		}
		if(m_pStreamDecoder->isBufferReceived())
		{
			for(uint32 s=0; s<m_pMatrix->getDimensionSize(1); s++)
			{
				if(m_oTypeIdentifier==OV_TypeId_Signal)   ::fprintf(m_pFile, "%f", ((l_ui64StartTime+((s*(l_ui64EndTime-l_ui64StartTime))/l_pMatrix->getDimensionSize(1)))>>16)/65536.);
				if(m_oTypeIdentifier==OV_TypeId_Spectrum) ::fprintf(m_pFile, "%f", (l_ui64EndTime>>16)/65536.);
				for(uint32 c=0; c<m_pMatrix->getDimensionSize(0); c++)
				{
					::fprintf(m_pFile,
						"%s%f",
						m_sSeparator.toASCIIString(),
						l_pMatrix->getBuffer()[c*m_pMatrix->getDimensionSize(1)+s]);
				}

				if(m_bFirstBuffer)
				{
					if(m_oTypeIdentifier==OV_TypeId_Signal)
					{
						uint64 op_ui64SamplingFrequency =  ((OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmCSVFileWriter >*)m_pStreamDecoder)->getOutputSamplingRate();

						::fprintf(m_pFile,
							"%s%lli",
							m_sSeparator.toASCIIString(),
							(uint64)op_ui64SamplingFrequency);

						m_bFirstBuffer=false;
					}
					else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
					{
						IMatrix* op_pMinMaxFrequencyBand =  ((OpenViBEToolkit::TSpectrumDecoder < CBoxAlgorithmCSVFileWriter >*)m_pStreamDecoder)->getOutputMinMaxFrequencyBands();
						::fprintf(m_pFile,
							"%s%f",
							m_sSeparator.toASCIIString(),
							op_pMinMaxFrequencyBand->getBuffer()[s*2+0]);
						::fprintf(m_pFile,
							"%s%f",
							m_sSeparator.toASCIIString(),
							op_pMinMaxFrequencyBand->getBuffer()[s*2+1]);
					}
					else
					{
					}
				}
				else
				{
					if(m_oTypeIdentifier==OV_TypeId_Signal)
					{
						::fprintf(m_pFile,
							"%s",
							m_sSeparator.toASCIIString());
					}
					else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
					{
						::fprintf(m_pFile,
							"%s",
							m_sSeparator.toASCIIString());
						::fprintf(m_pFile,
							"%s",
							m_sSeparator.toASCIIString());
					}
					else
					{
					}
				}

				::fprintf(m_pFile, "\n");
			}

			m_bFirstBuffer=false;
		}
		if(m_pStreamDecoder->isEndReceived())
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
		m_pStreamDecoder->decode(i);
		if(m_pStreamDecoder->isHeaderReceived())
		{
			::fprintf(m_pFile,
				"Time (s)%sIdentifier%sDuration\n",
				m_sSeparator.toASCIIString(),
				m_sSeparator.toASCIIString());
		}
		if(m_pStreamDecoder->isBufferReceived())
		{
			IStimulationSet* l_pStimulationSet = ((OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmCSVFileWriter >*)m_pStreamDecoder)->getOutputStimulationSet();
			for(uint32 j=0; j<l_pStimulationSet->getStimulationCount(); j++)
			{
				::fprintf(m_pFile,
					"%f%s%llu%s%f\n",
					(l_pStimulationSet->getStimulationDate(j)>>16)/65536.0,
					m_sSeparator.toASCIIString(),
					l_pStimulationSet->getStimulationIdentifier(j),
					m_sSeparator.toASCIIString(),
					(l_pStimulationSet->getStimulationDuration(j)>>16)/65536.0);
			}
		}
		if(m_pStreamDecoder->isEndReceived())
		{
		}
		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}

	return true;
}
