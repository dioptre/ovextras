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
    //this->getLogManager() << LogLevel_Warning << "Process from pairing\n";
    if(this->isInputTriggerActive(OVTK_Algorithm_PairingStrategy_InputTriggerId_DesignArchitecture))
    {
        TParameterHandler < CIdentifier* > ip_pClassifierIdentifier(this->getInputParameter(OVTK_Algorithm_PairingStrategy_InputParameterId_SubClassifierAlgorithm));
        TParameterHandler < uint64* > ip_pClassAmount(this->getInputParameter(OVTK_Algorithm_PairingStrategy_InputParameterId_ClassAmount));

        uint64 l_iClassCount = *((uint64*)ip_pClassAmount);
        CIdentifier l_oClassifierIdentifier = *((CIdentifier*)ip_pClassifierIdentifier);
        this->designArchitecture(l_oClassifierIdentifier, l_iClassCount);
    } else {
        CAlgorithmClassifier::process();
    }
    return true;
}
