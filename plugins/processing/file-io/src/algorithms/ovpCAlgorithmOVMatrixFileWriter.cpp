#include "ovpCAlgorithmOVMatrixFileWriter.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;

boolean CAlgorithmOVMatrixFileWriter::initialize(void)
{
	ip_sFilename.initialize(getInputParameter(OVP_Algorithm_OVMatrixFileWriter_InputParameterId_Filename));
	ip_pMatrix.initialize(getInputParameter(OVP_Algorithm_OVMatrixFileWriter_InputParameterId_Matrix));

	return true;
}

boolean CAlgorithmOVMatrixFileWriter::uninitialize(void)
{
	ip_sFilename.uninitialize();
	ip_pMatrix.uninitialize();

	return true;
}

boolean CAlgorithmOVMatrixFileWriter::process(void)
{

	if(OpenViBEToolkit::Tools::Matrix::saveToTextFile(*ip_pMatrix, ip_sFilename->toASCIIString()))
	{
		getLogManager() << LogLevel_Trace << "Writing " << *ip_sFilename << " succeeded\n";
		return true;
	}
	else
	{
		getLogManager() << LogLevel_Error << "Writing " << *ip_sFilename << " failed\n";
		return false;
	}
}
