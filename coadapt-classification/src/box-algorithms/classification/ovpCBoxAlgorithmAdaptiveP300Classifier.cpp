#include "ovpCBoxAlgorithmAdaptiveP300Classifier.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

#include <system/Memory.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;
using namespace std;

boolean CBoxAlgorithmAdaptiveP300Classifier::initialize(void)
{
	// Feature vector stream decoder
	m_oAlgo0_FeatureVectorDecoder.initialize(*this);
	m_oAlgo1_FeatureVectorDecoder.initialize(*this);
	// Stimulation stream decoder
	m_oAlgo1_StimulationDecoder.initialize(*this);
	// Streamed matrix stream encoder
	m_oAlgo2_StreamedMatrixEncoder.initialize(*this);
	
	m_pClassifier=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ClassifierPLDA));
	m_pClassifier->initialize();
	
	m_oAlgo2_StreamedMatrixEncoder.getInputMatrix().setReferenceTarget(m_pClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
	
	// If you need to, you can manually set the reference targets to link the codecs input and output. To do so, you can use :
	//m_oEncoder.getInputX().setReferenceTarget(m_oDecoder.getOutputX())
	// Where 'X' depends on the codec type. Please refer to the Codec Toolkit Reference Page
	// (http://openvibe.inria.fr/documentation/unstable/Doc_Tutorial_Developer_SignalProcessing_CodecToolkit_Ref.html) for a complete list.
	
	// If you need to retrieve setting values, use the FSettingValueAutoCast function.
	// For example :
	// - CString setting at index 0 in the setting list :
	// CString l_sSettingValue = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	// - unsigned int64 setting at index 1 in the setting list :
	// uint64 l_ui64SettingValue = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	// - float64 setting at index 2 in the setting list :
	// float64 l_f64SettingValue = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	// ...
	
	//m_bPredictionReceived = false;
	//m_ui64StimulationIdentifier = 0;
	
	TParameterHandler < int64 > ip_i64ShrinkageType(m_pClassifier->getInputParameter(OVP_Algorithm_ClassifierPLDA_InputParameterId_Shrinkage));
	ip_i64ShrinkageType = 2;
	TParameterHandler < float64 > l_f64Lambda(m_pClassifier->getInputParameter(OVP_Algorithm_ClassifierPLDA_InputParameterId_Lambda));
	l_f64Lambda = 0.5;
	
	m_bFlashGroupsReceived = false;
	m_bFeedbackReceived = false;
	m_bTargetReceived = false;
	m_bFeedbackBasedLearning = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_ui64FeedbackOnsetIdentifier = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	m_ui64TargetOnsetIdentifier = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
	m_ui64LetterIndex = 0;
	m_ui64SaveConfigurationTriggerIdentifier = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
	
	CString l_sConfigurationFilename;
	this->getStaticBoxContext().getSettingValue(0, l_sConfigurationFilename);
	
	TParameterHandler < IMemoryBuffer* > ip_pClassificationConfiguration(m_pClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_Configuration));
	IMemoryBuffer* l_pConfigurationFile = ip_pClassificationConfiguration;
	ifstream l_oFile(l_sConfigurationFilename.toASCIIString(), ios::binary);
	
	if(l_oFile.is_open())
	{
		size_t l_iFileLen;
		l_oFile.seekg(0, ios::end);
		l_iFileLen=l_oFile.tellg();
		l_oFile.seekg(0, ios::beg);
		l_pConfigurationFile->setSize(l_iFileLen, true);
		l_oFile.read((char*)l_pConfigurationFile->getDirectPointer(), l_iFileLen);
		l_oFile.close();
		//m_bIsConfigurationFile = true;
		std::cout << "trigger load configuration\n";
		m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_LoadConfiguration);
	}
	else
	{
		this->getLogManager() << LogLevel_Warning << "Could not load configuration from file [" << l_sConfigurationFilename << "]\n";
		//m_bIsConfigurationFile = false;
	}

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmAdaptiveP300Classifier::uninitialize(void)
{
	m_oAlgo0_FeatureVectorDecoder.uninitialize();
	m_oAlgo1_StimulationDecoder.uninitialize();
	m_oAlgo2_StreamedMatrixEncoder.uninitialize();
	m_oAlgo1_FeatureVectorDecoder.uninitialize();

	m_pClassifier->uninitialize();
	this->getAlgorithmManager().releaseAlgorithm(*m_pClassifier);
	
	return true;
}
/*******************************************************************************/

