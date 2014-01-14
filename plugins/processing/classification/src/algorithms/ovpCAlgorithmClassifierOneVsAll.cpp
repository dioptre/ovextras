#include "ovpCAlgorithmClassifierOneVsAll.h"

#include <map>
#include <sstream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::Local;

using namespace OpenViBEToolkit;


boolean CAlgorithmClassifierOneVsAll::uninitialize(void)
{
    for(int64 i = m_oSubClassifierList.size()-1 ; i >= 0 ; --i)
    {
        this->getLogManager() << LogLevel_Warning << "Release classifier " << i <<"\n";
        IAlgorithmProxy* l_pSubClassifier = m_oSubClassifierList[i];
        l_pSubClassifier->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*l_pSubClassifier);
        m_oSubClassifierList.pop_back();
    }
    return true;
}



boolean CAlgorithmClassifierOneVsAll::train(const IFeatureVectorSet& rFeatureVectorSet)
{

    uint32 i;
    std::map < float64, uint64 > l_vClassLabels;
    for(i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
    {
        l_vClassLabels[rFeatureVectorSet[i].getLabel()]++;
    }
    this->getLogManager() << "blabla" << l_vClassLabels.size() << "]\n";

    //We send new set of data to each classifer. They will all use two different classes 1 and 2. 1 is for the class it should recognize, 2 for the others
    for(uint32 l_iClassifierCounter = 1 ; l_iClassifierCounter <= m_oSubClassifierList.size() ; ++l_iClassifierCounter )
    {
        uint32 l_ui32FeatureVectorSize=rFeatureVectorSet[0].getSize();

        TParameterHandler < IMatrix* > ip_pFeatureVectorSet(m_oSubClassifierList[l_iClassifierCounter-1]->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet));
        ip_pFeatureVectorSet->setDimensionCount(2);
        ip_pFeatureVectorSet->setDimensionSize(0, rFeatureVectorSet.getFeatureVectorCount());
        ip_pFeatureVectorSet->setDimensionSize(1, l_ui32FeatureVectorSize+1);

        float64* l_pFeatureVectorSetBuffer=ip_pFeatureVectorSet->getBuffer();
        for(size_t j=0; j<rFeatureVectorSet.getFeatureVectorCount(); j++)
        {
            System::Memory::copy(
                l_pFeatureVectorSetBuffer,
                        rFeatureVectorSet[j].getBuffer(),
                l_ui32FeatureVectorSize*sizeof(float64));

            //Modify the class of each featureVector
            float64 l_f64Class = rFeatureVectorSet[j].getLabel();
            if((uint32)l_f64Class == l_iClassifierCounter)
            {
                l_pFeatureVectorSetBuffer[l_ui32FeatureVectorSize]=1;
            }
            else
            {
                l_pFeatureVectorSetBuffer[l_ui32FeatureVectorSize]=2;
            }
            l_pFeatureVectorSetBuffer+=(l_ui32FeatureVectorSize+1);
        }

        m_oSubClassifierList[l_iClassifierCounter-1]->process(OVTK_Algorithm_Classifier_InputTriggerId_Train);
        m_oSubClassifierList[l_iClassifierCounter-1]->process(OVTK_Algorithm_Classifier_InputTriggerId_SaveConfiguration);
    }
    return true;
}

boolean CAlgorithmClassifierOneVsAll::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues)
{

    return true;
}

boolean CAlgorithmClassifierOneVsAll::designArchitecture(OpenViBE::CIdentifier &rId, uint64 &rClassAmount)
{
    this->getLogManager() << LogLevel_Warning << "Start design architecture for " << rClassAmount <<" and algorithm "<< rId.toString() <<"\n";
    for(uint64 i = 1 ; i <= rClassAmount ; ++i)
    {
        this->getLogManager() << LogLevel_Warning << "Create Subclassifier for class" << i <<"\n";
        IAlgorithmProxy* l_pSubClassifier = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(rId));
        l_pSubClassifier->initialize();
        m_oSubClassifierList.push_back(l_pSubClassifier);
    }
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

