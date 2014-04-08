#ifndef __OpenViBEPlugins_BoxAlgorithm_MotionQuantities_H__
#define __OpenViBEPlugins_BoxAlgorithm_MotionQuantities_H__

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <vector>

#define OVP_ClassId_BoxAlgorithm_MotionQuantities OpenViBE::CIdentifier(0xBC1CC0FE, 0x1098054C)
#define OVP_ClassId_BoxAlgorithm_MotionQuantitiesDesc OpenViBE::CIdentifier(0xEA9ABFC9, 0x0F5C8608)

namespace OpenViBEPlugins
{
	namespace Tools
	{
		/**
		 * \class CBoxAlgorithmMotionQuantities
		 * \author Alison Cellard (Inria)
		 * \date Mon Mar 24 17:28:19 2014
		 * \brief The class CBoxAlgorithmMotionQuantities describes the box Angle and Speed.
		 *
		 */
		class CBoxAlgorithmMotionQuantities : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:
			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
				
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			
			virtual OpenViBE::boolean process(void);
			
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_MotionQuantities);

		protected:

			// Feature vector stream encoder
			OpenViBEToolkit::TFeatureVectorEncoder < CBoxAlgorithmMotionQuantities > m_oAngle_FeatureVectorEncoder;
			OpenViBEToolkit::TFeatureVectorEncoder < CBoxAlgorithmMotionQuantities > m_oHistoAngle_FeatureVectorEncoder;
			OpenViBEToolkit::TFeatureVectorEncoder < CBoxAlgorithmMotionQuantities > m_oLength_FeatureVectorEncoder;

			// Signal stream encoder
			OpenViBEToolkit::TSignalEncoder < CBoxAlgorithmMotionQuantities > m_oVelocity_SignalEncoder;
			OpenViBEToolkit::TSignalEncoder < CBoxAlgorithmMotionQuantities > m_oAcceleration_SignalEncoder;

			OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmMotionQuantities > m_oInputSignalDecoder;

                        OpenViBE::IMatrix* m_pAnglesMatrix;
                        OpenViBE::IMatrix* m_pLengthMatrix;
                        OpenViBE::IMatrix* m_pVelocityMatrix;
                        OpenViBE::IMatrix* m_pAccelerationMatrix;
                        OpenViBE::IMatrix* m_pHistoMatrix;

                        // Angle formed by the two vector starting from current point to previous and next point of mouse trajectory
                        std::vector<OpenViBE::float64> m_oTheta;

                        // Histogram of successive angles
                        std::vector<OpenViBE::uint32> m_AngleBin;

		};		
		
		class CBoxAlgorithmMotionQuantitiesListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier);
		};
		

		/**
		 * \class CBoxAlgorithmMotionQuantitiesDesc
		 * \author Alison Cellard (Inria)
		 * \date Mon Mar 24 17:28:19 2014
		 * \brief Descriptor of the box Angle and Speed.
		 *
		 */
		class CBoxAlgorithmMotionQuantitiesDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Motion quantities"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Alison Cellard"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Calculate angles, speed and acceleration of gesture from mouse coordinates"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Calculate angles, angle histogram, length between two points, speed, and accelaration of mouse trajectory"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Tools"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-page-setup"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_MotionQuantities; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Tools::CBoxAlgorithmMotionQuantities; }
			
			
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmMotionQuantitiesListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
			
			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Coordinates",OV_TypeId_Signal);
				
				rBoxAlgorithmPrototype.addOutput("Angles",OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addOutput("Histogramme d'angle", OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addOutput("Length", OV_TypeId_FeatureVector);
				rBoxAlgorithmPrototype.addOutput("Velocity",OV_TypeId_Signal);
				rBoxAlgorithmPrototype.addOutput("Acceleration", OV_TypeId_Signal);
				
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				
				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_MotionQuantitiesDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_MotionQuantities_H__