/*
boolean CBoxAlgorithmAdaptiveP300Classifier::processClock(IMessageClock& rMessageClock)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

/*
uint64 CBoxAlgorithmAdaptiveP300Classifier::getClockFrequency(void)
{
	// Note that the time is coded on a 64 bits unsigned integer, fixed decimal point (32:32)
	return 0LL<<32; // the box clock frequency
}
/*******************************************************************************/


boolean CBoxAlgorithmAdaptiveP300Classifier::processInput(uint32 ui32InputIndex)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmAdaptiveP300Classifier::process(void)
{
	
	// the static box context describes the box inputs, outputs, settings structures
	//IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	// the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	// here is some useful functions:
	// - To get input/output/setting count:
	// l_rStaticBoxContext.getInputCount();
	// l_rStaticBoxContext.getOutputCount();
	
	// - To get the number of chunks currently available on a particular input :
	// l_rDynamicBoxContext.getInputChunkCount(input_index)
	
	// - To send an output chunk :
	// l_rDynamicBoxContext.markOutputAsReadyToSend(output_index, chunk_start_time, chunk_end_time);
	
	
	// A typical process iteration may look like this.
	// This example only iterate over the first input of type Signal, and output a modified Signal.
	// thus, the box uses 1 decoder (m_oSignalDecoder) and 1 encoder (m_oSignalEncoder)
	
	//IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	//iterate over all chunk on input 0
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		// decode the chunk i on input 0
		m_oAlgo0_FeatureVectorDecoder.decode(0,i);
		// the decoder may have decoded 3 different parts : the header, a buffer or the end of stream.
		if(m_oAlgo0_FeatureVectorDecoder.isHeaderReceived())
		{
			// Header received. This happens only once when pressing "play". For example with a StreamedMatrix input, you now know the dimension count, sizes, and labels of the matrix
			// ... maybe do some process ...
			/*IMatrix* l_pMatrix = m_oAlgo0_FeatureVectorDecoder.getOutputMatrix();
			uint32 l_ui32Dimension = l_pMatrix->getDimensionSize(0)*l_pMatrix->getDimensionSize(1);
			m_vCircularSampleBuffer = vector<itpp::vec>(CIRCULAR_BUFFER_SIZE, itpp::vec(l_ui32Dimension));
			m_vCircularLabelBuffer = vector<uint64>(CIRCULAR_BUFFER_SIZE, 0);
			m_ui32BufferPointer = 0;*/
			IMatrix * l_f64ProbabilityHeader = m_oAlgo2_StreamedMatrixEncoder.getInputMatrix();
			l_f64ProbabilityHeader->setDimensionCount(2);
			l_f64ProbabilityHeader->setDimensionSize(0, 1);
			l_f64ProbabilityHeader->setDimensionSize(1, 1);
			*l_f64ProbabilityHeader->getBuffer() = 0.0;
			// Pass the header to the next boxes, by encoding a header on the output 0:
			m_oAlgo2_StreamedMatrixEncoder.encodeHeader(0);
			// send the output chunk containing the header. The dates are the same as the input chunk:
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}
		if(m_oAlgo0_FeatureVectorDecoder.isBufferReceived())
		{
			// Buffer received. For example the signal values
			// Access to the buffer can be done thanks to :
			IMatrix* l_pMatrix = m_oAlgo0_FeatureVectorDecoder.getOutputMatrix(); // the StreamedMatrix of samples.
			//std::cout << "matrix dimension " << l_pMatrix->getDimensionSize(0) << "\n";
			//for (uint32 ii=0; ii<l_pMatrix->getDimensionSize(0);ii++)
			//	std::cout << " " << *(l_pMatrix->getBuffer()+ii) << "\n";
			//m_pClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVector)->
			//	setReferenceTarget(m_oAlgo0_FeatureVectorDecoder.getOutputMatrix());
			
			IMatrix* l_pFeatureVectorMatrix=new CMatrix();
			OpenViBEToolkit::Tools::Matrix::copy(*l_pFeatureVectorMatrix, *l_pMatrix);
			m_vSampleVector.push_back(l_pFeatureVectorMatrix);	
			
			TParameterHandler < IMatrix* > tmp = m_pClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVector);
			OpenViBEToolkit::Tools::Matrix::copy(*tmp, *l_pMatrix);
			//TParameterHandler < IMatrix* > l_f64Probability(m_pClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));	
			//m_oAlgo2_StreamedMatrixEncoder.getInputMatrix().setReferenceTarget(l_f64Probability);
			if (m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Classify))
			{
				if (m_pClassifier->isOutputTriggerActive(OVTK_Algorithm_Classifier_OutputTriggerId_Success))
				{
					//getLogManager() << LogLevel_Info << "Classifier output " << *l_f64Probability->getBuffer() << "\n";
					/*itpp::vec l_oFeatures(l_pMatrix->getBuffer(), l_pMatrix->getBufferElementCount());
					float64 l_f64Probability;
					float64 l_f64Class;
					classify(l_oFeatures, l_f64Class, l_f64Probability);*/
					
					/*if (m_ui32BufferPointer==CIRCULAR_BUFFER_SIZE)
						m_ui32BufferPointer = 0;
					m_vCircularSampleBuffer[m_ui32BufferPointer] = l_oFeatures;
					m_vCircularLabelBuffer[m_ui32BufferPointer] = l_f64Class;
					m_ui32BufferPointer++;*/		

					// Encode the output buffer :
					//IMatrix* l_pOutputProbability = m_oAlgo2_StreamedMatrixEncoder.getInputMatrix();
					//OpenViBEToolkit::Tools::Matrix::copy(*l_pOutputProbability, *l_f64Probability);
					m_oAlgo2_StreamedMatrixEncoder.encodeBuffer(0);
					// and send it to the next boxes :
					l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));					
				}
			}
			
		}
		if(m_oAlgo0_FeatureVectorDecoder.isEndReceived())
		{
			// End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
			m_oAlgo2_StreamedMatrixEncoder.encodeEnd(0);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

		// The current input chunk has been processed, and automaticcaly discarded.
		// you don't need to call "l_rDynamicBoxContext.markInputAsDeprecated(0, i);"
	}
	
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(1); i++)
	{
		m_oAlgo1_FeatureVectorDecoder.decode(1,i);
		if(m_oAlgo1_FeatureVectorDecoder.isHeaderReceived())
		{
			
		}
		if(m_oAlgo1_FeatureVectorDecoder.isBufferReceived())
		{
			IMatrix* l_pFlashGroup = new CMatrix();
			OpenViBEToolkit::Tools::Matrix::copy(*l_pFlashGroup,*m_oAlgo1_FeatureVectorDecoder.getOutputMatrix());
			/*std::cout << "flashgroup matrix dimension " << l_pFlashGroup->getDimensionSize(0) << "\n";
			for (uint32 ii=0; ii<l_pFlashGroup->getDimensionSize(0);ii++)
				std::cout << " " << *(l_pFlashGroup->getBuffer()+ii) << "\n";*/			
			m_vFlashGroups.push_back(l_pFlashGroup);	
			m_bFlashGroupsReceived = true;
		}
		if(m_oAlgo1_FeatureVectorDecoder.isEndReceived())
		{
			
		}
	}	
	
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(2); i++)
	{
		m_oAlgo1_StimulationDecoder.decode(2,i);
		if(m_oAlgo1_StimulationDecoder.isHeaderReceived())
		{
			
		}
		if(m_oAlgo1_StimulationDecoder.isBufferReceived())
		{
			IStimulationSet* l_pStimulationSet = m_oAlgo1_StimulationDecoder.getOutputStimulationSet();
			uint64 l_ui64StimulationIdentifier = 0;
			
			for (uint32 ii=0; ii<l_pStimulationSet->getStimulationCount(); ii++)
			{
				l_ui64StimulationIdentifier = l_pStimulationSet->getStimulationIdentifier(ii);
				
				if (l_ui64StimulationIdentifier==m_ui64FeedbackOnsetIdentifier)
				{
					m_bFeedbackReceived = true;
					m_bTargetReceived = false;
					getLogManager() << LogLevel_Info << "Feedback cue received\n";
				}
				else if (l_ui64StimulationIdentifier==m_ui64TargetOnsetIdentifier)
				{
					m_bTargetReceived = true;
					m_bFeedbackReceived = false;
					getLogManager() << LogLevel_Info << "Target cue received\n";
				}
				else if (l_ui64StimulationIdentifier>0 && l_ui64StimulationIdentifier<180 && 
					((m_bFeedbackReceived && m_bFeedbackBasedLearning) || (m_bTargetReceived && !m_bFeedbackBasedLearning)))
				{
					m_ui64LetterIndex = l_ui64StimulationIdentifier;
					if (m_bTargetReceived)
						getLogManager() << LogLevel_Info << "Target train label " << l_ui64StimulationIdentifier << "\n";
					if(m_bFeedbackReceived)
						getLogManager() << LogLevel_Info << "Feedback train label " << l_ui64StimulationIdentifier << "\n";
				}	
				
				if (l_ui64StimulationIdentifier==m_ui64SaveConfigurationTriggerIdentifier)
				{
					CMemoryBuffer l_oConfiguration;
					TParameterHandler < IMemoryBuffer* > op_pConfiguration(m_pClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
					op_pConfiguration=&l_oConfiguration;
					
					m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_SaveConfiguration);
					
					CString l_sConfigurationFilename(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));
					std::ofstream l_oFile(l_sConfigurationFilename.toASCIIString(), ios::binary);
					if(l_oFile.is_open())
					{
						this->getLogManager() << LogLevel_Info << "Saving configuration to file [" << l_sConfigurationFilename << "]\n";
						l_oFile.write((char*)l_oConfiguration.getDirectPointer(), (std::streamsize)l_oConfiguration.getSize());
						l_oFile.close();
					}
					else
					{
						this->getLogManager() << LogLevel_Error << "Could not save configuration to file [" << l_sConfigurationFilename << "]\n";
						return false;
					}					
				}
			}
		}
		if(m_oAlgo1_StimulationDecoder.isEndReceived())
		{
			
		}
	}
	
	if (m_ui64LetterIndex!=0 && m_bFeedbackReceived && m_bFlashGroupsReceived)
	{
		getLogManager() << LogLevel_Info << "Start training... \n";
		train();
		m_bFlashGroupsReceived = false;
		m_bFeedbackReceived = false;
		m_bTargetReceived = false;
		m_ui64LetterIndex = 0;
	}

	// check the official developer documentation webpage for more example and information :
	
	// Tutorials:
	// http://openvibe.inria.fr/documentation/#Developer+Documentation
	// Codec Toolkit page :
	// http://openvibe.inria.fr/codec-toolkit-references/
	
	// Feel free to ask experienced developers on the forum (http://openvibe.inria.fr/forum) and IRC (#openvibe on irc.freenode.net).

	return true;
}

