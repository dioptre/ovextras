#ifndef __OpenViBEPlugins_BoxAlgorithm_AngleSpeed_H__
#define __OpenViBEPlugins_BoxAlgorithm_AngleSpeed_H__

//You may have to change this path to match your folder organisation
#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <vector>

// The unique identifiers for the box and its descriptor.
// Identifier are randomly chosen by the skeleton-generator.
#define OVP_ClassId_BoxAlgorithm_AngleSpeed OpenViBE::CIdentifier(0xBC1CC0FE, 0x1098054C)
#define OVP_ClassId_BoxAlgorithm_AngleSpeedDesc OpenViBE::CIdentifier(0xEA9ABFC9, 0x0F5C8608)

namespace OpenViBEPlugins
{
	namespace Tools
	{
		/**
		 * \class CBoxAlgorithmAngleSpeed
		 * \author Alison Cellard (Inria)
		 * \date Mon Mar 24 17:28:19 2014
		 * \brief The class CBoxAlgorithmAngleSpeed describes the box Angle and Speed.
		 *
		 */
		class CBoxAlgorithmAngleSpeed : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:
			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
				
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			
			virtual OpenViBE::boolean process(void);
			
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_AngleSpeed);

		protected:
			// Codec algorithms specified in the skeleton-generator:
			// Feature vector stream encoder
			OpenViBEToolkit::TFeatureVectorEncoder < CBoxAlgorithmAngleSpeed > m_oAngle_FeatureVectorEncoder;
			OpenViBEToolkit::TFeatureVectorEncoder < CBoxAlgorithmAngleSpeed > m_oLength_FeatureVectorEncoder;
			OpenViBEToolkit::TFeatureVectorEncoder < CBoxAlgorithmAngleSpeed > m_oVector_FeatureVectorEncoder;
			OpenViBEToolkit::TFeatureVectorEncoder < CBoxAlgorithmAngleSpeed > m_oHistoAngle_FeatureVectorEncoder;
			OpenViBEToolkit::TSignalEncoder < CBoxAlgorithmAngleSpeed > m_oVelocity_SignalEncoder;
			OpenViBEToolkit::TSignalEncoder < CBoxAlgorithmAngleSpeed > m_oAcceleration_SignalEncoder;

			// Signal stream decoder
			OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmAngleSpeed > m_oInputSignalDecoder;

                        OpenViBE::IMatrix* m_pAnglesMatrix;
                        OpenViBE::IMatrix* m_pLengthMatrix;
                        OpenViBE::IMatrix* m_pVectorMatrix;
                        OpenViBE::IMatrix* m_pVelocityMatrix;
                        OpenViBE::IMatrix* m_pAccelerationMatrix;
                         OpenViBE::IMatrix* m_pHistoMatrix;

/*			std::vector<float64> l_f64Vec1_x;
			std::vector<float64> l_f64Vec1_y;
			std::vector<float64> l_f64Vec2_x;
			std::vector<float64> l_f64Vec2_y;

                        std::vector<float64> l_f64NormVec1;
                        std::vector<float64> l_f64NormVec2;*/

                        std::vector<OpenViBE::float64> m_oTheta;

                        OpenViBE::float64 m_f64Acc;
                        OpenViBE::float64 m_f64Vit;

                        std::vector<OpenViBE::uint32> m_AngleBin1;
  /*                      OpenViBE::uint32 m_AngleBin2;
                        OpenViBE::uint32 m_AngleBin3;
                        OpenViBE::uint32 m_AngleBin4;
                        OpenViBE::uint32 m_AngleBin5;*/

		};

		
		
		// The box listener can be used to call specific callbacks whenever the box structure changes : input added, name changed, etc.
		// Please uncomment below the callbacks you want to use.
		class CBoxAlgorithmAngleSpeedListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);
		};
		

		/**
		 * \class CBoxAlgorithmAngleSpeedDesc
		 * \author Alison Cellard (Inria)
		 * \date Mon Mar 24 17:28:19 2014
		 * \brief Descriptor of the box Angle and Speed.
		 *
		 */
		class CBoxAlgorithmAngleSpeedDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Angle and Speed"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Alison Cellard"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Calculate angle and speed"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Calculate angle and speed of gesture from mouse coordinates"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Tools"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-page-setup"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_AngleSpeed; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Tools::CBoxAlgorithmAngleSpeed; }
			
			
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmAngleSpeedListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
			
			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Coordinates",OV_TypeId_Signal);
				
				rBoxAlgorithmPrototype.addOutput("Angles",OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addOutput("Length", OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addOutput("Vector", OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addOutput("Velocity",OV_TypeId_Signal);
				rBoxAlgorithmPrototype.addOutput("Acceleration", OV_TypeId_Signal);
				rBoxAlgorithmPrototype.addOutput("Histogramme d'angle", OV_TypeId_FeatureVector);

				
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				
				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_AngleSpeedDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_AngleSpeed_H__
