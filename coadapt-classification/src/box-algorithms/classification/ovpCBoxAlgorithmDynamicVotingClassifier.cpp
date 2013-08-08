#include "ovpCBoxAlgorithmDynamicVotingClassifier.h"

#include <fstream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace std;

boolean CBoxAlgorithmDynamicVotingClassifier::initialize(void)
{
	IBox* l_pStaticBoxContext=getBoxAlgorithmContext()->getStaticBoxContext();

	// Stimulation stream encoder
	m_oAlgo0_StimulationEncoder.initialize(*this);
	m_StimulationEncoderGroup1.initialize(*this);
	m_StimulationEncoderGroup2.initialize(*this);
	m_oAlgo0_StimulationDecoder.initialize(*this);
	// Streamed matrix stream decoder
	m_oAlgo1_StreamedMatrixDecoder.initialize(*this);


	m_ui32Dimension = l_pStaticBoxContext->getInputCount()-1;
	m_ui32NumberOfSymbol = 36;

	//pi letter score
	m_vPiLetterScore.set_size(m_ui32NumberOfSymbol);
	m_vPiLetterScore.ones();
	for (uint32 i=0;i<m_vPiLetterScore.size(); i++)
	{
		m_vPiLetterScore(i)/=m_vPiLetterScore.size();
	}

	m_vCumulativeScore.set_size(m_ui32NumberOfSymbol);
	m_vCumulativeScore.zeros();

	//score of the current flash
	m_vCurrentScore.set_size(m_ui32NumberOfSymbol);
	m_vCurrentScore.zeros();

	m_ui32CheckedEntries = 0;

	m_bSkipRepetition = false;
	
	m_f64Entropy = 0;
	m_f64Threeshold = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_ui64ClassLabelBaseGroup1 =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	m_ui64ClassLabelBaseGroup2 =FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
	m_ui32MinimalRepetition = 2;
	m_ui32CurrentRepetition = 0;
	m_ui32MaximalRepetition = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);


	//defining the group of letters------------------------------------------------------------------------
	CString l_sLettersConfigurationFilename(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3));
	this->getLogManager() << LogLevel_Info << "Getting letters configuration from file [" << l_sLettersConfigurationFilename << "]\n";
	CMemoryBuffer l_pLettersConfigurationFile;
	ifstream l_oLettersFile(l_sLettersConfigurationFilename.toASCIIString(), ios::binary);
	//*/
	if(l_oLettersFile.is_open())
	{
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
	/*//print to check
	map <OpenViBE::uint32,std::vector<int>>::iterator it;
	for (it=m_mLettersGroups.begin();it!=m_mLettersGroups.end();it++)
	{
		int test;
		this->getLogManager() << LogLevel_Fatal<<"group: "<< (*it).first <<"\n ";
		for (uint32 j=0;j< m_mLettersGroups[(*it).first].size();j++)
		{
			test = m_mLettersGroups[(*it).first][j];
			this->getLogManager() << LogLevel_Fatal<<"		file : "<< test<<"\n ";
		}
	}
	//*/


	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmDynamicVotingClassifier::uninitialize(void)
{
	m_oAlgo0_StimulationEncoder.uninitialize();
	m_StimulationEncoderGroup1.uninitialize();
	m_StimulationEncoderGroup2.uninitialize();
	m_oAlgo0_StimulationEncoder.uninitialize();
	m_oAlgo0_StimulationDecoder.uninitialize();
	m_oAlgo1_StreamedMatrixDecoder.uninitialize();

	return true;
}

boolean CBoxAlgorithmDynamicVotingClassifier::loadConfiguration(const IMemoryBuffer& rMemoryBuffer)
{

	XML::IReader* l_pReader=XML::createReader(*this);
	l_pReader->processData(rMemoryBuffer.getDirectPointer(), rMemoryBuffer.getSize());
	l_pReader->release();
	l_pReader=NULL;
	return true;
}

void CBoxAlgorithmDynamicVotingClassifier::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{
	m_vNode.push(sName);
}

void CBoxAlgorithmDynamicVotingClassifier::processChildData(const char* sData)
{
	std::stringstream l_sData(sData);

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

void CBoxAlgorithmDynamicVotingClassifier::closeChild(void)
{
	m_vNode.pop();
}

boolean CBoxAlgorithmDynamicVotingClassifier::processInput(uint32 ui32InputIndex)
{
	// some pre-processing code if needed...

	// ready to process !
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}
/*******************************************************************************/

boolean CBoxAlgorithmDynamicVotingClassifier::process(void)
{
	
	IBox* l_pStaticBoxContext=getBoxAlgorithmContext()->getStaticBoxContext();
	IBoxIO* l_pDynamicBoxContext=getBoxAlgorithmContext()->getDynamicBoxContext();

	uint32 l_ui32NrMatrixInputs = l_pStaticBoxContext->getInputCount()-1;

	uint64 l_ui64StartTime=0;
	uint64 l_ui64EndTime=0;
	uint64 l_ui64ChunkSize=0;
	const uint8* l_pChunkBuffer=NULL;
	OpenViBE::boolean l_bEarlyStopping = false;
	OpenViBE::boolean l_bCanComputeEntropy = true;
	float64 l_f64CurrentSoftmaxSum = 0;
	
	for(uint32 j=0; j<l_pDynamicBoxContext->getInputChunkCount(0); j++)
	{
		//decode
			m_oAlgo0_StimulationDecoder.decode(0,j);
			if (m_oAlgo0_StimulationDecoder.isHeaderReceived())
			{
			}
			if (m_oAlgo0_StimulationDecoder.isBufferReceived())
			{
				IStimulationSet* l_pStimulationSet = m_oAlgo0_StimulationDecoder.getOutputStimulationSet();
				for(uint64 j=0; j<l_pStimulationSet->getStimulationCount(); j++)
				{
					//Even though the trial stop stimulus is received, we can still receive scores from the last repetition
					if(l_pStimulationSet->getStimulationIdentifier(j)==OVTK_StimulationId_TrialStop)
						this->getLogManager() << LogLevel_Trace << "Trial Stop stimulus received\n";
					//note that the next segment start stimulus can be received before all scores from the previous repetition have arrived
					if(l_pStimulationSet->getStimulationIdentifier(j)==OVTK_StimulationId_SegmentStart)
						this->getLogManager() << LogLevel_Trace << "SegmentStart stimulus received\n";
					//We assume that once the Rest Stop stimulus is received, no other scores for this trial will arrive
					//Note: to avoid the above situation we might ignore all scores with a start date coming before the start date of the stimulus
					if(l_pStimulationSet->getStimulationIdentifier(j)==OVTK_StimulationId_RestStop)
					{
						this->getLogManager() << LogLevel_Trace << "Rest Stop stimulus received\n";
						m_bSkipRepetition = false;
						m_ui32CheckedEntries = 0;
						m_vCurrentScore.zeros();
					}	
				}
			}
			l_pDynamicBoxContext->getInputChunk(0, j, l_ui64StartTime, l_ui64EndTime, l_ui64ChunkSize, l_pChunkBuffer);
			l_pDynamicBoxContext->appendOutputChunkData(0, l_pChunkBuffer, l_ui64ChunkSize);
			l_pDynamicBoxContext->markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64EndTime);
			l_pDynamicBoxContext->markInputAsDeprecated(0, j);
	}


	for(uint32 i=1; i<l_pStaticBoxContext->getInputCount(); i++)
	{
		for(uint32 j=0; j<l_pDynamicBoxContext->getInputChunkCount(i); j++)
		{
			//decode
			m_oAlgo1_StreamedMatrixDecoder.decode(i,j);
			if (m_oAlgo1_StreamedMatrixDecoder.isBufferReceived() && !m_bSkipRepetition)
			{
				OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* >& l_mMatrix = m_oAlgo1_StreamedMatrixDecoder.getOutputMatrix();
				float64* l_mMatrixRawBuffer  = l_mMatrix->getBuffer(); 
				if ( l_mMatrix->getBufferElementCount() != 1)
					this->getLogManager() << LogLevel_ImportantWarning <<"input matrix should have a single value \n";

				std::set<int> current_groupOfLetter(m_mLettersGroups[i-1]);
				for (std::set<int>::iterator l=current_groupOfLetter.begin();l!=current_groupOfLetter.end();l++)
				{
					uint32 s = *l;
					m_vCurrentScore(s-1) -= 0.01*l_mMatrixRawBuffer[0];
				}
				
				m_ui32CheckedEntries++;

				this->getLogManager() << LogLevel_Debug << " Score for input " << i-1 << " is " << -l_mMatrixRawBuffer[0] << "\n";				
			}

			l_pDynamicBoxContext->markInputAsDeprecated(i, j);
		}
	}

	if(m_ui32CheckedEntries<l_ui32NrMatrixInputs)
		l_bCanComputeEntropy = false;	
	else if(m_ui32CheckedEntries==l_ui32NrMatrixInputs)
	{
		m_ui32CheckedEntries = 0;
	}
	else
		this->getLogManager() << LogLevel_Error << "Already received inputs for the next repetition. This possibility has not been taken into account yet.\n";

	//if all the entries are checked and we have not yet made a decision in this trial, we can compute the best choice
	int l_iBestChoice = 0; //index starts from zero
	if (l_bCanComputeEntropy && !m_bSkipRepetition)
	{
		//current repetition should only be increased when we have received scores on all inputs
		m_ui32CurrentRepetition++;

		for (uint32 i=0; i<m_ui32NumberOfSymbol;i++)
		{
			m_vCumulativeScore(i) += m_vCurrentScore(i);
			this->getLogManager() << LogLevel_Debug << "Cumulative score of symbol " << i << " is " << m_vCumulativeScore(i) << "\n";
		}
		//current set of scores should be reset to zero again, ready for the next round of voting
		m_vCurrentScore.zeros();

		//compute the probabilities of each letter
		float64 l_f64SoftmaxCoefficientsSum = 0;
		for (uint32 s=0;s<m_ui32NumberOfSymbol;s++)
			l_f64SoftmaxCoefficientsSum += std::exp(m_vCumulativeScore(s));

		//compute the PiLetter coefficients
		float64 l_f64PiLetterCheckSum = 0;
		float64 l_f64HighestScore = -1;
		for (uint32 s=0;s<m_ui32NumberOfSymbol;s++)
		{
			m_vPiLetterScore(s) = std::exp(m_vCumulativeScore(s))/l_f64SoftmaxCoefficientsSum;
			if( m_vPiLetterScore(s) > l_f64HighestScore ) 
			{
				l_f64HighestScore=m_vPiLetterScore(s);
				l_iBestChoice = s;
			}
			this->getLogManager() << LogLevel_Debug << "Letter likelihood " << m_vPiLetterScore(s) << "\n";
			l_f64PiLetterCheckSum += m_vPiLetterScore(s);
		}
		
		//the entropy
		/*m_f64Entropy = 0;
		for (uint32 i=0; i<m_ui32NumberOfSymbol;i++)
		{
			m_f64Entropy += -m_vPiLetterScore(i)*std::log(m_vPiLetterScore(i));
		}
		this->getLogManager() << LogLevel_Debug <<"entropy "<<m_f64Entropy<<" threshold "<<m_f64Threeshold<<"\n";*/
		
		//check if necessary repetitions have been completed and if the threshold is reached or not
		if (m_ui32MinimalRepetition <= m_ui32CurrentRepetition)
			if (l_f64HighestScore >= m_f64Threeshold)
				l_bEarlyStopping = true;	

		this->getLogManager() << LogLevel_Trace << "Repetition " << m_ui32CurrentRepetition << ", best choice in current repetition " << l_iBestChoice << "\n";

		//in case we reached the maximal number of repetitions we should also proceed with voting
		if ( m_ui32CurrentRepetition == m_ui32MaximalRepetition )
		{
			l_bEarlyStopping = true;
			this->getLogManager() << LogLevel_Trace << "Reached maximal number of repetitions\n";
		}
	}

	//We make a decision only when we have reached the threshold of confidence and we have not yet made a decision this trial
	if (l_bEarlyStopping && !m_bSkipRepetition)
	{
		for (uint32 i=0; i<m_ui32NumberOfSymbol;i++)
			this->getLogManager() << LogLevel_Debug << "Early stopping, score  for symbol " << i << " is " << m_vCumulativeScore(i) << "\n";

		// we get the current time of the player
		uint64 l_ui64StimulationDate = this->getPlayerContext().getCurrentTime();

		IStimulationSet* l_pStimulationSet = m_oAlgo0_StimulationEncoder.getInputStimulationSet();
		l_pStimulationSet->clear();
		l_pStimulationSet->appendStimulation(OVTK_StimulationId_Target, l_ui64StimulationDate, 0);
		m_oAlgo0_StimulationEncoder.encodeBuffer(0);
		this->getDynamicBoxContext().markOutputAsReadyToSend(0,l_ui64StartTime, l_ui64StimulationDate);

		uint64 l_ui64ClassLabelGroup1, l_ui64ClassLabelGroup2 = 0;

		for (int m=0; m<12; m++)
		{
			std::set<int> letterGroup(m_mLettersGroups[m]);
			if (letterGroup.find(l_iBestChoice+1)!=letterGroup.end())
			{
				if(m<6)
					l_ui64ClassLabelGroup1 = m_ui64ClassLabelBaseGroup1 + m;
				else
					l_ui64ClassLabelGroup2 = m_ui64ClassLabelBaseGroup1 + m;
			}
		}				

		IStimulationSet* l_pStimulationSetGroup1 = m_StimulationEncoderGroup1.getInputStimulationSet();
		IStimulationSet* l_pStimulationSetGroup2 = m_StimulationEncoderGroup2.getInputStimulationSet();
		l_pStimulationSetGroup1->clear();
		l_pStimulationSetGroup2->clear();
		l_pStimulationSetGroup1->appendStimulation(l_ui64ClassLabelGroup1, l_ui64StimulationDate, 0);
		l_pStimulationSetGroup2->appendStimulation(l_ui64ClassLabelGroup2, l_ui64StimulationDate, 0);
		m_StimulationEncoderGroup1.encodeBuffer(1);
		m_StimulationEncoderGroup2.encodeBuffer(2);
		this->getDynamicBoxContext().markOutputAsReadyToSend(1,l_ui64StartTime, l_ui64StimulationDate);
		this->getDynamicBoxContext().markOutputAsReadyToSend(2,l_ui64StartTime, l_ui64StimulationDate);

		//Reseting certain parameters
		l_bEarlyStopping = false; //we do not want to enter this part again before a new threshold is achieved
		//If we have not reached the maximal number of repetitions (and thus really stop early), we should indicate that the rest of the repetitions within this trial can be skipped (especially important when reading a file). If we have reached the maximum repetitions then we should not skip the next repetition (the trial stop stimulus can be received before all scores of the repetition have arrived, so setting this to true on the last repetition would skip the repetitions of the next trial as the variable would not be reset on reception of the trial stop stimulus).
		if ( m_ui32CurrentRepetition != m_ui32MaximalRepetition )
			m_bSkipRepetition = true;

		//reset the probabilities of the letters back to the uniform distribution
		m_vPiLetterScore.ones();
		for (uint32 i=0;i<m_vPiLetterScore.size(); i++)
		{
			m_vPiLetterScore(i)/=m_vPiLetterScore.size();
		}
		//reset the cumulutative score, so we can restart for the next trial
		m_vCumulativeScore.zeros();
		//reset the current repetition to zero
		m_ui32CurrentRepetition = 0;
	}
	//*/
	return true;
}
