#ifndef __OpenViBEPlugins_Algorithm_PairwiseDecision_H__
#define __OpenViBEPlugins_Algorithm_PairwiseDecision_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#define OVP_ClassId_Algorithm_PairwiseDecision												OpenViBE::CIdentifier(0x26EF6DDA, 0xF137053C)
#define OVP_ClassId_Algorithm_PairwiseDecisionDesc											OpenViBE::CIdentifier(0x191EB02A, 0x6866214A)

namespace OpenViBEPlugins
{
	namespace Classification
	{
		class CAlgorithmPairwiseDecision : virtual public OpenViBEToolkit::TAlgorithm < OpenViBE::Plugins::IAlgorithm >
		{

		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void)=0;
			virtual OpenViBE::boolean uninitialize(void)=0;

			virtual OpenViBE::boolean classify(void) =0;

			virtual OpenViBE::boolean process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TAlgorithm < OpenViBE::Plugins::IAlgorithm >, OVP_ClassId_Algorithm_PairwiseDecision)
		};

		class CAlgorithmPairwiseDecisionDesc : virtual public OpenViBE::Plugins::IAlgorithmDesc
		{
		public:
			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				rAlgorithmPrototype.addInputParameter(OVP_Algorithm_Classifier_InputParameter_ProbabilityMatrix, "Probability Matrix", OpenViBE::Kernel::ParameterType_Matrix);

				rAlgorithmPrototype.addOutputParameter(OVP_Algorithm_Classifier_OutputParameter_ProbabilityVector, "Output Probability Matrix", OpenViBE::Kernel::ParameterType_Matrix);


				rAlgorithmPrototype.addInputTrigger(OVP_Algorithm_Classifier_Pairwise_InputTriggerId_Classifiy, "Classify");
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IAlgorithmDesc, OVP_ClassId_Algorithm_PairwiseDecisionDesc)
		};
	}
}



#endif // __OpenViBEPlugins_Algorithm_PairwiseStrategy_PKPD_H__
