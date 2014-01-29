#ifndef __ovExternalP300StimulatorPropertyReader__
#define __ovExternalP300StimulatorPropertyReader__

#include <queue>
#include <list>
#include <string>
#include <cmath>

#include "ovexP300PropertyReader.h"

namespace OpenViBEApplications
{

	class P300StimulatorPropertyReader : public ExternalP300PropertyReader
	{
	public:	
		P300StimulatorPropertyReader(OpenViBE::Kernel::IKernelContext* kernelContext, std::list<std::string> * symbolList)
		: ExternalP300PropertyReader(kernelContext), m_lSymbolList(symbolList) 
		{
			m_oTargetStimuli = new std::queue< OpenViBE::uint64 >();
		}
		
		~P300StimulatorPropertyReader()
		{
			delete m_oTargetStimuli;
		}
		
		OpenViBE::uint64 getStimulationBase() { return m_ui64StimulationBase; }
		//OpenViBE::uint64 getBaseStimulationGroup2() { return m_ui64StimulationBaseGroup2; }
		//OpenViBE::uint64 getGroupSize() { return m_ui32GroupSize; }
		//OpenViBE::uint64 getNumberOfFlashesInRepetition() { return std::floor(m_lSymbolList->size()/m_ui32GroupSize); }
		//OpenViBE::uint64 getNumberOfFlashesInRepetition() { return 2*m_ui32InitialNumberOfGroups; }
		OpenViBE::uint32 getNumberOfGroups() { return m_ui32InitialNumberOfGroups; }
		//OpenViBE::uint64 getGroup2Size() { return m_ui32Group2Size; }
		OpenViBE::uint32 getNumberOfRepetitions() { return m_ui32RepetitionCountInTrial; }
		OpenViBE::uint32 getMinNumberOfRepetitions() { return m_ui32MinRepetitions; }
		OpenViBE::uint32 getNumberOfTrials() { return m_ui32TrialCount; }
		OpenViBE::uint64 getFlashDuration() { return m_ui64FlashDuration; }
		//OpenViBE::uint64 getNoFlashDuration() { return m_ui64NoFlashDuration; }
		OpenViBE::uint64 getInterStimulusOnset() { return m_ui64InterStimulusOnset; }
		OpenViBE::uint64 getInterRepetitionDelay() { return m_ui64InterRepetitionDuration; }
		OpenViBE::uint64 getInterTrialDuration() { return m_ui64InterTrialDuration; }
		std::queue< OpenViBE::uint64 > * getTargetStimuli() { return m_oTargetStimuli; }
		OpenViBE::CString getSharedMemoryName() { return m_sSharedMemoryName; }

	protected:
		void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount); // XML IReaderCallback
		void processChildData(const char* sData); // XML IReaderCallback
		void closeChild(void); // XML ReaderCallback

	protected:
		OpenViBE::uint64 m_ui64StimulationBase;
		//OpenViBE::uint64 m_ui64StimulationBaseGroup2;

		//OpenViBE::uint32 m_ui32Group2Size;
		OpenViBE::uint32 m_ui32InitialNumberOfGroups;

		OpenViBE::uint32 m_ui32RepetitionCountInTrial;
		OpenViBE::uint32 m_ui32MinRepetitions;
		OpenViBE::uint32 m_ui32TrialCount;
		OpenViBE::uint64 m_ui64FlashDuration;
		//OpenViBE::uint64 m_ui64NoFlashDuration;
		OpenViBE::uint64 m_ui64InterStimulusOnset;
		OpenViBE::uint64 m_ui64InterRepetitionDuration;
		OpenViBE::uint64 m_ui64InterTrialDuration;
		std::string m_sTargetWord;
		std::queue< OpenViBE::uint64 > * m_oTargetStimuli;
		std::list<std::string> * m_lSymbolList;

		OpenViBE::CString m_sSharedMemoryName;	
		
	private:
		OpenViBE::uint64 findSymbolIndex(std::string symbol);
	
	};
};

#endif
