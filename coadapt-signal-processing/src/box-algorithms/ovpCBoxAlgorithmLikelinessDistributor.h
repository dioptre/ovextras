#ifndef __OpenViBEPlugins_BoxAlgorithm_LikelinessDistributor_H__
#define __OpenViBEPlugins_BoxAlgorithm_LikelinessDistributor_H__

//You may have to change this path to match your folder organisation
#include "../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBEPlugins
{
	namespace SignalProcessingStatistics
	{
		/**
		 * \class CBoxAlgorithmLikelinessDistributor
		 * \author Dieter Devlaminck (INRIA)
		 * \date Mon Nov 12 10:06:07 2012
		 * \brief The class CBoxAlgorithmLikelinessDistributor describes the box Likeliness distributor.
		 *
		 */
		class CBoxAlgorithmLikelinessDistributor : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
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
			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_LikelinessDistributor);

		protected:
			// Codec algorithms specified in the skeleton-generator:
			// Streamed matrix stream encoder
			OpenViBEToolkit::TStreamedMatrixEncoder < CBoxAlgorithmLikelinessDistributor > m_oAlgo0_StreamedMatrixEncoder;
			// Streamed matrix stream decoder
			OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmLikelinessDistributor > m_oAlgo1_StreamedMatrixDecoder;

			OpenViBE::uint64 m_ui32Index;
			OpenViBE::uint64 m_ui32Number;
			OpenViBE::float64 m_f64Scale;

		};

		/**
		 * \class CBoxAlgorithmLikelinessDistributorDesc
		 * \author Dieter Devlaminck (INRIA)
		 * \date Mon Nov 12 10:06:07 2012
		 * \brief Descriptor of the box Likeliness distributor.
		 *
		 */
		class CBoxAlgorithmLikelinessDistributorDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Likeliness distributor"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Dieter Devlaminck"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Signal processing/Statistics"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString(""); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_LikelinessDistributor; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SignalProcessingStatistics::CBoxAlgorithmLikelinessDistributor; }
			
			/*
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmLikelinessDistributorListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
			*/
			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput("p-value",OV_TypeId_StreamedMatrix);

				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyInput);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddInput);
				
				rBoxAlgorithmPrototype.addOutput("p-value",OV_TypeId_StreamedMatrix);

				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifyOutput);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddOutput);
				
				rBoxAlgorithmPrototype.addSetting("index",OV_TypeId_Integer,"0");
				rBoxAlgorithmPrototype.addSetting("number",OV_TypeId_Integer,"1");
				rBoxAlgorithmPrototype.addSetting("scale",OV_TypeId_Float,"1");

				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanModifySetting);
				//rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_CanAddSetting);
				
				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);
				
				return true;
			}
			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_LikelinessDistributorDesc);
		};
	};
};

#endif // __OpenViBEPlugins_BoxAlgorithm_LikelinessDistributor_H__
