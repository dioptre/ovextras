#include "ovpCBoxAlgorithmMultipleSpatialFilters.h"

#if defined TARGET_HAS_ThirdPartyITPP

#include <system/Memory.h>

#include <fstream>
#include <sstream>
#include <string>
#include <cstdio>
#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessingCoAdapt;

/*OpenViBE::uint32 CBoxAlgorithmMultipleSpatialFilters::loadCoefficients(const OpenViBE::CString &rCoefficients, const char c1, const char c2) 
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
}*/

boolean CBoxAlgorithmMultipleSpatialFilters::initialize(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	m_pStreamDecoder=NULL;
	m_pStreamEncoder=NULL;

	boolean l_bValid=false;
	CIdentifier l_oIdentifier;
	l_rStaticBoxContext.getInputType(0, l_oIdentifier);

	if(l_oIdentifier==OV_TypeId_StreamedMatrix)
	{
		l_bValid=true;

		m_pStreamDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixStreamDecoder));
		m_pStreamEncoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixStreamEncoder));

		m_pStreamDecoder->initialize();
		m_pStreamEncoder->initialize();
	}

	if(l_oIdentifier==OV_TypeId_Signal)
	{
		l_bValid=true;

		m_pStreamDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamDecoder));
		m_pStreamEncoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamEncoder));

		m_pStreamDecoder->initialize();
		m_pStreamEncoder->initialize();

		TParameterHandler < uint64 > op_pSamplingFrequency(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_SamplingRate));
		TParameterHandler < uint64 > ip_pSamplingFrequency(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_SignalStreamEncoder_InputParameterId_SamplingRate));
		ip_pSamplingFrequency.setReferenceTarget(op_pSamplingFrequency);
	}

	if(l_oIdentifier==OV_TypeId_Spectrum)
	{
		l_bValid=true;

		m_pStreamDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumStreamDecoder));
		m_pStreamEncoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumStreamEncoder));

		m_pStreamDecoder->initialize();
		m_pStreamEncoder->initialize();

		TParameterHandler < IMatrix* > op_pBandMatrix(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_SpectrumStreamDecoder_OutputParameterId_MinMaxFrequencyBands));
		TParameterHandler < IMatrix* > ip_pBandMatrix(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_SpectrumStreamEncoder_InputParameterId_MinMaxFrequencyBands));
		ip_pBandMatrix.setReferenceTarget(op_pBandMatrix);
	}

	if(!l_bValid)
	{
		this->getLogManager() << LogLevel_Error << "Unhandled input stream type " << l_oIdentifier << "\n";
		return false;
	}

	ip_pMemoryBuffer.initialize(m_pStreamDecoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_InputParameterId_MemoryBufferToDecode));
	op_pMatrix.initialize(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_OutputParameterId_Matrix));

	ip_pMatrix.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixStreamEncoder_InputParameterId_Matrix));
	op_pMemoryBuffer.initialize(m_pStreamEncoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixStreamEncoder_OutputParameterId_EncodedMemoryBuffer));

	//m_oStimulationDecoder.initialize(*this);
	//m_oStreamedMatrixDecoder.initialize(*this);
	
	m_ui32OutputChannelCount = 0;
	m_ui32InputChannelCount = 0;
	m_ui32NumberOfFilters = 0;
	
	CString l_sConfigurationFilename;
	l_rStaticBoxContext.getSettingValue(0, l_sConfigurationFilename);
	std::ifstream l_oFile(l_sConfigurationFilename.toASCIIString(), std::ios::binary);
	
	if(l_oFile.is_open())
	{
		size_t l_iFileLen;
		l_oFile.seekg(0, std::ios::end);
		l_iFileLen=l_oFile.tellg();
		l_oFile.seekg(0, std::ios::beg);
		char* l_pConfigurationFile = new char[l_iFileLen];
		l_oFile.read(l_pConfigurationFile, l_iFileLen);
		l_oFile.close();
		
		XML::IReader* l_pReader=XML::createReader(*this);
		//std::cout << l_pConfigurationFile << "\n";
		l_pReader->processData(l_pConfigurationFile, l_iFileLen);
		l_pReader->release();
		l_pReader=NULL;			
	}
	else
	{
		this->getLogManager() << LogLevel_Warning << "Init : Could not load configuration from file [" << l_sConfigurationFilename << "]\n";
	}	
	

	
	return true;
}

boolean CBoxAlgorithmMultipleSpatialFilters::uninitialize(void)
{
	if(m_pStreamDecoder)
	{
		m_pStreamDecoder->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pStreamDecoder);
		m_pStreamDecoder=NULL;
	}

	if(m_pStreamEncoder)
	{
		m_pStreamEncoder->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pStreamEncoder);
		m_pStreamEncoder=NULL;
	}
	
	//m_oStimulationDecoder.uninitialize();
	//m_oStreamedMatrixDecoder.uninitialize();

	return true;
}

