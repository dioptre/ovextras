#ifndef __OpenViBEPlugins_BoxAlgorithm_StatisticGenerator_H__
#define __OpenViBEPlugins_BoxAlgorithm_StatisticGenerator_H__

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <map>

#define OVP_ClassId_BoxAlgorithm_StatisticGenerator OpenViBE::CIdentifier(0x83EDA40B, 0x425FBFFE)
#define OVP_ClassId_BoxAlgorithm_StatisticGeneratorDesc OpenViBE::CIdentifier(0x35A0CB63, 0x78882C28)

namespace OpenViBEPlugins
{
	namespace Measurement
	{
		/**
		 * \class CBoxAlgorithmStatisticGenerator
		 * \author Serrière Guillaume (Inria)
		 * \date Thu Apr 30 15:24:39 2015
		 * \brief The class CBoxAlgorithmStatisticGenerator describes the box Statistic generator.
		 *
		 */
		class CBoxAlgorithmStatisticGenerator : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:
			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
				
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::boolean process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_StatisticGenerator);

		private:
			// Input decoder:
			OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmStatisticGenerator > m_oSignalDecoder;
			OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmStatisticGenerator > m_oStimulationDecoder;

			OpenViBE::uint32 m_ui32AmountChannel;
			OpenViBE::CString m_oFilename;
			std::map < OpenViBE::CIdentifier, OpenViBE::uint32> m_oStimulationMap;

			OpenViBE::boolean m_bHasBeenStreamed;
		};


		/**
		 * \class CBoxAlgorithmStatisticGeneratorDesc
		 * \author Serrière Guillaume (Inria)
		 * \date Thu Apr 30 15:24:39 2015
		 * \brief Descriptor of the box Statistic generator.
		 *
		 */
		class CBoxAlgorithmStatisticGeneratorDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Statistic generator"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Serrière Guillaume"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Generate statistic on signal."); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Generate some general purpose statistic on signal and store them in a file. This box is required for some measurement tools."); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Measurement"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.1"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-yes"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_StatisticGenerator; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Measurement::CBoxAlgorithmStatisticGenerator; }
			
			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("Signal",OV_TypeId_Signal);
				rBoxAlgorithmPrototype.addInput("Stimulations",OV_TypeId_Stimulations);

				rBoxAlgorithmPrototype.addSetting("Filename for saving",OV_TypeId_Filename,"");

				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);
				
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				
				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_StatisticGeneratorDesc)
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_StatisticGenerator_H__
