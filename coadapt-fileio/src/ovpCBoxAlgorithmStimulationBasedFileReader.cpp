#include "ovpCBoxAlgorithmStimulationBasedFileReader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <locale> 
#include <system/Time.h>

using namespace std;

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileReadingAndWriting;

struct csvReader: ctype<char> {
    csvReader(): ctype<char>(get_table()) {}
    static ctype_base::mask const* get_table() {
        static vector<ctype_base::mask> rc(table_size, ctype_base::mask());
 
        rc[' '] = ctype_base::space;
        rc[';'] = ctype_base::space;
	  rc[','] = ctype_base::space;
	  rc['\t'] = ctype_base::space;
        rc['\n'] = ctype_base::space;
        return &rc[0];
    }
};

boolean CBoxAlgorithmStimulationBasedFileReader::initialize(void)
{
	m_oAlgo0_StimulationDecoder.initialize(*this);
	m_sFileName = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_ui64Trigger = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_bIncremental = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	//IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	//l_rStaticBoxContext.getOutputType(0, m_oTypeIdentifier);
	//if (m_oTypeIdentifier==OVTK_TypeId_StreamedMatrix)
		m_oAlgo1_StreamedMatrixEncoder.initialize(*this);
	//else if (m_oTypeIdentifier==OVTK_TypeId_FeatureVector)
	//	m_oAlgo2_FeatureVectorEncoder.initialize(*this);
	m_ui32FilePosition = 0;
	m_ui32LineNumber = 0;
	m_ui32ColumnNumber = 0;
	m_bFirstStimulusReceived = false;
	
	timingFile = fopen(OpenViBE::Directories::getDataDir() + "/group_stim_seek_timing.txt","w");	

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmStimulationBasedFileReader::uninitialize(void)
{
	m_oAlgo0_StimulationDecoder.uninitialize();
	//if(m_bFeatureVectorOutput)
	//	m_oAlgo2_FeatureVectorEncoder.uninitialize();
	//else
		m_oAlgo1_StreamedMatrixEncoder.uninitialize();
		
	fclose(timingFile);

	return true;
}

boolean CBoxAlgorithmStimulationBasedFileReader::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmStimulationBasedFileReader::process(void)
{
	
	// the static box context describes the box inputs, outputs, settings structures
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//iterate over all chunk on input 0
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		// decode the chunk i on input 0
		m_oAlgo0_StimulationDecoder.decode(0,i);
		// the decoder may have decoded 3 different parts : the header, a buffer or the end of stream.
		if(m_oAlgo0_StimulationDecoder.isHeaderReceived())
		{
		}
		if(m_oAlgo0_StimulationDecoder.isBufferReceived())
		{			
			// Buffer received. For example the signal values
			// Access to the buffer can be done thanks to :
			IStimulationSet* l_pStimulationSet = m_oAlgo0_StimulationDecoder.getOutputStimulationSet(); // the StreamedMatrix of samples.
			if(l_pStimulationSet->getStimulationCount()>0 && m_ui64Trigger==l_pStimulationSet->getStimulationIdentifier(0))
			{
				//std::cout << "Stim received " << l_pStimulationSet->getStimulationIdentifier(0) << "\n";
				CIdentifier l_oTypeIdentifier;
				l_rStaticBoxContext.getOutputType(0, l_oTypeIdentifier);
				
				ifstream l_FileHandle;
				l_FileHandle.open(m_sFileName.toASCIIString(), ifstream::in);
				
				float64 l_f64TimeBefore = float64((System::Time::zgetTime()>>22)/1024.0);
				fprintf(timingFile, "%f \n",l_f64TimeBefore);
				l_FileHandle.seekg(m_ui32FilePosition);
				float64 l_f64TimeAfter = float64((System::Time::zgetTime()>>22)/1024.0);
				fprintf(timingFile, "%f \n",l_f64TimeAfter);						
				
				char l_Line[32768];
				l_FileHandle.getline(l_Line,32768);
				std::istringstream l_LineBuffer(l_Line);
				l_LineBuffer.imbue(std::locale(std::locale(), new csvReader()));	
				
				IMatrix* l_pOutputMatrix = m_oAlgo1_StreamedMatrixEncoder.getInputMatrix();
				l_pOutputMatrix->setDimensionCount(2);				
				
				if(!m_bFirstStimulusReceived)
				{					
					m_ui32ColumnNumber = 0;
					float64 l_f64Value;
					while(l_LineBuffer>>l_f64Value)
						m_ui32ColumnNumber++;
					//init for entering the next if when first stimulus is received
					l_LineBuffer.clear();
					l_LineBuffer.seekg(0);	
					
					m_ui32LineNumber = 0;
					while (l_FileHandle.gcount()>2)	
					{
						l_FileHandle.getline(l_Line,32768);
						m_ui32LineNumber++;
					}	
					//init for entering the next if when first stimulus is received
					l_FileHandle.clear();		
					l_FileHandle.seekg(0);
					l_FileHandle.getline(l_Line,32768);
				
					//std::cout << "Line number " << m_ui32LineNumber << ", col number " << m_ui32ColumnNumber << "\n";
				
					m_bFirstStimulusReceived = true;	
					
					l_pOutputMatrix->setDimensionSize(0, m_ui32ColumnNumber);
					if (l_oTypeIdentifier==OVTK_TypeId_FeatureVector)
						l_pOutputMatrix->setDimensionSize(1, 1);
					else
						l_pOutputMatrix->setDimensionSize(1, m_ui32LineNumber);
					m_oAlgo1_StreamedMatrixEncoder.encodeHeader(0);
					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));					
				}
				if(m_bFirstStimulusReceived)
				{		
					l_pOutputMatrix->setDimensionSize(0, m_ui32ColumnNumber);
					if (l_oTypeIdentifier==OVTK_TypeId_FeatureVector)
						l_pOutputMatrix->setDimensionSize(1, 1);
					else
						l_pOutputMatrix->setDimensionSize(1, m_ui32LineNumber);						
					
					float64* l_f64MatrixBuffer = l_pOutputMatrix->getBuffer();

					uint32 l_ui32LineCounter = 0, l_ui32ColumnCounter = 0;
					float64 l_f64Value;
					while (l_FileHandle.gcount()>2)
					{
						l_ui32ColumnCounter = 0;
						if (l_oTypeIdentifier==OVTK_TypeId_FeatureVector)
						{
							while(l_LineBuffer>>l_f64Value)
								*(l_f64MatrixBuffer+l_ui32ColumnCounter++) = l_f64Value;
							m_oAlgo1_StreamedMatrixEncoder.encodeBuffer(0);
							l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));											
						}
						else
							while(l_LineBuffer>>l_f64Value)
								*(l_f64MatrixBuffer+(m_ui32LineNumber*l_ui32ColumnCounter++)+l_ui32LineCounter) = l_f64Value;
						//std::cout << l_Line << "\n";
						
						l_FileHandle.getline(l_Line,32768);
						
						l_LineBuffer.clear();
						l_LineBuffer.str(string(l_Line));
						l_LineBuffer.seekg(0);		
						
						l_ui32LineCounter++;
					}	
				}
				if(m_bIncremental)
				{
					m_ui32FilePosition = l_FileHandle.tellg();
					//std::cout << "Incremental pos " << m_ui32FilePosition << ", char " << l_FileHandle.get() <<"\n";
					//l_FileHandle.seekg(-1,ios_base::cur);
				}				
				
				l_FileHandle.close();
				
				if (l_oTypeIdentifier!=OVTK_TypeId_FeatureVector)
				{
					m_oAlgo1_StreamedMatrixEncoder.encodeBuffer(0);
					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));				
				}
			}
		}
		if(m_oAlgo0_StimulationDecoder.isEndReceived())
		{
			CIdentifier l_oTypeIdentifier;
			l_rStaticBoxContext.getOutputType(0, l_oTypeIdentifier);
			
			IMatrix* l_pOutputMatrix = m_oAlgo1_StreamedMatrixEncoder.getInputMatrix();
			l_pOutputMatrix->setDimensionCount(2);
			l_pOutputMatrix->setDimensionSize(0, m_ui32ColumnNumber);
			if (l_oTypeIdentifier==OVTK_TypeId_FeatureVector)
				l_pOutputMatrix->setDimensionSize(1, 1);
			else
				l_pOutputMatrix->setDimensionSize(1, m_ui32LineNumber);		
			m_oAlgo1_StreamedMatrixEncoder.encodeEnd(0);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

	}
	
	return true;
}
