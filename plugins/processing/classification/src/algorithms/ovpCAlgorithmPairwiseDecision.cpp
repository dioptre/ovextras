#include "ovpCAlgorithmPairwiseStrategy.h"


using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

CAlgorithmPairwiseDecision::process()
{
	if(this->isInputTriggerActive(OVTK_Algorithm_Classifier_InputTriggerId_Train))
	{
		return this->classify();
	}
	return true;
}
