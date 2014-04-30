#include "ovpCAlgorithmPairwiseDecision.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

boolean CAlgorithmPairwiseDecision::process()
{
	if(this->isInputTriggerActive(OVP_Algorithm_Classifier_Pairwise_InputTriggerId_Classifiy))
	{
		return this->classify();
	}
	return true;
}
