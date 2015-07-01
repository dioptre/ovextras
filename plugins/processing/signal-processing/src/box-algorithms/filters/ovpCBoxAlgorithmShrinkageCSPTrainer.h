
#ifndef __OpenViBEPlugins_BoxAlgorithm_ShrinkageCSPTrainer_H__
#define __OpenViBEPlugins_BoxAlgorithm_ShrinkageCSPTrainer_H__

#if defined TARGET_HAS_ThirdPartyEIGEN

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "../../algorithms/basic/ovpCAlgorithmOnlineCovariance.h"

#define OVP_ClassId_BoxAlgorithm_ShrinkageCSPTrainer      OpenViBE::CIdentifier(0x2EC14CC0, 0x428C48BD)
#define OVP_ClassId_BoxAlgorithm_ShrinkageCSPTrainerDesc  OpenViBE::CIdentifier(0x02205F54, 0x733C51EE)

#include <Eigen/Eigenvalues>

typedef Eigen::Matrix< double , Eigen::Dynamic , Eigen::Dynamic, Eigen::RowMajor > MatrixXdRowMajor;

namespace OpenViBEPlugins
{
	namespace SignalProcessing
	{
		class CBoxAlgorithmShrinkageCSPTrainer : public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
		{
		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
			virtual OpenViBE::boolean process(void);

		protected:

			void dumpMatrix(OpenViBE::Kernel::ILogManager& rMgr, const MatrixXdRowMajor& mat, const OpenViBE::CString& desc);
			void dumpMatrixFile(const Eigen::MatrixXd &mat, const char *fn);
			void dumpVector(OpenViBE::Kernel::ILogManager& rMgr, const Eigen::VectorXd& mat, const OpenViBE::CString& desc);

			virtual OpenViBE::boolean updateCov(int index);

			OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmShrinkageCSPTrainer > m_oStimulationDecoder;
			OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmShrinkageCSPTrainer > m_oSignalDecoders[2];

			OpenViBEToolkit::TStimulationEncoder <CBoxAlgorithmShrinkageCSPTrainer > m_oStimulationEncoder;

			OpenViBE::uint64 m_ui64StimulationIdentifier;
			OpenViBE::CString m_sSpatialFilterConfigurationFilename;
			OpenViBE::uint64 m_ui64FilterDimension;
			// OpenViBE::float64 m_f64Shrinkage;

			OpenViBE::Kernel::IAlgorithmProxy* m_pIncrementalCov[2];

			OpenViBE::uint64 m_ui64nBuffers[2];
			OpenViBE::uint64 m_ui64nSamples[2];

			// OpenViBE::CMatrix m_oSignalTranspose[2];

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_ShrinkageCSPTrainer)
		};

#if 0
		class CBoxAlgorithmShrinkageCSPTrainerListener : public OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >
		{
		public:
			virtual OpenViBE::boolean onInitialized(OpenViBE::Kernel::IBox& rBox) { 

				rBox.addSetting("test", OV_TypeId_Stimulation, "1", rBox.getSettingCount());
				
				return true;
			};

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxListener < OpenViBE::Plugins::IBoxListener >, OV_UndefinedIdentifier)
		};
#endif

		class CBoxAlgorithmShrinkageCSPTrainerDesc : public OpenViBE::Plugins::IBoxAlgorithmDesc
		{
		public:

			virtual void release(void) { }
			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Shrinkage CSP Trainer"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Computes Common Spatial Pattern filter with regularized cov."); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Signal processing/Filtering"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.5"); }
			virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString(""); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_ShrinkageCSPTrainer; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SignalProcessing::CBoxAlgorithmShrinkageCSPTrainer; }

#if 0
			virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const               { return new CBoxAlgorithmShrinkageCSPTrainerListener; }
			virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) const { delete pBoxListener; }
#endif

			virtual OpenViBE::boolean getBoxPrototype(
				OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
			{
				rBoxAlgorithmPrototype.addInput  ("Stimulations",                 OV_TypeId_Stimulations);
				rBoxAlgorithmPrototype.addInput  ("Signal condition 1",           OV_TypeId_Signal);
				rBoxAlgorithmPrototype.addInput  ("Signal condition 2",           OV_TypeId_Signal);
								
				rBoxAlgorithmPrototype.addInputSupport(OV_TypeId_Signal);
				rBoxAlgorithmPrototype.addInputSupport(OV_TypeId_StreamedMatrix);

				rBoxAlgorithmPrototype.addSetting("Train Trigger",                OV_TypeId_Stimulation, "OVTK_GDF_End_Of_Session");
				rBoxAlgorithmPrototype.addSetting("Spatial filter configuration", OV_TypeId_Filename, "");
				rBoxAlgorithmPrototype.addSetting("Filter dimension",             OV_TypeId_Integer, "2");

				// Params of the cov algorithm; would be better to poll the params from the algorithm, however this is not straightforward to do
				rBoxAlgorithmPrototype.addSetting("Covariance method",            OVP_TypeId_OnlineCovariance_UpdateMethod, OVP_TypeId_OnlineCovariance_UpdateMethod_ChunkAverage.toString());   
				rBoxAlgorithmPrototype.addSetting("Shrinkage coefficient",        OpenViBE::Kernel::ParameterType_Float, "0.0");

				rBoxAlgorithmPrototype.addOutput ("Train-completed Flag",         OV_TypeId_Stimulations);

				rBoxAlgorithmPrototype.addFlag(OpenViBE::Kernel::BoxFlag_IsUnstable);

#if 0
				// Pull params from the cov alg
				this->
				rBoxAlgorithmPrototype.
				const CIdentifier l_oCovAlgId = ->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_OnlineCovariance);
				if(l_oCovAlgId == OV_UndefinedIdentifier)
				{
					this->getLogManager() << LogLevel_Error << "Unable to create the online cov algorithm\n";
					return false;
				}

				IAlgorithm:: &this->getAlgorithmManager().getAlgorithm(l_oCovAlgId);
				m_pIncrementalCov[i]->initialize();

				OpenViBE::CIdentifier l_oIdentifier = OV_UndefinedIdentifier;
				while((l_oIdentifier=m_pClassifier->getNextInputParameterIdentifier(l_oIdentifier))!=OV_UndefinedIdentifier)
#endif

//				IAlgorithmProto proto;
//				OpenViBEPlugins::SignalProcessing::CAlgorithmOnlineCovarianceDesc::getAlgorithmPrototype(proto);

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_ShrinkageCSPTrainerDesc);
		};
	};
};

#endif // TARGET_HAS_ThirdPartyEIGEN

#endif // __OpenViBEPlugins_BoxAlgorithm_XDAWNSpatialFilterTrainer_H__
