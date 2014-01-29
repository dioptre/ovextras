#include "ovpCBoxAlgorithmPGTrainer.h"

#include <system/Memory.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;
using namespace OpenViBEToolkit;
using namespace std;

boolean CBoxAlgorithmPGTrainer::initialize(void)
{
	uint32 i;
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	//getting the algorithm coefficients
	m_f64Etha = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	m_f64Lambda = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

	//problem dimensions
	m_ui64NumberOfRows = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
	m_ui64NumberOfColumns = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

	m_ui64TrainStimulation=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

	//temperature parameter
	m_f64Temperature = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);
	//feature vector decoder---------------------------------------
	for(i=2; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		m_vFeatureVectorsDecoder[i-2]=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_FeatureVectorStreamDecoder));
		m_vFeatureVectorsDecoder[i-2]->initialize();
	}
	//----------------------------------------------errp stimulations decoder
	for(i=0; i<2; i++)
	{
		IAlgorithmProxy* m_pStreamDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamDecoder));
		m_pStreamDecoder->initialize();
		m_vStimulationsDecoder.push_back(m_pStreamDecoder);

		TParameterHandler < const IMemoryBuffer* > m_pMemoryBuffer(m_vStimulationsDecoder[i]->getInputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_InputParameterId_MemoryBufferToDecode));
		TParameterHandler < IStimulationSet* > m_pStimulationSet(m_vStimulationsDecoder[i]->getOutputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_OutputParameterId_StimulationSet));
		ip_pEncodedMemoryBuffer.push_back(m_pMemoryBuffer);
		op_pStimulationSet.push_back(m_pStimulationSet);
	}
	//-----------------------------------------------------

	m_vFeatureCount.clear();
	m_vFeatureCount.resize(this->getStaticBoxContext().getInputCount()-2);
	//stimulation encoder -------------------------------------------------------
	m_pStimulationsEncoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamEncoder));
	m_pStimulationsEncoder->initialize();

	//verify a previous training does not already exist --------------------------------------------------------------------
	CString l_sConfigurationFilename(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));
	CMemoryBuffer l_pConfigurationFile;
	ifstream l_oFile(l_sConfigurationFilename.toASCIIString(), ios::binary);
	if(l_oFile.is_open())
	{
		size_t l_iFileLen;
		l_oFile.seekg(0, ios::end);
		l_iFileLen=l_oFile.tellg();
		l_oFile.seekg(0, ios::beg);
		l_pConfigurationFile.setSize(l_iFileLen, true);
		l_oFile.read((char*)l_pConfigurationFile.getDirectPointer(), l_iFileLen);
		l_oFile.close();
		//make the algorithm use it
		loadConfiguration(l_pConfigurationFile);

		this->getLogManager() << LogLevel_Info<< "Loading last classifier configuration from file " << l_sConfigurationFilename << "\n";
	}			
	else
	{
		this->getLogManager() << LogLevel_Error << "Could not load classifier configuration from file [" << l_sConfigurationFilename << "]\n";
	}
	//------------------------------------------------------------------------------------------------
	m_ui32NumberOfRepetitions = 0;

	//loading the splotch configuration file
	CString l_sLettersConfigurationFilename(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6));
	CMemoryBuffer l_pLettersConfigurationFile;
	ifstream l_oLettersFile(l_sLettersConfigurationFilename.toASCIIString(), ios::binary);
	if(l_oLettersFile.is_open())
	{
		this->getLogManager() << LogLevel_Info << "Loading letter configuration file " << l_sLettersConfigurationFilename << "\n";
		size_t l_iFileLen;
		l_oLettersFile.seekg(0, ios::end);
		l_iFileLen=l_oLettersFile.tellg();
		l_oLettersFile.seekg(0, ios::beg);

		l_pLettersConfigurationFile.setSize(l_iFileLen, true);//set size and discard true
		l_oLettersFile.read((char*)l_pLettersConfigurationFile.getDirectPointer(), l_iFileLen);
		l_oLettersFile.close();
		//make the algorithm use it
		loadConfiguration(l_pLettersConfigurationFile);
	}			
	else
	{
		this->getLogManager() << LogLevel_Error << "Could not load letters configuration from file [" << l_sLettersConfigurationFilename << "]\n";
	}

	m_bCheckInput = false;
	m_bTrainStimulationReceived = false;
	m_bIsLineChose = false;
	m_bIsColumnChose = false;
	//m_ui32CheckedEntries = 0;
	
	return true;
}