void CBoxAlgorithmAdaptiveP300Classifier::train()
{
	uint32 l_ui32FeatureVectorCount=m_vSampleVector.size();
	uint32 l_ui32FeatureVectorSize=m_vSampleVector[0]->getBufferElementCount();

	TParameterHandler < IMatrix* > ip_pFeatureVectorSet(m_pClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet));
	ip_pFeatureVectorSet->setDimensionCount(2);
	ip_pFeatureVectorSet->setDimensionSize(0, l_ui32FeatureVectorCount);
	ip_pFeatureVectorSet->setDimensionSize(1, l_ui32FeatureVectorSize+1);

	float64* l_pFeatureVectorSetBuffer=ip_pFeatureVectorSet->getBuffer();
	for(size_t j=0; j<m_vSampleVector.size(); j++)
	{
		System::Memory::copy(l_pFeatureVectorSetBuffer, m_vSampleVector[j]->getBuffer(), l_ui32FeatureVectorSize*sizeof(float64));

		float64 l_f64Class;
		if (*(m_vFlashGroups[j]->getBuffer()+m_ui64LetterIndex-1) == 1)
			l_f64Class = 1;
		else
			l_f64Class = 2;
			
		l_pFeatureVectorSetBuffer[l_ui32FeatureVectorSize]=l_f64Class;
		l_pFeatureVectorSetBuffer+=(l_ui32FeatureVectorSize+1);
	}

	for(size_t j=0; j<m_vSampleVector.size(); j++)
		delete m_vSampleVector[j];
	for(size_t j=0; j<m_vFlashGroups.size(); j++)	
		delete m_vFlashGroups[j];

	m_pClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Train);	

	m_vSampleVector.clear();
	m_vFlashGroups.clear();
}