boolean CBoxAlgorithmMultipleSpatialFilters::processInput(uint32 ui32InputIndex)
{
	this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CBoxAlgorithmMultipleSpatialFilters::process(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	uint32 i, j, k, l;		

	for(i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		ip_pMemoryBuffer=l_rDynamicBoxContext.getInputChunk(0, i);
		op_pMemoryBuffer=l_rDynamicBoxContext.getOutputChunk(0);

		m_pStreamDecoder->process();
		CString l_sSettingOverrideFilename = l_rStaticBoxContext.getAttributeValue(OV_AttributeId_Box_SettingOverrideFilename);
		//this->getLogManager() << LogLevel_Info << "Override filename " << l_sSettingOverrideFilename << "\n";
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			//CString l_sCoefficient=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
			
			//loadCoefficients(l_sCoefficient, ' ', OV_Value_EnumeratedStringSeparator);

			//uint32 l_ui32OutputChannelCountSetting=(uint32)(uint64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
			//uint32 l_ui32InputChannelCountSetting=(uint32)(uint64)FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

			uint32 l_ui32OutputChannelCount=m_vCoefficients[0].size() / op_pMatrix->getDimensionSize(0);
			uint32 l_ui32InputChannelCount=op_pMatrix->getDimensionSize(0);

			if(l_ui32InputChannelCount!=m_ui32InputChannelCount)
			{
				this->getLogManager() << LogLevel_Error  << "Bad matrix size - Expected " << m_ui32InputChannelCount << " input channels and received " << l_ui32InputChannelCount << " input channels\n";
				return false;
			}

			if(m_vCoefficients[0].size() != m_ui32OutputChannelCount*m_ui32InputChannelCount)
			{
				this->getLogManager() << LogLevel_Error << "Bad matrix size - The number of coefficients of the filter (" << uint32(m_vCoefficients[0].size()) << ") does not match the input/output channel count settings (" << m_ui32InputChannelCount << "x" << m_ui32OutputChannelCount << ")\n";
				return false;
			}

			ip_pMatrix->setDimensionCount(2);
			ip_pMatrix->setDimensionSize(0, l_ui32OutputChannelCount);
			ip_pMatrix->setDimensionSize(1, op_pMatrix->getDimensionSize(1));

			m_pStreamEncoder->process(OVP_GD_Algorithm_StreamedMatrixStreamEncoder_InputTriggerId_EncodeHeader);
			l_rDynamicBoxContext.markInputAsDeprecated(0, i);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_OutputTriggerId_ReceivedBuffer))
		{
			float64* l_pMatrix=op_pMatrix->getBuffer();
			float64* l_pFilteredMatrix=ip_pMatrix->getBuffer();
			uint32 l_ui32InputChannelCount=op_pMatrix->getDimensionSize(0);
			uint32 l_ui32OutputChannelCount=ip_pMatrix->getDimensionSize(0);
			uint32 l_ui32SampleCount=ip_pMatrix->getDimensionSize(1);

			System::Memory::set(l_pFilteredMatrix, l_ui32SampleCount*l_ui32OutputChannelCount*sizeof(float64), 0);

			itpp::Mat<float64> l_oSignal(l_pMatrix, l_ui32InputChannelCount, l_ui32SampleCount, true);
			/*std::cout << "signal\n";
			for(k=0; k<l_ui32InputChannelCount; k++)
			{
				for(l=0; l<l_ui32SampleCount; l++)
				{
					//std::cout << *(l_pMatrix+k*l_ui32SampleCount+l) << " ";
					std::cout << l_oSignal.get(k,l) << " ";
				}
				std::cout << "\n";
			}*/
			
			for(j=0; j<m_ui32NumberOfFilters; j++)
			{
				//this->getLogManager() << LogLevel_Info << "spatial filter dimensions " << (uint32)m_vCoefficients[j].rows() << "," << (uint32)m_vCoefficients[j].cols() << "\n";
				//this->getLogManager() << LogLevel_Info << "signal dimensions " << (uint32)l_oSignal.rows() << "," << (uint32)l_oSignal.cols() << "\n";
				itpp::Mat<float64> l_oFilteredMatrix = m_vCoefficients[j]*l_oSignal;
				//std::cout << "Spatially filtered signal\n";
				for(uint32 k=0; k<l_ui32OutputChannelCount; k++)
				{
					for(uint32 l=0; l<l_ui32SampleCount; l++)
					{
						l_pFilteredMatrix[k*l_ui32SampleCount+l]=l_oFilteredMatrix.get(k,l);
						//std::cout << l_pFilteredMatrix[k*l_ui32SampleCount+l] << " ";
					}
					//std::cout << "\n";
				}
				//std::cout << "\n";
				
				m_pStreamEncoder->process(OVP_GD_Algorithm_StreamedMatrixStreamEncoder_InputTriggerId_EncodeBuffer);
				l_rDynamicBoxContext.markInputAsDeprecated(0, i);
				l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
			}	
		}
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixStreamDecoder_OutputTriggerId_ReceivedEnd))
		{
			m_pStreamEncoder->process(OVP_GD_Algorithm_StreamedMatrixStreamEncoder_InputTriggerId_EncodeEnd);
			l_rDynamicBoxContext.markInputAsDeprecated(0, i);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
	}
	
	/*for(i=0; i<l_rDynamicBoxContext.getInputChunkCount(1); i++)
	{
		m_oStimulationDecoder.decode(1,i);
		IStimulationSet* l_pStimSet = m_oStimulationDecoder.getOutputStimulationSet();
		for (j=0; j<l_pStimSet->getStimulationCount(); j++)
		{
			if (l_pStimSet->getStimulationIdentifier(j)==m_ui64Trigger)
			{
				if(l_rStaticBoxContext.initializeFromAlgorithmClassIdentifier(OVP_ClassId_BoxAlgorithm_SpatialFilterWithUpdate))
				{
					this->getLogManager() << LogLevel_Info << "Resetting box\n";
					CString l_sCoefficient=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
					this->getLogManager() << LogLevel_Info << "Box reset, reloaded filter coefficients " << l_sCoefficient << "\n";
				}
			}
		}
	}*/	

	return true;
}