boolean CBoxAlgorithmPGTrainer::uninitialize(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();

	m_pStimulationsEncoder->uninitialize();

	this->getAlgorithmManager().releaseAlgorithm(*m_pClassifier);
	this->getAlgorithmManager().releaseAlgorithm(*m_pStimulationsEncoder);

	for(uint32 i=2; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		m_vFeatureVectorsDecoder[i-2]->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_vFeatureVectorsDecoder[i-2]);
	}
	m_vFeatureVectorsDecoder.clear();

	// uninitialize decoder from errp
	for(uint32 i=0; i<2; i++)
	{
		m_vStimulationsDecoder[i]->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_vStimulationsDecoder[i]);
	}
	m_vStimulationsDecoder.clear();

	return true;
}

boolean CBoxAlgorithmPGTrainer::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmPGTrainer::process(void)
{
	IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
	

	uint32 i, j;
	//m_bTrainStimulationReceived=false;

	// Parses stimulations
	for(i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		
		ip_pEncodedMemoryBuffer[0]=l_rDynamicBoxContext.getInputChunk(0, i);

		TParameterHandler < IStimulationSet* > ip_pStimulationSet(m_pStimulationsEncoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_InputParameterId_StimulationSet));
		TParameterHandler < IMemoryBuffer* > op_pEncodedMemoryBuffer(m_pStimulationsEncoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_OutputParameterId_EncodedMemoryBuffer));

		CStimulationSet l_oStimulationSet;
		ip_pStimulationSet=&l_oStimulationSet;

		if(l_rStaticBoxContext.getOutputCount()>=1)
		{
			op_pEncodedMemoryBuffer=l_rDynamicBoxContext.getOutputChunk(0);
		}
		else
		{
			op_pEncodedMemoryBuffer->setSize(0, true);
		}

		m_vStimulationsDecoder[0]->process();

		if(m_vStimulationsDecoder[0]->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			m_pStimulationsEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeHeader);
		}
		if(m_vStimulationsDecoder[0]->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedBuffer))
		{
			for(uint64 j=0; j<op_pStimulationSet[0]->getStimulationCount(); j++)
			{
				if(op_pStimulationSet[0]->getStimulationIdentifier(j)==m_ui64TrainStimulation)
				{
					this->getLogManager() << LogLevel_Warning << "Received train stimulation " << "\n";

					m_bTrainStimulationReceived=true;

					//we check the number of feature vectors that have been received on each input and take the highest number as the effective number of repetitions
					m_ui32NumberOfRepetitions = 0;
					for (int ii=0; ii<m_vFeatureCount.size(); ii++)
						if (m_ui32NumberOfRepetitions<m_vFeatureCount[ii])
							m_ui32NumberOfRepetitions = m_vFeatureCount[ii];

					this->getLogManager() << LogLevel_Trace << "Repetition " << m_ui32NumberOfRepetitions << "\n";

					m_bCheckInput = checkInputs();
				}

				//on the trial stop stimulus it could still happen that feature vectors arrive so we reset everything upon reception of the rest stop stimulation (this should clearly indicate the end of the trial and no further vectors of that trial should arrive - how else could feedback be generated)
				if(op_pStimulationSet[0]->getStimulationIdentifier(j)==OVTK_StimulationId_RestStop)
				{
					this->getLogManager() << LogLevel_Warning << "Received REST STOP stimulation " << "\n";
					m_ui32NumberOfRepetitions = 0;
					m_vFeatureVector.clear();
					m_vFeatureCount.clear();
					m_vFeatureCount.resize(this->getStaticBoxContext().getInputCount()-2);
				}
			}
			m_pStimulationsEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeBuffer);
		}
		if(m_vStimulationsDecoder[0]->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedEnd))
		{
			m_pStimulationsEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeEnd);
		}

		if(l_rStaticBoxContext.getOutputCount()>=1)
		{
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		}

		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}

	// Parses feature vectors ---------------------------------------------------------------------------------------------------------------------------------------
	for(i=2; i<l_rStaticBoxContext.getInputCount(); i++)
	{
		for(j=0; j<l_rDynamicBoxContext.getInputChunkCount(i); j++)
		{
			TParameterHandler < const IMemoryBuffer* > ip_pFeatureVectorMemoryBuffer(m_vFeatureVectorsDecoder[i-2]->getInputParameter(OVP_GD_Algorithm_FeatureVectorStreamDecoder_InputParameterId_MemoryBufferToDecode));
			TParameterHandler < const IMatrix* > op_pFeatureVectorMatrix(m_vFeatureVectorsDecoder[i-2]->getOutputParameter(OVP_GD_Algorithm_FeatureVectorStreamDecoder_OutputParameterId_Matrix));
			ip_pFeatureVectorMemoryBuffer=l_rDynamicBoxContext.getInputChunk(i, j);
			m_vFeatureVectorsDecoder[i-2]->process();

			if(m_vFeatureVectorsDecoder[i-2]->isOutputTriggerActive(OVP_GD_Algorithm_FeatureVectorStreamDecoder_OutputTriggerId_ReceivedBuffer))
			{
				CBoxAlgorithmPGTrainer::SFeatureVector l_oFeatureVector;
				l_oFeatureVector.m_pFeatureVectorMatrix=new CMatrix();
				l_oFeatureVector.m_ui64StartTime=l_rDynamicBoxContext.getInputChunkStartTime(i, j);
				l_oFeatureVector.m_ui64EndTime=l_rDynamicBoxContext.getInputChunkEndTime(i, j);
				l_oFeatureVector.m_ui32InputIndex=i-2;
				
				//count the number of feature vectors received per input
				m_vFeatureCount[i-2]++;
				l_oFeatureVector.m_ui32RepetitionIndex=m_vFeatureCount[i-2];
				OpenViBEToolkit::Tools::Matrix::copy(*l_oFeatureVector.m_pFeatureVectorMatrix, *op_pFeatureVectorMatrix);
				m_vFeatureVector.push_back(l_oFeatureVector);

				if (m_bTrainStimulationReceived) 
					m_bCheckInput = checkInputs();			
				
			}

			l_rDynamicBoxContext.markInputAsDeprecated(i, j);

		}
	}

	//errp input stimulation------------------------------------------------------------------------------------------------------------------------------------------------
	for (uint32 k =1; k<2; k++)
	{
		for(i=0; i<l_rDynamicBoxContext.getInputChunkCount(k); i++)
		{
			ip_pEncodedMemoryBuffer[k]=l_rDynamicBoxContext.getInputChunk(k, i);
			m_vStimulationsDecoder[k]->process();

			if(m_vStimulationsDecoder[k]->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedBuffer))
			{
				for(uint64 j=0; j<op_pStimulationSet[k]->getStimulationCount(); j++)
				{
					uint64 l_ui64StimulationCode = op_pStimulationSet[k]->getStimulationIdentifier(j);
					//the first stimulation tell if the target has been reached
					if (j==0)
					{
						if (l_ui64StimulationCode == OVTK_StimulationId_Target)
							m_bIsAnswerCorrect = true;
						else
							m_bIsAnswerCorrect = false;
					}
					//the second stimulation is the line
					if (j==1)
					{
						m_ui64ChosenLineIdentifier = l_ui64StimulationCode;
						m_bIsLineChose=true;
					}
					//the third is the column
					if (j==2)
					{
						m_ui64ChosenColumnIdentifier = l_ui64StimulationCode;
						m_bIsColumnChose=true;
					}	
				}
			}
			l_rDynamicBoxContext.markInputAsDeprecated(k, i);
		}
	}
	//---------------------------------------------------------------------------------------------------------------------------------------

	//On train stimulation reception, build up the labelled feature vector set matrix and go on training
	//if the errp reward is computed
	//m_bAllowTraining = m_bAllowTraining && m_bIsLineChose && m_bIsColumnChose;
	if(m_bIsLineChose && m_bIsColumnChose && m_bTrainStimulationReceived && m_bCheckInput)
	{
		if(m_vFeatureVector.size()==0)
			this->getLogManager() << LogLevel_Warning << "Received train stimulation but no feature vector\n";
		else
		{
			this->getLogManager() << LogLevel_Info << "Start training\n";
			
			m_oBoxConfiguration.setSize(0,true);

			m_ui64ChosenLineIdentifier -= OVTK_StimulationId_Label_01;
			m_ui64ChosenColumnIdentifier -= OVTK_StimulationId_Label_01;

			// create a vector used for mapping feature vectors (initialize it as v[i] = i)
			//clear the previous trial
			m_vFeatureVectorIndex.clear();
			for (uint32 i = 0; i < m_vFeatureVector.size(); i++)
				m_vFeatureVectorIndex.push_back(i);

			//CALL TRAINING ALGORITHM
			Algotrain(m_vFeatureVector);

			uint64 l_ui64StimulationDate = this->getPlayerContext().getCurrentTime();

			this->getLogManager() << LogLevel_Info << "Raising train-completed Flag.\n";

			TParameterHandler < IStimulationSet* > ip_pStimulationSet(m_pStimulationsEncoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_InputParameterId_StimulationSet));
			
			CStimulationSet l_oStimulationSet;
			ip_pStimulationSet=&l_oStimulationSet;
			uint64 l_ui32TrainCompletedStimulation = this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation,"OVTK_StimulationId_TrainCompleted");
			l_oStimulationSet.appendStimulation(l_ui32TrainCompletedStimulation, l_ui64StimulationDate, 0);
			m_pStimulationsEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeBuffer);

			this->getDynamicBoxContext().markOutputAsReadyToSend(0,0, l_ui64StimulationDate);

			m_ui32NumberOfRepetitions = 0;
			saveConfiguration(m_oBoxConfiguration);

			CString l_sConfigurationFilename(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));
			std::ofstream l_oFile(l_sConfigurationFilename.toASCIIString(), ios::binary);
			if(l_oFile.is_open())
			{
				l_oFile.write((char*)m_oBoxConfiguration.getDirectPointer(), (std::streamsize)m_oBoxConfiguration.getSize());
				l_oFile.close();
			}
			else
			{
				this->getLogManager() << LogLevel_Error << "Could not save configuration to file [" << l_sConfigurationFilename << "]\n";
				return false;
			}	

			//clear the deprecated FV
			m_vFeatureVector.clear();
			m_vFeatureCount.clear();
			m_vFeatureCount.resize(this->getStaticBoxContext().getInputCount()-2);

			m_bCheckInput = false;
			m_bIsLineChose = false;
			m_bIsColumnChose = false;
			m_bTrainStimulationReceived = false;			
		}
	}

	return true;
}

