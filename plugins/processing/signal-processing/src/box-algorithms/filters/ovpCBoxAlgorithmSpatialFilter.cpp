
#include "ovpCBoxAlgorithmSpatialFilter.h"

#include <system/ovCMemory.h>

#include <sstream>
#include <string>
#include <cstdio>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

#if defined(TARGET_HAS_ThirdPartyEIGEN)
#include <Eigen/Dense>
typedef Eigen::Matrix< double , Eigen::Dynamic , Eigen::Dynamic, Eigen::RowMajor > MatrixXdRowMajor;
#endif

OpenViBE::uint32 CBoxAlgorithmSpatialFilter::loadCoefficients(const OpenViBE::CString &rCoefficients, const char c1, const char c2, uint32 nRows, uint32 nCols) 
{
	this->getLogManager() << LogLevel_Trace << "Parsing coefficients matrix\n";

	// Count the number of entries
	// @Note To avoid doing a ton of subsequent memory allocations (very slow on Windows debug builds), we first count the number of entries in the vector. If the file format had specified the vector dimension, we wouldn't have to do this step.
	this->getLogManager() << LogLevel_Trace << "Counting the number of coefficients\n";
	uint32 l_u32count = 0;
	const char *l_sPtr = rCoefficients.toASCIIString();
	while(*l_sPtr!=0) 
	{
		// Skip separator characters
		while(*l_sPtr==c1 || *l_sPtr==c2) 
		{ 
			l_sPtr++; 
		}
		if(*l_sPtr==0) 
		{
			break;
		}
		// Ok, we have reached something that is not NULL or separator, assume its a number
		l_u32count++;
		// Skip the normal characters
		while(*l_sPtr!=c1 && *l_sPtr!=c2 && *l_sPtr!=0) 
		{ 
			l_sPtr++; 
		}
	}

	// Resize in one step for efficiency.
	m_oFilterBank.setDimensionCount(2);
	m_oFilterBank.setDimensionSize(0,nRows);
	m_oFilterBank.setDimensionSize(1,nCols);

	float64* l_pFilter = m_oFilterBank.getBuffer();

	// Ok, convert to floats
	this->getLogManager() << LogLevel_Trace << "Converting the coefficients to a float vector\n";
	l_sPtr = rCoefficients.toASCIIString();
	uint32 l_ui32currentIdx = 0;
	while(*l_sPtr!=0) 
	{
		const int BUFFSIZE=1024;
		char l_sBuffer[BUFFSIZE];
		// Skip separator characters
		while(*l_sPtr==c1 || *l_sPtr==c2) 
		{ 
			l_sPtr++; 
		}
		if(*l_sPtr==0) 
		{
			break;
		}
		// Copy the normal characters, don't exceed buffer size
		int i=0;
		while(*l_sPtr!=c1 && *l_sPtr!=c2 && *l_sPtr!=0) 
		{ 
			if(i<BUFFSIZE-1) {
				l_sBuffer[i++] = *l_sPtr; 
			}
			l_sPtr++;
		}
		l_sBuffer[i]=0;
		// Finally, convert
		if(!sscanf(l_sBuffer, "%lf", &l_pFilter[l_ui32currentIdx])) 
		{
			const uint32 l_ui32currentRow = l_ui32currentIdx/nRows + 1;
			const uint32 l_ui32currentCol = l_ui32currentIdx%nRows + 1;

			this->getLogManager() << LogLevel_Error << "Error parsing coefficient nr. " << l_ui32currentIdx 
				<< " for matrix position (" << l_ui32currentRow << "," << l_ui32currentCol << "), stopping.\n";
			break;
		}
		l_ui32currentIdx++;
	}
	if(l_ui32currentIdx != l_u32count) {
		this->getLogManager() << LogLevel_Warning << "Number of coefficients expected did not match the number read\n";
	}

	return l_ui32currentIdx;
}

