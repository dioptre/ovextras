#ifndef __OpenViBEPlugins_BoxAlgorithm_KappaCoefficient_H__
#define __OpenViBEPlugins_BoxAlgorithm_KappaCoefficient_H__

//You may have to change this path to match your folder organisation
#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <sstream>
#include <gtk/gtk.h>

#define OVP_ClassId_BoxAlgorithm_KappaCoefficient OpenViBE::CIdentifier         (0x160D8F1B, 0xD864C5BB)
#define OVP_ClassId_BoxAlgorithm_KappaCoefficientDesc OpenViBE::CIdentifier     (0xD8BA2199, 0xD252BECB)

namespace OpenViBEPlugins
{
	namespace Evaluation
	{

		/**
		 * \class CBoxAlgorithmKappaCoefficient
		 * \author Serrière Guillaume (Inria)
		 * \date Tue May  5 12:45:13 2015
		 * \brief The class CBoxAlgorithmKappaCoefficient describes the box Kappa coefficient.
		 *
		 */
		class CBoxAlgorithmKappaCoefficient : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:
			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
				
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::boolean process(void);
			
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_KappaCoefficient)

		protected:
				void updateKappaValue();

				OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmKappaCoefficient > m_oTargetStimulationDecoder;
				OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmKappaCoefficient > m_oClassifierStimulationDecoder;

				OpenViBEToolkit::TStreamedMatrixEncoder < CBoxAlgorithmKappaCoefficient > m_oOutputMatrixEncoder;

				OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > op_pConfusionMatrix;

				OpenViBE::Kernel::IAlgorithmProxy* m_pConfusionMatrixAlgorithm;

				OpenViBE::uint32 m_ui32AmountClass;
				OpenViBE::uint64 m_ui64CurrentProcessingTimeLimit;
				OpenViBE::float64 m_f64KappaCoefficient;

				::GtkWidget* m_pKappaLabel;
		};
		
		
		// The box listener can be used to call specific callbacks whenever the box structure changes : input added, name changed, etc.
		// Please uncomment below the callbacks you want to use.
		class CBoxAlgorithmKappaCoefficientListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:

			virtual OpenViBE::boolean onSettingValueChanged(OpenViBE::Kernel::IBox& rBox, const OpenViBE::uint32 ui32Index) {
				if(ui32Index == 0)
				{
					OpenViBE::CString l_sClassCount;
					rBox.getSettingValue(ui32Index, l_sClassCount);

					if(l_sClassCount.length() == 0 )
					{
						return true;
					}

					OpenViBE::int32 l_i32SettingCount;
					std::stringstream l_sStream(l_sClassCount.toASCIIString());
					l_sStream >> l_i32SettingCount;

					//First of all we prevent for the value to goes under 1.
					if(l_i32SettingCount < 1)
					{
						rBox.setSettingValue(ui32Index, "1");
						l_i32SettingCount = 1;
					}
					OpenViBE::int32 l_i32CurrentCount = rBox.getSettingCount()-1;
					//We have two choice 1/We need to add class, 2/We need to remove some
					if(l_i32CurrentCount < l_i32SettingCount)
					{
						while(l_i32CurrentCount < l_i32SettingCount)
						{
							std::stringstream l_sSettingName;
							l_sSettingName << "Stimulation of class " << (l_i32CurrentCount+1);
							rBox.addSetting(l_sSettingName.str().c_str(), OVTK_TypeId_Stimulation, "");
							++l_i32CurrentCount;
						}
					}
					else
					{
						while(l_i32CurrentCount > l_i32SettingCount)
						{
							rBox.removeSetting(rBox.getSettingCount()-1);
							--l_i32CurrentCount;
						}
					}
				}
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier)
		};

		/**
		 * \class CBoxAlgorithmKappaCoefficientDesc
		 * \author Serrière Guillaume (Inria)
		 * \date Tue May  5 12:45:13 2015
		 * \brief Descriptor of the box Kappa coefficient.
		 *
		 */
		class CBoxAlgorithmKappaCoefficientDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Kappa coefficient"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Serrière Guillaume"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Compute the kappa coefficient for the classifier."); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("The box computes kappa coefficient for a classifier."); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Evaluation/Classification"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.1"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-yes"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_KappaCoefficient; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Evaluation::CBoxAlgorithmKappaCoefficient; }
			
			
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmKappaCoefficientListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }

			virtual OpenViBE::boolean hasFunctionality(OpenViBE::Kernel::EPluginFunctionality ePluginFunctionality) const
			{
				return ePluginFunctionality == OpenViBE::Kernel::PluginFunctionality_Visualization;
			}

			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Expected stimulations",       OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput("Found stimulations",          OV_TypeId_Stimulations);

				rBoxAlgorithmPrototype.addOutput ("Confusion Matrix",          OV_TypeId_StreamedMatrix);

				rBoxAlgorithmPrototype.addSetting("Amount of class",           OV_TypeId_Integer,     "2");
				rBoxAlgorithmPrototype.addSetting("Stimulation of class 1",    OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
				rBoxAlgorithmPrototype.addSetting("Stimulation of class 2",    OV_TypeId_Stimulation, "OVTK_StimulationId_Label_02");

				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				
				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_KappaCoefficientDesc)
		};
	}
}

#endif // __OpenViBEPlugins_BoxAlgorithm_KappaCoefficient_H__
