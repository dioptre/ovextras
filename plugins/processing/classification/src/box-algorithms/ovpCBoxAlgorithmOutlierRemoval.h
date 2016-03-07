#ifndef __OpenViBEPlugins_BoxAlgorithm_OutlierRemoval_H__
#define __OpenViBEPlugins_BoxAlgorithm_OutlierRemoval_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <vector>
#include <map>

#define OVP_ClassId_BoxAlgorithm_OutlierRemovalDesc    OpenViBE::CIdentifier(0x11DA1C24, 0x4C7A74C0)
#define OVP_ClassId_BoxAlgorithm_OutlierRemoval        OpenViBE::CIdentifier(0x09E41B92, 0x4291B612)

namespace OpenViBEPlugins
{
	namespace Classification
	{
		class CBoxAlgorithmOutlierRemoval : public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::boolean process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_OutlierRemoval);

		protected:

			typedef struct
			{
				OpenViBE::CMatrix* m_pFeatureVectorMatrix;
				OpenViBE::uint64 m_ui64StartTime;
				OpenViBE::uint64 m_ui64EndTime;
			} SFeatureVector;

			OpenViBE::boolean pruneSet( std::vector<CBoxAlgorithmOutlierRemoval::SFeatureVector>& l_vPruned);

	        OpenViBEToolkit::TFeatureVectorDecoder< CBoxAlgorithmOutlierRemoval > m_oFeatureVectorDecoder;
	    	OpenViBEToolkit::TStimulationDecoder< CBoxAlgorithmOutlierRemoval > m_oStimulationDecoder;
        
			OpenViBEToolkit::TFeatureVectorEncoder< CBoxAlgorithmOutlierRemoval > m_oFeatureVectorEncoder;
	    	OpenViBEToolkit::TStimulationEncoder< CBoxAlgorithmOutlierRemoval > m_oStimulationEncoder;

			std::vector < CBoxAlgorithmOutlierRemoval::SFeatureVector > m_vDataset;

			OpenViBE::float64 m_f64LowerQuantile;
			OpenViBE::float64 m_f64UpperQuantile;
			OpenViBE::uint64 m_ui64Trigger;
			OpenViBE::uint64 m_ui64TriggerTime;

		};

		class CBoxAlgorithmOutlierRemovalDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Outlier removal"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Discards feature vectors with extremal values"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Simple outlier removal based on quantile estimation"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Classification"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_OutlierRemoval; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CBoxAlgorithmOutlierRemoval; }
			virtual OpenViBE::CString getStockItemName(void) const       { return "gtk-cut"; }

			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Input stimulations",              OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput("Input features",                  OV_TypeId_FeatureVector);

				rBoxAlgorithmPrototype.addOutput("Output stimulations",            OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addOutput("Output features",                OV_TypeId_FeatureVector);

				rBoxAlgorithmPrototype.addSetting("Lower quantile",   OV_TypeId_Float,       "0.01");
				rBoxAlgorithmPrototype.addSetting("Upper quantile",   OV_TypeId_Float,       "0.99");
				rBoxAlgorithmPrototype.addSetting("Start trigger",    OV_TypeId_Stimulation, "OVTK_StimulationId_Train");

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_OutlierRemovalDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_OutlierRemoval_H__
