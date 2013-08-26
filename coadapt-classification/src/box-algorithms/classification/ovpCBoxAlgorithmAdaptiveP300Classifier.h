#ifndef __OpenViBEPlugins_BoxAlgorithm_AdaptiveP300Classifier_H__
#define __OpenViBEPlugins_BoxAlgorithm_AdaptiveP300Classifier_H__

//You may have to change this path to match your folder organisation
#include "../../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

// The unique identifiers for the box and its descriptor.
// Identifier are randomly chosen by the skeleton-generator.

namespace OpenViBEPlugins
{
	namespace Classification
	{
		/**
		 * \class CBoxAlgorithmAdaptiveP300Classifier
		 * \author Dieter Devlaminck (INRIA)
		 * \date Mon Aug 26 10:26:08 2013
		 * \brief The class CBoxAlgorithmAdaptiveP300Classifier describes the box AdaptiveP300Classifier.
		 *
		 */
		class CBoxAlgorithmAdaptiveP300Classifier : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
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
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_AdaptiveP300Classifier);

		protected:
			// Codec algorithms specified in the skeleton-generator:
			// Feature vector stream decoder
			OpenViBEToolkit::TFeatureVectorDecoder < CBoxAlgorithmAdaptiveP300Classifier > m_oAlgo0_FeatureVectorDecoder;
			// Stimulation stream decoder
			OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmAdaptiveP300Classifier > m_oAlgo1_StimulationDecoder;
			// Streamed matrix stream encoder
			OpenViBEToolkit::TStreamedMatrixEncoder < CBoxAlgorithmAdaptiveP300Classifier > m_oAlgo2_StreamedMatrixEncoder;

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
		class CBoxAlgorithmAdaptiveP300ClassifierListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
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
		 * \class CBoxAlgorithmAdaptiveP300ClassifierDesc
		 * \author Dieter Devlaminck (INRIA)
		 * \date Mon Aug 26 10:26:08 2013
		 * \brief Descriptor of the box AdaptiveP300Classifier.
		 *
		 */
		class CBoxAlgorithmAdaptiveP300ClassifierDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("AdaptiveP300Classifier"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Dieter Devlaminck"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Classification"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString(""); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_AdaptiveP300Classifier; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CBoxAlgorithmAdaptiveP300Classifier; }
			
			/*
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmAdaptiveP300ClassifierListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
			*/
			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Flash vectors",OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput("groups",OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addInput("label",OV_TypeId_Stimulations);

				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyInput);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddInput);
				
				rBoxAlgorithmPrototype.addOutput("prediction",OV_TypeId_StreamedMatrix);

				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyOutput);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddOutput);
				
				rBoxAlgorithmPrototype.addSetting("eta",OV_TypeId_Float,"0.03");
				rBoxAlgorithmPrototype.addSetting("lambda",OV_TypeId_Float,"0.0001");

				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddSetting);
				
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				
				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_AdaptiveP300ClassifierDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_AdaptiveP300Classifier_H__
