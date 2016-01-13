#include "ovpCAlgorithmOVMatrixFileReader.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;

boolean CAlgorithmOVMatrixFileReader::initialize(void)
{
	ip_sFilename.initialize(getInputParameter(OVP_Algorithm_OVMatrixFileReader_InputParameterId_Filename));

	op_pMatrix.initialize(getOutputParameter(OVP_Algorithm_OVMatrixFileReader_OutputParameterId_Matrix));

	return true;
}

boolean CAlgorithmOVMatrixFileReader::uninitialize(void)
{
	op_pMatrix.uninitialize();

	ip_sFilename.uninitialize();

	return true;
}

boolean CAlgorithmOVMatrixFileReader::process(void)
{
	if(OpenViBEToolkit::Tools::Matrix::loadFromTextFile(*op_pMatrix, ip_sFilename->toASCIIString()))
	{
		getLogManager() << LogLevel_Trace << "Parsing " << *ip_sFilename << " succeeded\n";
		return true;
	}
	else
	{
		getLogManager() << LogLevel_Warning << "Parsing " << *ip_sFilename << " failed\n";
		return false;
	}
}