/*boolean CBoxAlgorithmAdaptiveP300Classifier::classify(const itpp::vec& l_oFeatures, float64& rf64Class, float64& rProbability)
{
	if(l_oFeatures.length()+1!=(unsigned int)m_oCoefficientsClass1.size())
	{
		this->getLogManager() << LogLevel_Warning << "Feature vector size " << l_oFeatures.length() << " and hyperplane parameter size " << (uint32) m_oCoefficientsClass1.size() << " does not match\n";
		return false;
	}

	l_oFeatures.ins(0, 1);

	rProbability=std::exp(l_oFeatures*m_oCoefficientsClass1)
					/(std::exp(l_oFeatures*m_oCoefficientsClass1)+std::exp(l_oFeatures*m_oCoefficientsClass2));
					
	this->getLogManager() << LogLevel_Debug << "p(Class1|x)=" << rProbability << "\n";

	if(rProbability >= 0.5)
	{
		rf64Class=m_f64Class1;
	}
	else
	{
		rf64Class=m_f64Class2;
	}

	return true;
}

boolean CBoxAlgorithmAdaptiveP300Classifier::saveConfiguration(IMemoryBuffer& rMemoryBuffer)
{
	rMemoryBuffer.setSize(0, true);
	rMemoryBuffer.append(m_oConfiguration);
	return true;
}

boolean CBoxAlgorithmAdaptiveP300Classifier::loadConfiguration(const IMemoryBuffer& rMemoryBuffer)
{
	m_f64Class1=0;
	m_f64Class2=0;
	XML::IReader* l_pReader=XML::createReader(*this);
	l_pReader->processData(rMemoryBuffer.getDirectPointer(), rMemoryBuffer.getSize());
	l_pReader->release();
	l_pReader=NULL;

	return true;
}

void CBoxAlgorithmAdaptiveP300Classifier::write(const char* sString)
{
	m_oConfiguration.append((const uint8*)sString, ::strlen(sString));
}

void CBoxAlgorithmAdaptiveP300Classifier::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{
	m_vNode.push(sName);
	
	if(CString(sName)==CString("Sample"))
	{
		for (uint32 i=0; i<ui64AttributeCount; i++)
			if (CString(*(sAttributeName+i))==CString("label"))
				m_ui64TmpLabel = getConfigurationManager().expandAsUInteger(CString(*(sAttributeValue+i)));
	}	
}

void CBoxAlgorithmAdaptiveP300Classifier::processChildData(const char* sData)
{
	std::stringstream l_sData(sData);

	if(m_vNode.top()==CString("Classes"))
	{
		l_sData >> m_f64Class1;
		l_sData >> m_f64Class2;
		//std::cout << "Classes \n";
	}

	if(m_vNode.top()==CString("CoefficientsClass1") || m_vNode.top()==CString("CoefficientsClass2"))
	{
		//std::cout << "Coefficients \n";
		std::vector < float64 > l_vCoefficients;
		while(!l_sData.eof())
		{
			float64 l_f64Value;
			l_sData >> l_f64Value;
			l_vCoefficients.push_back(l_f64Value);
		}
		if(m_vNode.top()==CString("CoefficientsClass1"))
		{
			m_oCoefficientsClass1.set_size(l_vCoefficients.size());
			//std::cout << "coefficients 1\n";
			for(size_t i=0; i<l_vCoefficients.size(); i++)
			{
				m_oCoefficientsClass1[i]=l_vCoefficients[i];
				//std::cout << m_oCoefficientsClass1[i] << " ";
			}
			//std::cout << "\n";
		}	
		else if (m_vNode.top()==CString("CoefficientsClass2"))
		{
			m_oCoefficientsClass2.set_size(l_vCoefficients.size());
			//std::cout << "coefficients 2\n";
			for(size_t i=0; i<l_vCoefficients.size(); i++)
			{
				m_oCoefficientsClass2[i]=l_vCoefficients[i];
				//std::cout << m_oCoefficientsClass2[i] << " ";
			}
			//std::cout << "\n";
		}		
	}
	
	if(m_vNode.top()==CString("Sample")
	{
		std::vector < float64 > l_vSample;
		while(!l_sData.eof())
		{
			float64 l_f64Value;
			l_sData >> l_f64Value;
			l_vSample.push_back(l_f64Value);
		}	
		ittp:vec l_vSampleVector(l_vSample.size());
		for(size_t i=0; i<l_vCoefficients.size(); i++)
			l_vSampleVector[i]=l_vSample[i];
		
		m_vCircularSampleBuffer[m_ui32BufferPointer] = l_vSampleVector;
		m_vCircularLabelBuffer[m_ui32BufferPointer] = m_ui64TmpLabel;
		m_ui32BufferPointer++;
	}	
}

void CBoxAlgorithmAdaptiveP300Classifier::closeChild(void)
{
	m_vNode.pop();
}*/
