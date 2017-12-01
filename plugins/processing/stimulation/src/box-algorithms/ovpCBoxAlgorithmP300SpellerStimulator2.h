#ifndef __OpenViBEPlugins_BoxAlgorithm_P300SpellerStimulator2_H__
#define __OpenViBEPlugins_BoxAlgorithm_P300SpellerStimulator2_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <csv/ovICSV.h>

// #include <map>

#define OVP_ClassId_BoxAlgorithm_P300SpellerStimulator2     OpenViBE::CIdentifier(0x258C5823, 0x6B285824)
#define OVP_ClassId_BoxAlgorithm_P300SpellerStimulator2Desc OpenViBE::CIdentifier(0x10093D14, 0x71A59616)

namespace OpenViBEPlugins
{
	namespace Stimulation
	{
		class CBoxAlgorithmP300SpellerStimulator2 : public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
			enum NextStimulatorState
			{
				State_None,
				State_Experiment_Start,
				State_Trial_Start,
				State_Display_Target_Start,
				State_Display_Target_End,
				State_Flash_Start,
				State_Flash_End,
				State_Trial_End,
				State_Rest_Start,
				State_Rest_End,
				State_Experiment_Stop
			};

		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::uint64 getClockFrequency(void);
			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32Index);
			virtual OpenViBE::boolean processClock(OpenViBE::CMessageClock& rMessageClock);
			virtual OpenViBE::boolean process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_P300SpellerStimulator2);

		protected:

			OpenViBE::uint64 m_ui64StartStimulation;
			OpenViBE::uint64 m_ui64RowStimulationBase;
			OpenViBE::uint64 m_ui64ColumnStimulationBase;

			OpenViBE::uint64 m_ui64RowCount;
			OpenViBE::uint64 m_ui64ColumnCount;

			OpenViBE::uint64 m_ui64TotalTrials;
			OpenViBE::uint64 m_ui64FlashesPerTrial;

			OpenViBE::uint64 m_ui64TrialStartDuration;
			OpenViBE::uint64 m_ui64FlashDuration;
			OpenViBE::uint64 m_ui64AfterFlashDuration;
			OpenViBE::uint64 m_ui64RestDuration;

		private:

			OpenViBEToolkit::TStimulationDecoder<CBoxAlgorithmP300SpellerStimulator2> m_oStimulationDecoder;

			OpenViBEToolkit::TStimulationEncoder<CBoxAlgorithmP300SpellerStimulator2> m_oTimelineEncoder;
			OpenViBEToolkit::TFeatureVectorEncoder<CBoxAlgorithmP300SpellerStimulator2> m_oFlashGroupEncoder;
			OpenViBEToolkit::TFeatureVectorEncoder<CBoxAlgorithmP300SpellerStimulator2> m_oSequenceEncoder;

			OpenViBE::uint64 m_ui64LastTime;

			OpenViBE::uint64 m_ui64TrialCount;
			OpenViBE::uint64 m_ui64FlashCount;

			NextStimulatorState m_oNextState;
			OpenViBE::uint64 m_ui64NextStateTime;

			std::string m_sKeyboardConfig;
			std::string m_sTextToSpell;

			OpenViBE::CSV::ICSVHandler *m_pGroupHandler = NULL;
			OpenViBE::CSV::ICSVHandler *m_pSequenceHandler = NULL;

		private:

			bool sendTextToSpell(const std::string& sString, OpenViBE::uint64 timeToSend, OpenViBE::IStimulationSet& oSet, bool isWholeText);
			bool sendFlashParameters(OpenViBE::uint64 timeToSend, OpenViBE::IStimulationSet& oSet);
		};

		class CBoxAlgorithmP300SpellerStimulator2Desc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("P300 Speller Stimulator II"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Generates a stimulation sequence suitable for a P300 speller"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Stimulation"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-select-font"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_P300SpellerStimulator2; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Stimulation::CBoxAlgorithmP300SpellerStimulator2; }

			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput  ("Start stimulation",               OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addOutput ("Timeline",                        OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addOutput ("Flash groups",                    OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addOutput ("Flash sequence",                  OV_TypeId_FeatureVector);

				rBoxAlgorithmPrototype.addSetting("Start stimulation",               OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
				rBoxAlgorithmPrototype.addSetting("Row stimulation base",            OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
				rBoxAlgorithmPrototype.addSetting("Column stimulation base",         OV_TypeId_Stimulation, "OVTK_StimulationId_Label_08");

				rBoxAlgorithmPrototype.addSetting("Number of rows",                  OV_TypeId_Integer,     "6");
				rBoxAlgorithmPrototype.addSetting("Number of columns",               OV_TypeId_Integer,     "7");

				rBoxAlgorithmPrototype.addSetting("Number of flashes per letter",    OV_TypeId_Integer,     "68");
				rBoxAlgorithmPrototype.addSetting("Number of letters to spell",      OV_TypeId_Integer,     "5");
			//	rBoxAlgorithmPrototype.addSetting("Number of flashes per block",     OV_TypeId_Integer,     "68");

				rBoxAlgorithmPrototype.addSetting("Trial start duration (in sec)",   OV_TypeId_Float,       "1");
				rBoxAlgorithmPrototype.addSetting("Flash duration (in sec)",         OV_TypeId_Float,       "0.075");
				rBoxAlgorithmPrototype.addSetting("After flash duration (in sec)",   OV_TypeId_Float,       "0.125");
				rBoxAlgorithmPrototype.addSetting("Rest duration (in sec)",          OV_TypeId_Float,       "5");

				rBoxAlgorithmPrototype.addSetting("Flash group file",                OV_TypeId_Filename, "");
				rBoxAlgorithmPrototype.addSetting("Flash sequence type file",        OV_TypeId_Filename, "");

				rBoxAlgorithmPrototype.addSetting("Keyboard config",                 OV_TypeId_String, "ab#c#de#fghij#klm#nopq#rstu#vwxy#z_#.#,!?<");
				rBoxAlgorithmPrototype.addSetting("Text to spell",                   OV_TypeId_String, "quick_brown_fox");

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_P300SpellerStimulator2Desc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_P300SpellerStimulator2_H__
