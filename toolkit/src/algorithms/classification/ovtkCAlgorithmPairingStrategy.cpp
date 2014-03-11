#include "ovtkCAlgorithmClassifier.h"
#include "ovtkCAlgorithmPairingStrategy.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEToolkit;


boolean CAlgorithmPairingStrategy::process(void)
{
	if(this->isInputTriggerActive(OVTK_Algorithm_PairingStrategy_InputTriggerId_DesignArchitecture))
	{
		TParameterHandler < CIdentifier* > ip_pClassifierIdentifier(this->getInputParameter(OVTK_Algorithm_PairingStrategy_InputParameterId_SubClassifierAlgorithm));
		TParameterHandler < int64 > ip_pClassAmount(this->getInputParameter(OVTK_Algorithm_PairingStrategy_InputParameterId_ClassAmount));

		int64 l_iClassCount = (int64) ip_pClassAmount;
		CIdentifier l_oClassifierIdentifier = *((CIdentifier*)ip_pClassifierIdentifier);
		this->designArchitecture(l_oClassifierIdentifier, l_iClassCount);
	}
	else
	{
		CAlgorithmClassifier::process();
	}
	return true;
}
