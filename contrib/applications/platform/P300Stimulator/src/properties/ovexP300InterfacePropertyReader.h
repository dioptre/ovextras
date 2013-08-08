#ifndef __ovExternalP300InterfacePropertyReader__
#define __ovExternalP300InterfacePropertyReader__

#include "ovexP300PropertyReader.h"

namespace OpenViBEApplications
{

	class P300InterfacePropertyReader : public ExternalP300PropertyReader
	{	
		
	public:	
		P300InterfacePropertyReader(OpenViBE::Kernel::IKernelContext* kernelContext);

		OpenViBE::CString getScreenDefinitionFile() { return m_sSymbolDefinitionFile; }	
		OpenViBE::CString getFlashGroupDefinitionFile() { return m_sFlashGroupDefinitionFile; }
		OpenViBE::CString getStimulatorConfigFile() { return m_sStimulatorConfigFile; }
		OpenViBE::float32 getWidth() { return m_f32WindowWidth; }
		OpenViBE::float32 getHeight() { return m_f32WindowHeight; }	
		OpenViBE::boolean getFullScreen() { return m_bFullScreen; }
		OpenViBE::uint32 getParallelPortNumber() { return m_ui32ParallelPortNumber; }
		OpenViBE::uint32 getSampleFrequency() { return m_ui32SampleFrequency; }
		OpenViBE::boolean getCentralFeedbackFreeMode() { return m_bCentralFeedbackFreeMode; }
		OpenViBE::boolean getCentralFeedbackCopyMode() { return m_bCentralFeedbackCopyMode; }
		SpellingMode getSpellingMode() { return m_eSpellingMode; }
            OpenViBE::boolean getHardwareTagging() { return m_bHardwareTagging; }
            OpenViBE::boolean isPhotoDiodeEnabled() { return m_bEnablePhotoDiode; }
            OpenViBE::CString getFlashMode() { return m_sFlashMode; }
            OpenViBE::uint32 getMaxFeedbackSymbols() { return m_ui32MaxFeedbackSymbols; }
            GColor getFeedbackStartColor() { return m_oFeedbackStartColor;}
		GColor getFeedbackEndColor() { return m_oFeedbackEndColor;}
		OpenViBE::uint32 getColorFeedbackSteps() { return m_ui32ColorFeedbackSteps;}
		OpenViBE::float32 getFeedbackStartValue() { return m_f32FeedbackStartValue;}
		OpenViBE::boolean isContinuousFeedbackEnabled() { return m_bContinuousFeedback;}
		OpenViBE::CString getNGramDatabaseName() { return m_sNGramDatabaseName; }

	protected:
		void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount); // XML IReaderCallback
		void processChildData(const char* sData); // XML IReaderCallback
		void closeChild(void); // XML ReaderCallback

	protected:
		SpellingMode m_eSpellingMode;
		OpenViBE::CString m_sStimulatorConfigFile;
		OpenViBE::CString m_sSymbolDefinitionFile;
		OpenViBE::CString m_AdditionalConfigurationFile;
		OpenViBE::CString m_sFlashGroupDefinitionFile;
		OpenViBE::boolean m_bFullScreen;
		OpenViBE::boolean m_bCentralFeedbackFreeMode;
		OpenViBE::boolean m_bCentralFeedbackCopyMode;
		OpenViBE::uint32 m_ui32ParallelPortNumber;
		OpenViBE::uint32 m_ui32SampleFrequency;
		OpenViBE::float32 m_f32WindowWidth;
		OpenViBE::float32 m_f32WindowHeight;
		OpenViBE::CString m_sFlashMode;
		OpenViBE::CString m_sNGramDatabaseName;
		
		OpenViBE::boolean m_bHardwareTagging;
		OpenViBE::CString m_sFeedbackPresentationMode;
		OpenViBE::boolean m_bEnablePhotoDiode;
		
		OpenViBE::uint32 m_ui32MaxFeedbackSymbols;
		GColor m_oFeedbackStartColor;
		GColor m_oFeedbackEndColor;
		OpenViBE::uint32 m_ui32ColorFeedbackSteps;
		OpenViBE::float32 m_f32FeedbackStartValue;
		OpenViBE::boolean m_bContinuousFeedback;
		
	};
};

#endif
