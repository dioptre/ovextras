#include "ovtkCAlgorithmClassifier.h"
#include "ovtkCFeatureVector.hpp"
#include "ovtkCFeatureVectorSet.hpp"
#include "ovtkCVector.hpp"
#include "ovtkCAlgorithmPairingStrategy.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEToolkit;


boolean CAlgorithmPairingStrategy::process(void)
{
    this->getLogManager() << LogLevel_Warning << "Process from pairing\n";
    if(this->isInputTriggerActive(OVTK_Algorithm_PairingStrategy_InputTriggerId_DesignArchitecture))
    {
        this->getLogManager() << LogLevel_Warning << "Design architecture\n";
    } else {
        CAlgorithmClassifier::process();
    }
    return true;
}
