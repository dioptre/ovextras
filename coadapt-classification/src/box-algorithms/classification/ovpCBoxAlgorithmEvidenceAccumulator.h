#ifndef __OpenViBEPlugins_BoxAlgorithm_EvidenceAccumulator_H__
#define __OpenViBEPlugins_BoxAlgorithm_EvidenceAccumulator_H__

//You may have to change this path to match your folder organisation
#include "../../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBEPlugins
{
	namespace Classification
	{
		/**
		 * \class CBoxAlgorithmEvidenceAccumulator
		 * \author Dieter Devlaminck (INRIA)
		 * \date Fri Mar 15 12:45:51 2013
		 * \brief The class CBoxAlgorithmEvidenceAccumulator describes the box EvidenceAccumulator.
		 *
		 */
		class CBoxAlgorithmEvidenceAccumulator : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
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

			// As we do with any class in openvibe, we use the macro below 
			// to associate this box to an unique identifier. 
			// The inheritance information is also made available, 
			// as we provide the superclass OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_EvidenceAccumulator);

		protected:
			// Codec algorithms specified in the skeleton-generator:
			// Stimulation stream decoder
			OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmEvidenceAccumulator > m_oAlgo0_StimulationDecoder;
			// Feature vector stream decoder
			OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmEvidenceAccumulator > m_oAlgo1_FeatureVectorDecoder;
			// Stimulation stream encoder
			OpenViBEToolkit::TStimulationEncoder < CBoxAlgorithmEvidenceAccumulator > m_oAlgo2_StimulationEncoder;
			// Feature vector stream encoder
			OpenViBEToolkit::TStreamedMatrixEncoder < CBoxAlgorithmEvidenceAccumulator > m_oAlgo3_FeatureVectorEncoder;
			
			OpenViBE::uint64 m_pResetTrigger;
			OpenViBE::uint64 m_pStimulationBase;
			OpenViBE::IMatrix* m_pAccumulatedEvidence;
			OpenViBE::IMatrix* m_pNormalizedAccumulatedEvidence;
			OpenViBE::CIdentifier m_oEvidenceAlgorithm;
			OpenViBE::CIdentifier m_oInputType;

			
		private:
			//void findMaximum(OpenViBE::uint32& l_ui32MaximumIndex, OpenViBE::float32& l_f32Maximum);
			void findMaximum(OpenViBE::float64* vector, OpenViBE::uint32& l_ui32MaximumIndex, OpenViBE::float32& l_f32Maximum);
			
			OpenViBE::uint32 m_ui32MaximumIndex;
			OpenViBE::boolean m_bReceivedSomeEvidence;
			OpenViBE::boolean m_bEarlyStoppingConditionMet;
			OpenViBE::boolean m_bEarlyStoppingEnabled;
			OpenViBE::float64 m_bStopCondition;
			OpenViBE::float64 m_bScaleFactor;

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
		
		
		// The box listener can be used to call specific callbacks whenever the box structure changes : input added, name changed, etc.
		// Please uncomment below the callbacks you want to use.
		class CBoxAlgorithmEvidenceAccumulatorListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
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
		

		/**
		 * \class CBoxAlgorithmEvidenceAccumulatorDesc
		 * \author Dieter Devlaminck (INRIA)
		 * \date Fri Mar 15 12:45:51 2013
		 * \brief Descriptor of the box EvidenceAccumulator.
		 *
		 */
		class CBoxAlgorithmEvidenceAccumulatorDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("EvidenceAccumulator"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Dieter Devlaminck"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("accumulates evidence for each class"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("accumulates evidence for each class (feature vector inputs represent evidence for each class)\nbased on a stimulus input the evidence can be reset for the next trial"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Classification"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString(""); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_EvidenceAccumulator; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CBoxAlgorithmEvidenceAccumulator; }
			
			
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmEvidenceAccumulatorListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
			
			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Trigger",OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput("Evidence",OV_TypeId_StreamedMatrix);

				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyInput);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddInput);
				
				rBoxAlgorithmPrototype.addOutput("Class label",OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addOutput("AccumulatedEvidence",OV_TypeId_FeatureVector);

				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyOutput);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddOutput);
				
				rBoxAlgorithmPrototype.addSetting("ActivationTrigger",OV_TypeId_Stimulation,"");
				rBoxAlgorithmPrototype.addSetting("EvidenceAlgorithm",OVP_TypeId_EvidenceAccumulationAlgorithm,"");
				rBoxAlgorithmPrototype.addSetting("StimulationLabelBase",OV_TypeId_Stimulation,"");
				rBoxAlgorithmPrototype.addSetting("EarlyStopping",OV_TypeId_Boolean,"false");
				rBoxAlgorithmPrototype.addSetting("StopCondition",OV_TypeId_Float,"0.0");
				rBoxAlgorithmPrototype.addSetting("InputType",OVP_TypeId_EvidenceAccumulationAlgorithmInputType,"Distance to hyperplane");

				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddSetting);
				
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				
				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_EvidenceAccumulatorDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_EvidenceAccumulator_H__
