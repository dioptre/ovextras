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
    if(this->isInputTriggerActive(OVTK_Algorithm_Classifier_InputTriggerId_Train))
    {

    }
}
