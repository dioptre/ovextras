#ifndef __OpenViBEPlugins_BoxAlgorithm_DynamicVotingClassifier_H__
#define __OpenViBEPlugins_BoxAlgorithm_DynamicVotingClassifier_H__

//You may have to change this path to match your folder organisation
#include "../../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <itpp/itbase.h>

#include <map>
#include <stack>
#include <iostream>
#include <set>

#include <xml/IReader.h>

// The unique identifiers for the box and its descriptor.
// Identifier are randomly chosen by the skeleton-generator.
#define OVP_ClassId_BoxAlgorithm_DynamicVotingClassifier OpenViBE::CIdentifier(0x3DCBA7F3, 0x0F606C35)
#define OVP_ClassId_BoxAlgorithm_DynamicVotingClassifierDesc OpenViBE::CIdentifier(0x279850DD, 0x3AD029A3)

namespace OpenViBEPlugins
{
	namespace Classification
	{
		/**
		 * \class CBoxAlgorithmRepetitionManager
		 * \author Loïc MAHE (ECM)
		 * \date Fri May 18 11:50:42 2012
		 * \brief The class CBoxAlgorithmRepetitionManager describes the box Repetition Manager.
		 *
		 */
		class CBoxAlgorithmDynamicVotingClassifier : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, public XML::IReaderCallback
		{
		public:
			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
				
			//Here is the different process callbacks possible
			// - On clock ticks :
			//virtual OpenViBE::boolean processClock(OpenViBE::CMessageClock& rMessageClock);
			// - On new input received (the most common behaviour for signal processing) :
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			
			// If you want to use processClock, you must provide the clock frequency.
			//virtual OpenViBE::uint64 getClockFrequency(void);
			
			virtual OpenViBE::boolean process(void);

			virtual OpenViBE::boolean loadConfiguration(const OpenViBE::IMemoryBuffer& rMemoryBuffer);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_DynamicVotingClassifier);

		protected:

			virtual void openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount); // XML IReaderCallback
			virtual void processChildData(const char* sData); // XML IReaderCallback
			virtual void closeChild(void); // XML ReaderCallback

			std::stack<OpenViBE::CString> m_vNode;

			//splotch
			std::map <OpenViBE::uint32,std::set<int> > m_mLettersGroups;
			OpenViBE::uint32 m_ui32LetterGroupId;

			// Codec algorithms specified in the skeleton-generator:
			// Stimulation stream encoder
			OpenViBEToolkit::TStimulationEncoder < CBoxAlgorithmDynamicVotingClassifier > m_oAlgo0_StimulationEncoder;
			OpenViBEToolkit::TStimulationEncoder < CBoxAlgorithmDynamicVotingClassifier > m_StimulationEncoderGroup1;
			OpenViBEToolkit::TStimulationEncoder < CBoxAlgorithmDynamicVotingClassifier > m_StimulationEncoderGroup2;
			OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmDynamicVotingClassifier > m_oAlgo0_StimulationDecoder;
			// Streamed matrix stream decoder
			OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmDynamicVotingClassifier > m_oAlgo1_StreamedMatrixDecoder;

			itpp::vec m_vCumulativeScore;
			itpp::vec m_vCurrentScore;
			itpp::vec m_vPiLetterScore;

