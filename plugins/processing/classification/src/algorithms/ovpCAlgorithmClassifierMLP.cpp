#include "ovpCAlgorithmClassifierMLP.h"

//#if defined TARGET_HAS_ThirdPartyEIGEN

#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <system/ovCMemory.h>
#include <xml/IXMLHandler.h>

#include <Eigen/Eigenvalues>


using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

using namespace Eigen;

boolean CAlgorithmClassifierMLP::initialize()
{
}

boolean CAlgorithmClassifierMLP::uninitialize()
{
	return true;
}

boolean CAlgorithmClassifierMLP::train(const IFeatureVectorSet &rFeatureVectorSet)
{
	return true;
}

boolean CAlgorithmClassifierMLP::classify(const IFeatureVector &rFeatureVector, float64 &rf64Class, IVector &rDistanceValue, IVector &rProbabilityValue)
{
	return true;
}

XML::IXMLNode *CAlgorithmClassifierMLP::saveConfiguration()
{
	return NULL;
}

boolean CAlgorithmClassifierMLP::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	return true;
}

//#endif