boolean CBoxAlgorithmPGTrainer::checkInputs()
{
	boolean l_bCheckInput = true;
	this->getLogManager() << LogLevel_Trace << "m_vFeatureCount size is " << (int)m_vFeatureCount.size() << "\n";
	for (uint32 input=0;input<m_vFeatureCount.size();input++)
	{
		if ( m_vFeatureCount[input] != m_ui32NumberOfRepetitions)
		{				
			l_bCheckInput = false;
		}
		this->getLogManager() << LogLevel_Trace << "Input " << input << " has " << m_vFeatureCount[input] << " and should have " << m_ui32NumberOfRepetitions << "\n";
	}

	return l_bCheckInput;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

boolean CBoxAlgorithmPGTrainer::saveConfiguration(IMemoryBuffer& rMemoryBuffer)
{
	rMemoryBuffer.setSize(0, true);
	rMemoryBuffer.append(m_oConfiguration);
	return true;
}

boolean CBoxAlgorithmPGTrainer::loadConfiguration(const IMemoryBuffer& rMemoryBuffer)
{
	m_f64Class1=0;
	m_f64Class2=0;
	this->getLogManager() << LogLevel_Error <<m_f64Class1<<" "<<m_f64Class2<<"\n";
	this->getLogManager() << LogLevel_Error <<"memory buffer size "<<rMemoryBuffer.getSize()<<"\n";

	XML::IReader* l_pReader=XML::createReader(*this);
	l_pReader->processData(rMemoryBuffer.getDirectPointer(), rMemoryBuffer.getSize());
	l_pReader->release();
	l_pReader=NULL;
	return true;
}

void CBoxAlgorithmPGTrainer::write(const char* sString)
{
	m_oConfiguration.append((const uint8*)sString, ::strlen(sString));
}

void CBoxAlgorithmPGTrainer::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{
	m_vNode.push(sName);
}

void CBoxAlgorithmPGTrainer::processChildData(const char* sData)
{
	std::stringstream l_sData(sData);

	if(m_vNode.top()==CString("Classes"))
	{
		this->getLogManager() << LogLevel_Error <<"classes\n";
		l_sData >> m_f64Class1;
		l_sData >> m_f64Class2;
	}

	if(m_vNode.top()==CString("Coefficients"))
	{
		this->getLogManager() << LogLevel_Error <<"coefficients\n";
		std::vector < float64 > l_vCoefficients;
		while(!l_sData.eof())
		{
			float64 l_f64Value;
			l_sData >> l_f64Value;
			l_vCoefficients.push_back(l_f64Value);
		}

		m_oCoefficients.set_size(l_vCoefficients.size());
		for(size_t i=0; i<l_vCoefficients.size(); i++)
		{
			m_oCoefficients[i]=l_vCoefficients[i];
		}
	}
	//*/
	if(m_vNode.top()==CString("Id"))
	{
		//this->getLogManager() << LogLevel_Error <<"id\n";
		l_sData >> m_ui32LetterGroupId;
	}


	if(m_vNode.top()==CString("Group"))
	{
		//this->getLogManager() << LogLevel_Error <<"group\n";
		std::set<int> l_vLetterGroup;
		while(!l_sData.eof())
		{
			uint64 l_ui64LetterIdValue;
			l_sData >> l_ui64LetterIdValue;
			l_vLetterGroup.insert(l_ui64LetterIdValue);
		}
		m_mLettersGroups[m_ui32LetterGroupId] = l_vLetterGroup;
	}
	//*/

}

void CBoxAlgorithmPGTrainer::closeChild(void)
{
	m_vNode.pop();
}

boolean CBoxAlgorithmPGTrainer::Algotrain(std::vector < CBoxAlgorithmPGTrainer::SFeatureVector > rFeatureVectorSet)//(const IFeatureVectorSet& rFeatureVectorSet)
{
	// recuperation of the parameters 
	//feedback
	m_bPreviousResultValidation = m_bIsAnswerCorrect;
	m_ui64PreviousResultLine = m_ui64ChosenLineIdentifier ;
	m_ui64PreviousResultColumn = m_ui64ChosenColumnIdentifier;
	
	//check
	this->getLogManager() << LogLevel_Debug << "PGtrainer choice made: "<<m_ui64PreviousResultLine<<", "<<m_ui64PreviousResultColumn<<" (correct: "<< m_bPreviousResultValidation<<") \n";
	
	//storing parameters

	//some local variables ------------------------------------------------------------------------------------------
	uint32 l_ui32NumberOfFeatures=rFeatureVectorSet[0].m_pFeatureVectorMatrix->getBufferElementCount();
	uint32 l_ui32NumberOfFeatureVectors=rFeatureVectorSet.size();

	//fixed dimensions of the problem given by the user
	uint32 l_ui32NumberOfRow = m_ui64NumberOfRows;
	uint32 l_ui32NumberOfColumn = m_ui64NumberOfColumns;
	uint32 l_ui32NumberOfSymbol = l_ui32NumberOfRow * l_ui32NumberOfColumn;
	
	//variable dimensions given by the box
	uint32 l_ui32NumberOfRepetition = m_ui32NumberOfRepetitions;
	//iterators
	uint32 i,j;

	//initilization
	m_bAreCoefficientsDefine = false;
	if (m_oCoefficients.size()-1==l_ui32NumberOfFeatures)
	{
		m_bAreCoefficientsDefine = true;
		this->getLogManager() << LogLevel_Debug << "Coefficients decision plane are defined: first entry " << m_oCoefficients[1] << " and last entry " << m_oCoefficients[m_oCoefficients.size()-1] << ", temperature: " << m_f64Temperature <<"\n";
	}
	else
		this->getLogManager() << LogLevel_Debug << "coefficients are not defined\n";
	
	//if the coefficients are not defined	
	if (!m_bAreCoefficientsDefine)
	{
		m_oCoefficients.set_size(l_ui32NumberOfFeatures);
		m_oCoefficients.zeros();
	}
	else //removing the bias		
		m_oCoefficients.del(0);

	//probability for each symbol
	m_vPiLetterCoefficients.set_size(l_ui32NumberOfSymbol);
	m_vPiLetterCoefficients.ones();
	for (uint32 i=0;i<m_vPiLetterCoefficients.size();i++)
		m_vPiLetterCoefficients(i) /= m_vPiLetterCoefficients.size();

	//score for each symbol
	m_vSoftmaxCoefficients.set_size(l_ui32NumberOfSymbol);
	m_vSoftmaxCoefficients.zeros();
	this->getLogManager() << LogLevel_Trace << "Number of feature vectors: " << l_ui32NumberOfFeatureVectors << "\n";
	///* for each feature vector
	for (i=0; i<l_ui32NumberOfFeatureVectors; i++)
	{	
		//copy in case we have to modify the feature vector
		itpp::vec l_rFeatureVectorCopy = itpp::vec(rFeatureVectorSet[i].m_pFeatureVectorMatrix->getBuffer(), rFeatureVectorSet[i].m_pFeatureVectorMatrix->getBufferElementCount());	
		//get the label i.e the id of the group of letter of this flash
		j =  rFeatureVectorSet[i].m_ui32InputIndex;	

		this->getLogManager() << LogLevel_Debug << "First and last entry for input vector " << j << " and repetition " << rFeatureVectorSet[i].m_ui32RepetitionIndex << " are " << l_rFeatureVectorCopy[0] << ", " << l_rFeatureVectorCopy[l_ui32NumberOfFeatures-1] << "\n";	
		//group of letter corresponding to this label
		std::set<int> current_groupOfLetter(m_mLettersGroups[j]);
		//computing softmax coefficients for each letters of this group
		set<int>::iterator it;
		for (it=current_groupOfLetter.begin(); it!=current_groupOfLetter.end(); it++)
		{
			uint32 s = *it;// s is between 1 and NumberOfSymbol
			//the vector begins at 0 so we take s-1
			m_vSoftmaxCoefficients(s-1) += m_f64Temperature*m_oCoefficients*l_rFeatureVectorCopy; //W*x
			this->getLogManager() << LogLevel_Debug << "m_vSoftmaxCoefficients(" << s-1 << "): " << m_vSoftmaxCoefficients(s-1) << "\n" ;
		}
	}
	
	float64 l_f64SoftmaxCoefficientsSum = 0;
	for (uint32 s=0;s<l_ui32NumberOfSymbol;s++)
		{
			m_vSoftmaxCoefficients(s) += std::log(m_vPiLetterCoefficients(s)) ;
			this->getLogManager() << LogLevel_Debug << "m_vSoftmaxCoefficients(" << s << "): " << m_vSoftmaxCoefficients(s) << "\n" ;
			l_f64SoftmaxCoefficientsSum += std::exp(m_vSoftmaxCoefficients(s));
		}
	//compute the PiLetter coefficients
	for (uint32 s=0;s<l_ui32NumberOfSymbol;s++)
	{
		m_vPiLetterCoefficients(s) = std::exp(m_vSoftmaxCoefficients(s))/l_f64SoftmaxCoefficientsSum;
		this->getLogManager() << LogLevel_Debug << "m_vPiLetterCoefficients(" << s << "): " << m_vPiLetterCoefficients(s) << "\n" ;
	}

	//--------------------------------------------------------------------------------------------------------------
	int32 l_i32PositiveReward = 5;//at reward to high is not good, some coefficients will go out of bounds
	int32 l_i32NegativeReward = -1;
	
	
	int32 l_i32Reward = m_bPreviousResultValidation?l_i32PositiveReward:l_i32NegativeReward; //+5 is the positive reward, -1 the negative one

	float64 m_f64EthaPrime = m_f64Etha/m_ui32NumberOfRepetitions;
	m_oCoefficients *= 1-m_f64Etha*m_f64Lambda;

	//learning term
	itpp::vec l_vLearningTerm;
	l_vLearningTerm.set_size(l_ui32NumberOfFeatures);
	l_vLearningTerm.zeros();

	//for each repetition, the sum of the probability of the flashes must be 1
	//if we flash rows and columns, each letter is flashed two times, so the sum will be 2 instead of 1
	itpp::vec l_vPiFlash(l_ui32NumberOfRepetition);
	l_vPiFlash.zeros();


	for (i=0; i<l_ui32NumberOfFeatureVectors; i++)
	{
		itpp::vec l_rFeatureVectorCopy = itpp::vec(rFeatureVectorSet[i].m_pFeatureVectorMatrix->getBuffer(), rFeatureVectorSet[i].m_pFeatureVectorMatrix->getBufferElementCount());	
		
		//get the id of the group of letter flashed
		j =  rFeatureVectorSet[i].m_ui32InputIndex;	
		uint32 p = rFeatureVectorSet[i].m_ui32RepetitionIndex;
		//group of letter corresponding to this label
		std::set<int> current_groupOfLetter(m_mLettersGroups[j]);
		
		//probability that this flash contain the target i.e sum of the probability of the letters of this flash
		float64 l_f64PiFlash = 0;
		set<int>::iterator it;
		for (it=current_groupOfLetter.begin(); it!=current_groupOfLetter.end(); it++)
		{
			uint32 s = *it;
			uint32 column=0, line=0;
			//get the coordinate of this symbol on the screen according to the group definitions
			if (j<6)
			{
				line = j;
				for (int m=0; m<6; m++)
				{
					std::set<int> letterGroup(m_mLettersGroups[m+6]);
					if (letterGroup.find(*it)!=letterGroup.end())
						column = m+6;
				}				
			}
			else
			{
				column = j;
				for (int m=0; m<6; m++)
				{
					std::set<int> letterGroup(m_mLettersGroups[m]);
					if (letterGroup.find(*it)!=letterGroup.end())
						line = m;
				}				
			}
		
			this->getLogManager() << LogLevel_Debug << "Char with gridindex " << s << ", line " << line << ", column " << column << ", pred row " << m_ui64PreviousResultLine << ", pred col " << m_ui64PreviousResultColumn << "\n";

			//if this symbol is the selected symbol
			if ((line == m_ui64PreviousResultLine)&&(column == m_ui64PreviousResultColumn))
				l_vLearningTerm += l_rFeatureVectorCopy;
			l_f64PiFlash +=  m_vPiLetterCoefficients(s-1);
		}

		l_vLearningTerm -= l_rFeatureVectorCopy * l_f64PiFlash;
		this->getLogManager() << LogLevel_Debug << "First and last entry of dW for flash " << j << " and repetition " <<  p << " is " << l_vLearningTerm[0]  << ", " << l_vLearningTerm[l_ui32NumberOfFeatures-1] << ". Flash probability is " << l_f64PiFlash << "\n";
		l_vPiFlash(p-1) += l_f64PiFlash;
	}

	this->getLogManager() << LogLevel_Debug << "first and last entry for final vector dW\n";
	this->getLogManager() << LogLevel_Debug << l_vLearningTerm[0]  << ", " << l_vLearningTerm[l_ui32NumberOfFeatures-1] << "\n";

	l_vLearningTerm *=m_f64EthaPrime* l_i32Reward;
	m_oCoefficients += l_vLearningTerm;
	m_oCoefficients.ins(0,0); //bias term at 0

	//update the learning coefficients
	m_f64Class1=1;
	m_f64Class2=2;

	std::stringstream l_sClasses;
	std::stringstream l_sCoefficients;

	l_sClasses << m_f64Class1 << " " << m_f64Class2;
	l_sCoefficients << std::scientific << m_oCoefficients[0];
	for(int i=1; i<m_oCoefficients.size(); i++)
	{
		l_sCoefficients << " " << m_oCoefficients[i];
	}

	m_oConfiguration.setSize(0, true);
	XML::IWriter* l_pWriter=XML::createWriter(*this);
	l_pWriter->openChild("OpenViBE-Classifier");
	 l_pWriter->openChild("LDA");
	  l_pWriter->openChild("Classes");
	   l_pWriter->setChildData(l_sClasses.str().c_str());
	  l_pWriter->closeChild();
	  l_pWriter->openChild("Coefficients");
	   l_pWriter->setChildData(l_sCoefficients.str().c_str());
	  l_pWriter->closeChild();
	 l_pWriter->closeChild();
	l_pWriter->closeChild();
	l_pWriter->release();
	l_pWriter=NULL;

	return true;
}

/*boolean CBoxAlgorithmPGTrainer::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
{
	if(rFeatureVector.getSize()+1!=(unsigned int)m_oCoefficients.size())
	{
		this->getLogManager() << LogLevel_Warning << "Feature vector size " << rFeatureVector.getSize() << " and hyperplane parameter size " << (uint32) m_oCoefficients.size() << " does not match\n";
		return false;
	}
	
	itpp::vec l_oFeatures(rFeatureVector.getBuffer(), rFeatureVector.getSize());
	l_oFeatures.ins(0, 1);

	float64 l_f64Result=-l_oFeatures*m_oCoefficients;

	rClassificationValues.setSize(1);
	rClassificationValues[0]=l_f64Result;
	if(l_f64Result < 0)
	{
		rf64Class=m_f64Class1;
	}
	else
	{
		rf64Class=m_f64Class2;
	}

	return true;
}*/
