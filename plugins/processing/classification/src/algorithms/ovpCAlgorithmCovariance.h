#if defined TARGET_HAS_ThirdPartyEIGEN

#ifndef __OpenViBEPlugins_Algorithm_Covariance_H__
#define __OpenViBEPlugins_Algorithm_Covariance_H__

// See e.g. Ledoit & Wolf for paper

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <Eigen/Dense> 

#define OVP_ClassId_Algorithm_Covariance                                           OpenViBE::CIdentifier(0x0F3B77A6, 0x0301518A)
#define OVP_ClassId_Algorithm_CovarianceDesc                                       OpenViBE::CIdentifier(0x18D15C41, 0x70545A66)

#define OVP_Algorithm_Covariance_InputParameterId_Shrinkage                        OpenViBE::CIdentifier(0x54B90EA7, 0x600A4ACC)
#define OVP_Algorithm_Covariance_InputParameterId_FeatureVectorSet                 OpenViBE::CIdentifier(0x2CF30E42, 0x051F3996)

#define OVP_Algorithm_Covariance_OutputParameterId_CovarianceMatrix                OpenViBE::CIdentifier(0x19F07FB4, 0x084E273B)

namespace OpenViBEPlugins
{
	namespace Classification
	{
		class CAlgorithmCovariance : virtual public OpenViBEToolkit::TAlgorithm < OpenViBE::Plugins::IAlgorithm >
		{
		typedef Eigen::Matrix< double , Eigen::Dynamic , Eigen::Dynamic, Eigen::RowMajor > MatrixXdRowMajor;

		protected:
			void dumpMatrix(OpenViBE::Kernel::ILogManager& pMgr, const MatrixXdRowMajor& mat, const OpenViBE::CString& desc);

		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);

			virtual OpenViBE::boolean process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TAlgorithm < OpenViBE::Plugins::IAlgorithm >, OVP_ClassId_Algorithm_Covariance);

		};

		class CAlgorithmCovarianceDesc : virtual public OpenViBE::Plugins::IAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Covariance"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Computes covariance with shrinkage"); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Shrinkage: {<0 = auto-estimate, [0,1] balance between prior and sample cov}"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_Covariance; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CAlgorithmCovariance; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				rAlgorithmPrototype.addInputParameter (OVP_Algorithm_Covariance_InputParameterId_Shrinkage,         "Shrinkage",           OpenViBE::Kernel::ParameterType_Float);
				rAlgorithmPrototype.addInputParameter (OVP_Algorithm_Covariance_InputParameterId_FeatureVectorSet,  "Feature vectors",     OpenViBE::Kernel::ParameterType_Matrix);

				rAlgorithmPrototype.addOutputParameter (OVP_Algorithm_Covariance_OutputParameterId_CovarianceMatrix, "Covariance matrix",   OpenViBE::Kernel::ParameterType_Matrix);

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IAlgorithmDesc, OVP_ClassId_Algorithm_CovarianceDesc);
		};
	};
};

#endif // __OpenViBEPlugins_Algorithm_Covariance_H__

#endif // TARGET_HAS_ThirdPartyEIGEN

