/*
 *
 * Incremental covariance estimators with shrinkage
 *
 */
#if defined TARGET_HAS_ThirdPartyEIGEN

#ifndef __OpenViBEPlugins_Algorithm_OnlineCovariance_H__
#define __OpenViBEPlugins_Algorithm_OnlineCovariance_H__

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <Eigen/Dense> 

#define OVP_ClassId_Algorithm_OnlineCovariance                                            OpenViBE::CIdentifier(0x5ADD4F8E, 0x005D29C1)
#define OVP_ClassId_Algorithm_OnlineCovarianceDesc                                        OpenViBE::CIdentifier(0x00CD2DEA, 0x4C000CEB)

#define OVP_Algorithm_OnlineCovariance_InputParameterId_Shrinkage                         OpenViBE::CIdentifier(0x16577C7B, 0x4E056BF7) 
#define OVP_Algorithm_OnlineCovariance_InputParameterId_InputVectors                      OpenViBE::CIdentifier(0x47E55F81, 0x27A519C4)
#define OVP_Algorithm_OnlineCovariance_InputParameterId_UpdateMethod                      OpenViBE::CIdentifier(0x1C4F444F, 0x3CA213E2)
#define OVP_Algorithm_OnlineCovariance_InputParameterId_TraceNormalization                OpenViBE::CIdentifier(0x269D5E63, 0x3B6D486E)

#define OVP_Algorithm_OnlineCovariance_OutputParameterId_Mean                             OpenViBE::CIdentifier(0x3F1F50A3, 0x05504D0E)
#define OVP_Algorithm_OnlineCovariance_OutputParameterId_CovarianceMatrix                 OpenViBE::CIdentifier(0x203A5472, 0x67C5324C)

#define OVP_Algorithm_OnlineCovariance_Process_Reset                                      OpenViBE::CIdentifier(0x4C1C510C, 0x3CF56E7C) // to reset estimates to 0
#define OVP_Algorithm_OnlineCovariance_Process_Update                                     OpenViBE::CIdentifier(0x72BF2277, 0x2974747B) // update estimates with a new chunk of data
#define OVP_Algorithm_OnlineCovariance_Process_GetCov                                     OpenViBE::CIdentifier(0x2BBC4A91, 0x27050CFD) // also returns the mean estimate
#define OVP_Algorithm_OnlineCovariance_Process_GetCovRaw                                  OpenViBE::CIdentifier(0x0915148C, 0x5F792B2A) // also returns the mean estimate

#define OVP_TypeId_OnlineCovariance_UpdateMethod									      OpenViBE::CIdentifier(0x59E83F33, 0x592F1DD0)
#define OVP_TypeId_OnlineCovariance_UpdateMethod_ChunkAverage						      OpenViBE::CIdentifier(0x079E14D3, 0x784A2BD1)
#define OVP_TypeId_OnlineCovariance_UpdateMethod_Incremental		    			      OpenViBE::CIdentifier(0x39E20E6D, 0x6A87073C)



namespace OpenViBEPlugins
{
	namespace SignalProcessing
	{
		class CAlgorithmOnlineCovariance : virtual public OpenViBEToolkit::TAlgorithm < OpenViBE::Plugins::IAlgorithm >
		{
		typedef Eigen::Matrix< double , Eigen::Dynamic , Eigen::Dynamic, Eigen::RowMajor > MatrixXdRowMajor;

		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TAlgorithm < OpenViBE::Plugins::IAlgorithm >, OVP_ClassId_Algorithm_OnlineCovariance);

		protected:
			// Debug method. Prints the matrix to the logManager. May be disabled in implementation.
			void dumpMatrix(OpenViBE::Kernel::ILogManager& pMgr, const MatrixXdRowMajor& mat, const OpenViBE::CString& desc);

			// These are non-normalized estimates for the corresp. statistics
			Eigen::MatrixXd m_oIncrementalCov;
			Eigen::MatrixXd m_oIncrementalMean;

			// The divisor for the above estimates to do the normalization
			OpenViBE::uint64 m_ui64Count;

		};

		class CAlgorithmOnlineCovarianceDesc : virtual public OpenViBE::Plugins::IAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Online Covariance"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Jussi T. Lindgren"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("Inria"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Incrementally computes covariance with shrinkage."); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Regularized covariance output is computed as (diag*shrink + cov)"); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.5"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_OnlineCovariance; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::SignalProcessing::CAlgorithmOnlineCovariance; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_Shrinkage,          "Shrinkage",           OpenViBE::Kernel::ParameterType_Float);
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_InputVectors,       "Input vectors",       OpenViBE::Kernel::ParameterType_Matrix);
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_UpdateMethod,       "Cov update method",   OpenViBE::Kernel::ParameterType_Enumeration, OVP_TypeId_OnlineCovariance_UpdateMethod);
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_OnlineCovariance_InputParameterId_TraceNormalization, "Trace normalization", OpenViBE::Kernel::ParameterType_Boolean);

				// The algorithm returns these outputs
				rAlgorithmPrototype.addOutputParameter (OVP_Algorithm_OnlineCovariance_OutputParameterId_Mean,             "Mean vector",        OpenViBE::Kernel::ParameterType_Matrix);
				rAlgorithmPrototype.addOutputParameter (OVP_Algorithm_OnlineCovariance_OutputParameterId_CovarianceMatrix, "Covariance matrix",  OpenViBE::Kernel::ParameterType_Matrix);

				rAlgorithmPrototype.addInputTrigger(OVP_Algorithm_OnlineCovariance_Process_Reset,     "Reset the algorithm");
				rAlgorithmPrototype.addInputTrigger(OVP_Algorithm_OnlineCovariance_Process_Update,    "Append a chunk of data");
				rAlgorithmPrototype.addInputTrigger(OVP_Algorithm_OnlineCovariance_Process_GetCov,    "Get the current regularized covariance matrix & mean");
				rAlgorithmPrototype.addInputTrigger(OVP_Algorithm_OnlineCovariance_Process_GetCovRaw, "Get the current covariance matrix & mean");

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IAlgorithmDesc, OVP_ClassId_Algorithm_OnlineCovarianceDesc);
		};
	};
};

#endif // __OpenViBEPlugins_Algorithm_OnlineCovariance_H__

#endif // TARGET_HAS_ThirdPartyEIGEN