boolean CBoxAlgorithmSpatialFilter::initialize(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	m_pStreamDecoder=NULL;
	m_pStreamEncoder=NULL;

	CIdentifier l_oIdentifier;
	l_rStaticBoxContext.getInputType(0, l_oIdentifier);

	if(l_oIdentifier==OV_TypeId_StreamedMatrix)
	{
		m_pStreamDecoder=new OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmSpatialFilter >(*this, 0);
		m_pStreamEncoder=new OpenViBEToolkit::TStreamedMatrixEncoder < CBoxAlgorithmSpatialFilter >(*this, 0);
	}
	else if(l_oIdentifier==OV_TypeId_Signal)
	{
		m_pStreamDecoder=new OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmSpatialFilter >(*this, 0);
		m_pStreamEncoder=new OpenViBEToolkit::TSignalEncoder < CBoxAlgorithmSpatialFilter >(*this, 0);

		((OpenViBEToolkit::TSignalEncoder<CBoxAlgorithmSpatialFilter>*)m_pStreamEncoder)->getInputSamplingRate().setReferenceTarget(
			((OpenViBEToolkit::TSignalDecoder<CBoxAlgorithmSpatialFilter>*)m_pStreamDecoder)->getOutputSamplingRate());
	} 
	else if(l_oIdentifier==OV_TypeId_Spectrum)
	{
		m_pStreamDecoder=new OpenViBEToolkit::TSpectrumDecoder < CBoxAlgorithmSpatialFilter >(*this, 0);
		m_pStreamEncoder=new OpenViBEToolkit::TSpectrumEncoder < CBoxAlgorithmSpatialFilter >(*this, 0);

		((OpenViBEToolkit::TSpectrumEncoder<CBoxAlgorithmSpatialFilter>*)m_pStreamEncoder)->getInputMinMaxFrequencyBands().setReferenceTarget(
			((OpenViBEToolkit::TSpectrumDecoder<CBoxAlgorithmSpatialFilter>*)m_pStreamDecoder)->getOutputMinMaxFrequencyBands());
	} 
	else
	{
		this->getLogManager() << LogLevel_Error << "Unhandled input stream type " << l_oIdentifier << "\n";
		return false;
	}

	// If we have a filter file, use dimensions and coefficients from that. Otherwise, use box config params.
	CString l_sFilterFile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);	
	if(l_sFilterFile!=CString(""))
	{
		if(!OpenViBEToolkit::Tools::Matrix::loadFromTextFile(m_oFilterBank, l_sFilterFile)) {
			this->getLogManager() << LogLevel_Error << "Unable to load filter file [" << l_sFilterFile << "]\n";
			return false;
		}
		if(m_oFilterBank.getDimensionCount() != 2)
		{
			this->getLogManager() << LogLevel_Error << "Specified filter matrix in the file didn't have 2 dimensions\n";
			return false;
		}

#ifdef DEBUG
		OpenViBEToolkit::Tools::Matrix::saveToTextFile(m_oFilterBank, "C:/temp/filters.txt");
#endif
	}
	else
	{
		const CString l_sCoefficient=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);	
		const uint32 l_ui32OutputChannelCountSetting=(uint32)(uint64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
		const uint32 l_ui32InputChannelCountSetting=(uint32)(uint64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

		if(loadCoefficients(l_sCoefficient, ' ', OV_Value_EnumeratedStringSeparator, l_ui32OutputChannelCountSetting, l_ui32InputChannelCountSetting) != l_ui32OutputChannelCountSetting * l_ui32InputChannelCountSetting)
		{
			return false;
		}
#ifdef DEBUG
		OpenViBEToolkit::Tools::Matrix::saveToTextFile(m_oFilterBank, "C:/temp/filters.txt");
#endif
	}

	return true;
}

boolean CBoxAlgorithmSpatialFilter::uninitialize(void)
{
	if(m_pStreamDecoder)
	{
		m_pStreamDecoder->uninitialize();
		delete m_pStreamDecoder;
		m_pStreamDecoder=NULL;
	}

	if(m_pStreamEncoder)
	{
		m_pStreamEncoder->uninitialize();
		delete m_pStreamEncoder;
		m_pStreamEncoder=NULL;
	}

	return true;
}

