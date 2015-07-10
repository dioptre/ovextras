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

OpenViBE::uint32 CBoxAlgorithmSpatialFilter::loadCoefficients(const OpenViBE::CString &rCoefficients, const char c1, const char c2) 
{
	this->getLogManager() << LogLevel_Trace << "Parsing coefficients matrix\n";

	m_vCoefficient.clear();

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
	m_vCoefficient.resize(l_u32count);

	// Ok, convert to floats
	this->getLogManager() << LogLevel_Trace << "Converting the coefficients to a float vector\n";
	l_sPtr = rCoefficients.toASCIIString();
	uint32 l_u32currentIdx = 0;
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
		if(!sscanf(l_sBuffer, "%lf", &m_vCoefficient[l_u32currentIdx])) 
		{
			this->getLogManager() << LogLevel_Error << "Error parsing coefficient nr. " << l_u32currentIdx << ", stopping.\n";
			break;
		}
		l_u32currentIdx++;
	}
	if(l_u32currentIdx != l_u32count) {
		this->getLogManager() << LogLevel_Warning << "Number of coefficients expected did not match the number read\n";
	}

	return l_u32currentIdx;
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

		((OpenViBEToolkit::TSignalDecoder<CBoxAlgorithmSpatialFilter>*)m_pStreamEncoder)->getOutputSamplingRate() 
			= ((OpenViBEToolkit::TSignalDecoder<CBoxAlgorithmSpatialFilter>*)m_pStreamDecoder)->getOutputSamplingRate();
	} 
	else if(l_oIdentifier==OV_TypeId_Spectrum)
	{
		m_pStreamDecoder=new OpenViBEToolkit::TSpectrumDecoder < CBoxAlgorithmSpatialFilter >(*this, 0);
		m_pStreamEncoder=new OpenViBEToolkit::TSpectrumEncoder < CBoxAlgorithmSpatialFilter >(*this, 0);

		((OpenViBEToolkit::TSpectrumDecoder<CBoxAlgorithmSpatialFilter>*)m_pStreamEncoder)->getOutputMinMaxFrequencyBands() 
			= ((OpenViBEToolkit::TSpectrumDecoder<CBoxAlgorithmSpatialFilter>*)m_pStreamDecoder)->getOutputMinMaxFrequencyBands();
	} 
	else
	{
		this->getLogManager() << LogLevel_Error << "Unhandled input stream type " << l_oIdentifier << "\n";
		return false;
	}

	const CString l_sCoefficient=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			
	loadCoefficients(l_sCoefficient, ' ', OV_Value_EnumeratedStringSeparator);

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
			const uint32 l_ui32OutputChannelCountSetting=(uint32)(uint64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
			const uint32 l_ui32InputChannelCountSetting=(uint32)(uint64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

			// we can treat them all as matrix decoders as they inherit from it
			const IMatrix *l_pInputMatrix = ((OpenViBEToolkit::TStreamedMatrixDecoder<CBoxAlgorithmSpatialFilter>*)m_pStreamDecoder)->getOutputMatrix();

			const uint32 l_ui32InputChannelCount=l_pInputMatrix->getDimensionSize(0);
			const uint32 l_ui32InputSamplesCount=l_pInputMatrix->getDimensionSize(1);

			if(l_ui32InputChannelCount == 0 || l_ui32InputSamplesCount == 0) 
			{
				this->getLogManager() << LogLevel_Error  << "Bad matrix size on input, [" << l_ui32InputChannelCount << " x " << l_ui32InputSamplesCount << "]\n";
				return false;
			}

			if(l_ui32InputChannelCount!=l_ui32InputChannelCountSetting)
			{
				this->getLogManager() << LogLevel_Error  << "Bad matrix size - Expected " << l_ui32InputChannelCountSetting << " input channels and received " << l_ui32InputChannelCount << " input channels\n";
				return false;
			}

			if(m_vCoefficient.size() != l_ui32OutputChannelCountSetting*l_ui32InputChannelCountSetting)
			{
				this->getLogManager() << LogLevel_Error << "Bad matrix size - The number of coefficients of the filter (" << uint32(m_vCoefficient.size()) << ") does not match the input/output channel count settings (" << l_ui32InputChannelCountSetting << "x" << l_ui32OutputChannelCountSetting << ")\n";
				return false;
			}

			const uint32 l_ui32OutputChannelCount=m_vCoefficient.size() / l_ui32InputChannelCount;

			IMatrix *l_pOutputMatrix = ((OpenViBEToolkit::TStreamedMatrixEncoder<CBoxAlgorithmSpatialFilter>*)m_pStreamEncoder)->getInputMatrix();
			l_pOutputMatrix->setDimensionCount(2);
			l_pOutputMatrix->setDimensionSize(0, l_ui32OutputChannelCount);
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

			System::Memory::set(l_pOutput, l_ui32SampleCount*l_ui32OutputChannelCount*sizeof(float64), 0);

			for(uint32 j=0; j<l_ui32OutputChannelCount; j++)
			{
				for(uint32 k=0; k<l_ui32InputChannelCount; k++)
				{
					for(uint32 l=0; l<l_ui32SampleCount; l++)
					{
						l_pOutput[j*l_ui32SampleCount+l]+=m_vCoefficient[j*l_ui32InputChannelCount+k]*l_pInput[k*l_ui32SampleCount+l];
					}
				}
			}

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
