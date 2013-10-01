#include "ovasCDriverGenericRawFileReader.h"
#include "ovasCConfigurationGenericRawReader.h"

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;

CDriverGenericRawFileReader::CDriverGenericRawFileReader(IDriverContext& rDriverContext)
	:CDriverGenericRawReader(rDriverContext)
	,m_pFile(NULL)
{
	m_sFilename="/tmp/some_raw_file";
}

boolean CDriverGenericRawFileReader::configure(void)
{
	CConfigurationGenericRawReader m_oConfiguration(OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-Generic-RawFileReader.ui",
		m_bLimitSpeed,
		m_ui32SampleFormat,
		m_ui32SampleEndian,
		m_ui32StartSkip,
		m_ui32HeaderSkip,
		m_ui32FooterSkip,
		m_sFilename);
	return m_oConfiguration.configure(m_oHeader);
}

boolean CDriverGenericRawFileReader::open(void)
{
	m_pFile=::fopen(m_sFilename.toASCIIString(), "rb");
	if(!m_pFile)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not open file [" << m_sFilename << "]\n";
		return false;
	}
	if(fseek(m_pFile, m_ui32StartSkip, SEEK_SET)!=0)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not seek to " << m_ui32StartSkip << " bytes from the file beginning\n";
		fclose(m_pFile);
		return false;
	}
	return true;
}

boolean CDriverGenericRawFileReader::close(void)
{
	if(m_pFile)
	{
		::fclose(m_pFile);
		m_pFile=NULL;
	}
	return true;
}

boolean CDriverGenericRawFileReader::read(void)
{
	if(!m_pFile)
	{
		return false;
	}
	boolean l_bReturnValue = (::fread(m_pDataFrame, 1, m_ui32DataFrameSize, m_pFile)==m_ui32DataFrameSize);
	if(!l_bReturnValue && ::feof(m_pFile)) {
		m_rDriverContext.getLogManager() << LogLevel_Info << "End of file reached.\n";
	}
	return l_bReturnValue;
}