boolean CBoxAlgorithmSpatialFilter::processInput(uint32 ui32InputIndex)
{
	this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CBoxAlgorithmSpatialFilter::process(void)
{
	// IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{

		m_pStreamDecoder->decode(i);
		if(m_pStreamDecoder->isHeaderReceived()) 
		{
			// we can treat them all as matrix decoders as they all inherit from it
			const IMatrix *l_pInputMatrix = (static_cast< OpenViBEToolkit::TStreamedMatrixDecoder<CBoxAlgorithmSpatialFilter>* >(m_pStreamDecoder))->getOutputMatrix();

			const uint32 l_ui32InputChannelCount=l_pInputMatrix->getDimensionSize(0);
			const uint32 l_ui32InputSamplesCount=l_pInputMatrix->getDimensionSize(1);

			if(l_ui32InputChannelCount == 0 || l_ui32InputSamplesCount == 0) 
			{
				this->getLogManager() << LogLevel_Error  << "Bad matrix size on input, [" << l_ui32InputChannelCount << " x " << l_ui32InputSamplesCount << "]\n";
				return false;
			}

			const uint32 l_ui32FilterInputChannelCount = m_oFilterBank.getDimensionSize(1);
			const uint32 l_ui32FilterOutputChannelCount = m_oFilterBank.getDimensionSize(0);

			if(l_ui32InputChannelCount!=l_ui32FilterInputChannelCount)
			{
				this->getLogManager() << LogLevel_Error  << "Bad matrix size - Filter has " << l_ui32FilterInputChannelCount << " channels but data needs " << l_ui32InputChannelCount << " channels\n";
				return false;
			}

			IMatrix *l_pOutputMatrix = ((OpenViBEToolkit::TStreamedMatrixEncoder<CBoxAlgorithmSpatialFilter>*)m_pStreamEncoder)->getInputMatrix();
			l_pOutputMatrix->setDimensionCount(2);
			l_pOutputMatrix->setDimensionSize(0, l_ui32FilterOutputChannelCount);
			l_pOutputMatrix->setDimensionSize(1, l_ui32InputSamplesCount);

			// Name channels
			for(uint32 i=0;i<l_pOutputMatrix->getDimensionSize(0);i++)
			{
				char l_sBuffer[64];
				sprintf(l_sBuffer, "sFiltered %d", i);
				l_pOutputMatrix->setDimensionLabel(0, i, l_sBuffer);
			}

			m_pStreamEncoder->encodeHeader();
		}
		if(m_pStreamDecoder->isBufferReceived())
		{
			const IMatrix *l_pInputMatrix = ((OpenViBEToolkit::TStreamedMatrixDecoder<CBoxAlgorithmSpatialFilter>*)m_pStreamDecoder)->getOutputMatrix();
			IMatrix *l_pOutputMatrix = ((OpenViBEToolkit::TStreamedMatrixEncoder<CBoxAlgorithmSpatialFilter>*)m_pStreamEncoder)->getInputMatrix();

			const float64* l_pInput=l_pInputMatrix->getBuffer();
			float64* l_pOutput=l_pOutputMatrix->getBuffer();
			const uint32 l_ui32InputChannelCount=l_pInputMatrix->getDimensionSize(0);
			const uint32 l_ui32OutputChannelCount=l_pOutputMatrix->getDimensionSize(0);
			const uint32 l_ui32SampleCount=l_pInputMatrix->getDimensionSize(1);

#if TARGET_HAS_ThirdPartyEIGEN
			const Eigen::Map<MatrixXdRowMajor> l_oInputMapper(const_cast<float64*>(l_pInput), l_ui32InputChannelCount, l_ui32SampleCount);
			const Eigen::Map<MatrixXdRowMajor> l_oFilterMapper(const_cast<float64*>(m_oFilterBank.getBuffer()), m_oFilterBank.getDimensionSize(0),  m_oFilterBank.getDimensionSize(1));
			Eigen::Map<MatrixXdRowMajor> l_oOutputMapper(l_pOutput, l_ui32OutputChannelCount, l_ui32SampleCount);

			l_oOutputMapper = l_oFilterMapper * l_oInputMapper;
#else
			const float64* l_pFilter = m_oFilterBank.getBuffer();

			System::Memory::set(l_pOutput, l_ui32SampleCount*l_ui32OutputChannelCount*sizeof(float64), 0);

			for(uint32 j=0; j<l_ui32OutputChannelCount; j++)
			{
				for(uint32 k=0; k<l_ui32InputChannelCount; k++)
				{
					for(uint32 l=0; l<l_ui32SampleCount; l++)
					{
						l_pOutput[j*l_ui32SampleCount+l] += l_pFilter[j*l_ui32InputChannelCount+k]*l_pInput[k*l_ui32SampleCount+l];
					}
				}
			}
#endif
			m_pStreamEncoder->encodeBuffer();
		}
		if(m_pStreamDecoder->isEndReceived())
		{
			m_pStreamEncoder->encodeEnd();
		}

		l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
	}

	return true;
}