void CBoxAlgorithmMultipleSpatialFilters::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{
	m_vNode.push(sName);
}

void CBoxAlgorithmMultipleSpatialFilters::processChildData(const char* sData)
{
	std::stringstream l_sData(sData);

	if(m_vNode.top()==CString("InputChannels"))
	{
		uint32 l_ui32InputChannelCount;
		l_sData >> l_ui32InputChannelCount;
		if (m_ui32InputChannelCount!=0)
		{
			if (m_ui32InputChannelCount!=l_ui32InputChannelCount)
				this->getLogManager() << LogLevel_Error  <<  "The number of input channels should be the same for all spatial filters\n";
		}
		else
			m_ui32InputChannelCount = l_ui32InputChannelCount;
		
		//std::cout << "Input Channel Count " << m_ui32InputChannelCount << "\n";
	}
	
	if(m_vNode.top()==CString("OutputChannels"))
	{
		uint32 l_ui32OutputChannelCount;
		l_sData >> l_ui32OutputChannelCount;
		if (m_ui32OutputChannelCount!=0)
		{
			if (m_ui32OutputChannelCount!=l_ui32OutputChannelCount)
				this->getLogManager() << LogLevel_Error  <<  "The number of output channels should be the same for all spatial filters\n";
		}
		else
			m_ui32OutputChannelCount = l_ui32OutputChannelCount;
		
		//std::cout << "OUtput Channel Count " << m_ui32OutputChannelCount << "\n";
	}	

	if(m_vNode.top()==CString("Coefficients"))
	{
		//std::cout << "Coefficients\n";
		//std::vector < float64 > l_vCoefficients;
		float64* l_vCoefficients = new float64[m_ui32OutputChannelCount*m_ui32InputChannelCount];
		int i=0;
		while(!l_sData.eof())
		{
			float64 l_f64Value;
			l_sData >> l_f64Value;
			//l_vCoefficients.push_back(l_f64Value);
			l_vCoefficients[i++] = l_f64Value;
			//std::cout << " " << l_vCoefficients[i-1];
		}
		//std::cout << "--------------\n" <<  i << ", " << m_ui32OutputChannelCount*m_ui32InputChannelCount <<"\n";

		m_vCoefficients.push_back(itpp::Mat<float64>(l_vCoefficients,m_ui32OutputChannelCount,m_ui32InputChannelCount,true));
		//std::cout << "spatial filter\n";
		for(uint32 k=0; k<m_ui32OutputChannelCount; k++)
		{
			for(uint32 l=0; l<m_ui32InputChannelCount; l++)
			{
				//std::cout << *(l_pMatrix+k*l_ui32SampleCount+l) << " ";
				//std::cout << m_vCoefficients.back().get(k,l) << " ";
			}
			//std::cout << "\n";
		}		
		//m_vCoefficients[m_ui32NumberOfExperts].set_size(l_vCoefficients.size());
		/*for(size_t i=0; i<l_vCoefficients.size(); i++)
		{
			m_vCoefficients[m_ui32NumberOfExperts].set(l_vCoefficients[i]);
		}*/
		//std::cout << "test \n";
		//std::cout << "test " << l_vCoefficients[0] << "," << l_vCoefficients[m_ui32OutputChannelCount*m_ui32InputChannelCount-1] << "\n";
		delete[] l_vCoefficients;
	}
}

void CBoxAlgorithmMultipleSpatialFilters::closeChild(void)
{
	if(m_vNode.top()==CString("SpatialFilter"))
	{
		m_ui32NumberOfFilters++;
		//this->getLogManager() << LogLevel_Info << "Number of experts  " << m_ui32NumberOfExperts << "\n";
	}
	m_vNode.pop();
}

#endif
