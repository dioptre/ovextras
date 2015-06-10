#ifndef __OpenViBEPlugins_BoxAlgorithm_ROCCurve_H__
#define __OpenViBEPlugins_BoxAlgorithm_ROCCurve_H__

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <set>
#include <map>
#include <iostream>

#define OVP_ClassId_BoxAlgorithm_ROCCurve OpenViBE::CIdentifier(0x06FE5B1B, 0xDE066FEC)
#define OVP_ClassId_BoxAlgorithm_ROCCurveDesc OpenViBE::CIdentifier(0xCB5DFCEA, 0xAF41EAB2)

namespace OpenViBEPlugins
{
	namespace Measurement
	{
		typedef struct{
			OpenViBE::uint64 m_ui64ExpectedClass;
			OpenViBE::uint64 m_ui64PredictedClass;
			OpenViBE::float64* m_f64ClassificationValues;
		}SROCClassificationInfo;


		typedef std::pair < OpenViBE::CIdentifier, OpenViBE::uint64 > CTimelineStimulationPair;
		typedef std::pair < OpenViBE::uint64, OpenViBE::float64* > CTimelineValuePair;
		typedef std::pair < OpenViBE::CIdentifier, OpenViBE::float64* > CLabelValuesPair;
		typedef std::pair < OpenViBE::float64, OpenViBE::float64 > CCoordinate;

		typedef std::pair < OpenViBE::uint32, OpenViBE::float64 > CRocPairValue;

		class CRocVectorBuilder{
		public:
			CRocVectorBuilder(std::vector < CRocPairValue >& rTargetVector, const OpenViBE::CIdentifier& rIdentifier, const OpenViBE::uint32& ui32Index):
				m_rTargetVector(rTargetVector),
				m_oClassLabel(rIdentifier),
				m_ui32Index(ui32Index),
				m_ui32AmountPositive(0)
			{
			}

			void operator() (CLabelValuesPair oLabelValuePair)
			{
				CRocPairValue l_oRocPairValue;
				l_oRocPairValue.first = (oLabelValuePair.first == m_oClassLabel)? 1:0;
				m_ui32AmountPositive += l_oRocPairValue.first;
				l_oRocPairValue.second = oLabelValuePair.second[m_ui32Index];
				m_rTargetVector.push_back(l_oRocPairValue);
			}

			OpenViBE::uint32 getPositiveCount(void) const
			{
				return m_ui32AmountPositive;
			}

		private:
			std::vector < CRocPairValue >& m_rTargetVector;
			OpenViBE::CIdentifier m_oClassLabel;
			OpenViBE::uint32 m_ui32Index;
			OpenViBE::uint32 m_ui32AmountPositive;
		};

		/**
		 * \class CBoxAlgorithmROCCurve
		 * \author Serrière Guillaume (Inria)
		 * \date Thu May 28 11:49:24 2015
		 * \brief The class CBoxAlgorithmROCCurve describes the box ROC curve.
		 *
		 */
		class CBoxAlgorithmROCCurve : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:
			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			
			virtual OpenViBE::boolean process(void);
			

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_ROCCurve)

		private:
			OpenViBE::boolean computeROCCurves();
			OpenViBE::boolean computeOneROCCurve(OpenViBE::CIdentifier rClassIdentifier, OpenViBE::uint32 ui32ClassIndex);

			// Input decoder:
			OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmROCCurve > m_oExpectedDecoder;
			OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmROCCurve > m_oClassificationValueDecoder;

			std::set < OpenViBE::CIdentifier > m_oClassStimulationSet;
			OpenViBE::CIdentifier m_oComputationTrigger;

			std::vector < CTimelineStimulationPair > m_oStimulationTimeline;
			std::vector < CTimelineValuePair > m_oValueTimeline;

			std::vector< CLabelValuesPair > m_oLabelValueList;

		};
		
		/**
		 * \class CBoxAlgorithmROCCurveDesc
		 * \author Serrière Guillaume (Inria)
		 * \date Thu May 28 11:49:24 2015
		 * \brief Descriptor of the box ROC curve.
		 *
		 */
		class CBoxAlgorithmROCCurveDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("ROC curve"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Serrière Guillaume"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Compute the ROC curve for each class."); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("The box computes the ROC curve for each class."); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Measurement"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.1"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-yes"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_ROCCurve; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Measurement::CBoxAlgorithmROCCurve; }
			
			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Expected label",OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput("Classification values",OV_TypeId_StreamedMatrix);

				rBoxAlgorithmPrototype.addSetting("Computation trigger", OV_TypeId_Stimulation, "");
				rBoxAlgorithmPrototype.addSetting("Class 1 identifier" , OV_TypeId_Stimulation, "");
				rBoxAlgorithmPrototype.addSetting("Class 2 identifier" , OV_TypeId_Stimulation, "");

				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);
				
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				
				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_ROCCurveDesc)
		};
	}
}

#endif // __OpenViBEPlugins_BoxAlgorithm_ROCCurve_H__
