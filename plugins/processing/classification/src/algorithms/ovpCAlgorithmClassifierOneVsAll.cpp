#include "ovpCAlgorithmClassifierOneVsAll.h"

#include <map>
#include <sstream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Local;

using namespace OpenViBEToolkit;





boolean CAlgorithmClassifierOneVsAll::train(const IFeatureVectorSet& rFeatureVectorSet)
{

    uint32 i;
    std::map < float64, uint64 > l_vClassLabels;
    for(i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
    {
        l_vClassLabels[rFeatureVectorSet[i].getLabel()]++;
    }
    this->getLogManager() << "blabla" << l_vClassLabels.size() << "]\n";

    return true;
}

boolean CAlgorithmClassifierOneVsAll::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
{

    return true;
}

boolean CAlgorithmClassifierOneVsAll::designArchitecture(OpenViBE::CIdentifier &rId, OpenViBEToolkit::IVector& rClassesList)
{
    return true;
}

boolean CAlgorithmClassifierOneVsAll::saveConfiguration(IMemoryBuffer& rMemoryBuffer)
{

    return true;
}

boolean CAlgorithmClassifierOneVsAll::loadConfiguration(const IMemoryBuffer& rMemoryBuffer)
{


    return true;
}

void CAlgorithmClassifierOneVsAll::write(const char* sString)
{
}

void CAlgorithmClassifierOneVsAll::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{

}

void CAlgorithmClassifierOneVsAll::processChildData(const char* sData)
{


}

void CAlgorithmClassifierOneVsAll::closeChild(void)
{
}