			OpenViBE::float64 m_f64Threeshold;
			OpenViBE::float64 m_f64Entropy;
			OpenViBE::uint32 m_ui32Dimension;
			OpenViBE::uint32 m_ui32NumberOfSymbol;
			std::map <OpenViBE::uint32, OpenViBE::uint32> m_mCheckedEntries;
			OpenViBE::uint32 m_ui32CheckedEntries;
			OpenViBE::uint32 m_ui32MinimalRepetition;
			OpenViBE::uint32 m_ui32CurrentRepetition;
			OpenViBE::uint64 m_ui32MaximalRepetition;
			OpenViBE::uint64 m_ui64ClassLabelBaseGroup1;
			OpenViBE::uint64 m_ui64ClassLabelBaseGroup2;
			OpenViBE::boolean m_bSkipRepetition;

		};

		class CBoxAlgorithmDynamicVotingClassifierListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			OpenViBE::boolean check(OpenViBE::Kernel::IBox& rBox)
			{
				char l_sName[1024];
				OpenViBE::uint32 i;
				for(i=0; i<rBox.getInputCount(); i++)
				{
					sprintf(l_sName, "Input stream %u", i);
					rBox.setInputName(i, l_sName);
				}
				for(i=0; i<rBox.getOutputCount(); i++)
				{
					sprintf(l_sName, "Output stream %u", i);
					rBox.setOutputName(i, l_sName);
				}
				return true;
			}

			virtual OpenViBE::boolean onInputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				rBox.setInputType(ui32Index, OVTK_TypeId_StreamedMatrix);
				this->check(rBox);
				return true;
			}

			/*virtual OpenViBE::boolean onInputRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				rBox.removeOutput(ui32Index);
				this->check(rBox);
				return true;
			}

			virtual OpenViBE::boolean onInputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				OpenViBE::CIdentifier l_oTypeIdentifier;
				rBox.getInputType(ui32Index, l_oTypeIdentifier);
				return true;
			}

			virtual OpenViBE::boolean onOutputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				rBox.addInput("", OV_UndefinedIdentifier);
				this->check(rBox);
				return true;
			}

			virtual OpenViBE::boolean onOutputRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				rBox.removeInput(ui32Index);
				this->check(rBox);
				return true;
			}

			virtual OpenViBE::boolean onOutputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
			{
				OpenViBE::CIdentifier l_oTypeIdentifier;
				rBox.getOutputType(ui32Index, l_oTypeIdentifier);
				rBox.setInputType(ui32Index-2, l_oTypeIdentifier);
				return true;
			};*/

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);
		};
		// If you need to implement a box Listener, here is a sekeleton for you.
		// Use only the callbacks you need.
		// For example, if your box has a variable number of input, but all of them must be stimulation inputs.
		// The following listener callback will ensure that any newly added input is stimulations :
		/*		
		virtual OpenViBE::boolean onInputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index)
		{
			rBox.setInputType(ui32Index, OV_TypeId_Stimulations);
		};
		*/
		
		/*
		// The box listener can be used to call specific callbacks whenever the box structure changes : input added, name changed, etc.
		// Please uncomment below the callbacks you want to use.
		class CBoxAlgorithmRepetitionManagerListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			//virtual OpenViBE::boolean onInitialized(OpenViBE::Kernel::IBox& rBox) { return true; };
			//virtual OpenViBE::boolean onNameChanged(OpenViBE::Kernel::IBox& rBox) { return true; };
			//virtual OpenViBE::boolean onInputConnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onInputDisconnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onInputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onInputRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onInputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onInputNameChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputConnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputDisconnected(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onOutputNameChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onSettingAdded(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onSettingRemoved(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onSettingTypeChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onSettingNameChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onSettingDefaultValueChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };
			//virtual OpenViBE::boolean onSettingValueChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) { return true; };

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);
		};
		*/

		/**
		 * \class CBoxAlgorithmRepetitionManagerDesc
		 * \author Loïc MAHE (ECM)
		 * \date Fri May 18 11:50:42 2012
		 * \brief Descriptor of the box Repetition Manager.
		 *
		 */
		class CBoxAlgorithmDynamicVotingClassifierDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Dynamic Voting Classifier"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Loïc MAHE"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("ECM"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("The box only makes a choice when it is certain enough of its decision"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Classification"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-zoom-out"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_DynamicVotingClassifier; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CBoxAlgorithmDynamicVotingClassifier; }
			
			//*
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmDynamicVotingClassifierListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
			//*/
			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Reset",OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput("Classification Result",OV_TypeId_StreamedMatrix);

				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyInput);
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddInput);
				
				rBoxAlgorithmPrototype.addOutput("End of repetitions",OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addOutput ("Choice group 1",   OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addOutput ("Choice group 2",   OV_TypeId_Stimulations);
				//rBoxAlgorithmPrototype.addOutput("Classification Result",OV_TypeId_StreamedMatrix);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyOutput);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddOutput);
				
				//No setting specified.To add settings use :
//rBoxAlgorithmPrototype.addSetting("Setting Name",OV_TypeId_XXXX,"default value");
				rBoxAlgorithmPrototype.addSetting("Threshold",OV_TypeId_Float,"0");
				rBoxAlgorithmPrototype.addSetting("Class label base for group 1", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
				rBoxAlgorithmPrototype.addSetting("Class label base for group 2", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_07");
				rBoxAlgorithmPrototype.addSetting("Filename to load group of letters from",    OV_TypeId_Filename,                  "");
				rBoxAlgorithmPrototype.addSetting("Number of repetitions",OV_TypeId_Integer,"12");
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddSetting);
				
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				
				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_DynamicVotingClassifierDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_DynamicVotingClassifier_H__
