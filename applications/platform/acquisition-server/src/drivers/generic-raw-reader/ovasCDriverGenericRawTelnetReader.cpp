#include "ovasCDriverGenericRawTelnetReader.h"
#include "ovasCConfigurationGenericRawReader.h"

#include "../ovasCSettingsHelper.h"

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;

CDriverGenericRawTelnetReader::CDriverGenericRawTelnetReader(IDriverContext& rDriverContext)
	:CDriverGenericRawReader(rDriverContext)
	,m_pConnection(NULL)
{
	m_sHostName="localhost";
	m_ui32HostPort=1337;

}

boolean CDriverGenericRawTelnetReader::configure(void)
{
	CString l_sFilename;
	CConfigurationGenericRawReader m_oConfiguration(OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-Generic-RawTelnetReader.ui",
		m_bLimitSpeed,
		m_ui32SampleFormat,
		m_ui32SampleEndian,
		m_ui32StartSkip,
		m_ui32HeaderSkip,
		m_ui32FooterSkip,
		l_sFilename);

	// Relay configuration properties to the configuration manager
	SettingsHelper l_oProperties("AcquisitionServer_Driver_GenericRawTelnetReader", m_rDriverContext.getConfigurationManager());
	// l_oProperties.add("Header", &m_oHeader);
	l_oProperties.add("LimitSpeed", &m_bLimitSpeed);
	l_oProperties.add("SampleFormat", &m_ui32SampleFormat);
	l_oProperties.add("SampleEndian", &m_ui32SampleEndian);
	l_oProperties.add("StartSkip", &m_ui32StartSkip);
	l_oProperties.add("HeaderSkip", &m_ui32HeaderSkip);
	l_oProperties.add("FooterSkip", &m_ui32FooterSkip);
	l_oProperties.add("HostName", &m_sHostName);
	l_oProperties.add("HostPort", &m_ui32HostPort);
	l_oProperties.load();

	m_oConfiguration.setHostName(m_sHostName);
	m_oConfiguration.setHostPort(m_ui32HostPort);

	if(!m_oConfiguration.configure(m_oHeader))
	{
		return false;
	}

	m_sHostName=m_oConfiguration.getHostName();
	m_ui32HostPort=m_oConfiguration.getHostPort();

	l_oProperties.save();

	return true;
}

boolean CDriverGenericRawTelnetReader::open(void)
{
	m_pConnection=Socket::createConnectionClient();
	if(!m_pConnection)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not create client connection\n";
		return false;
	}
	if(!m_pConnection->connect(m_sHostName.toASCIIString(), m_ui32HostPort))
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not connect to server [" << m_sHostName << ":" << m_ui32HostPort << "]\n";
		return false;
	}
	char *l_sBuffer = new char[m_ui32StartSkip];
	if(m_ui32StartSkip>0 && !m_pConnection->receiveBufferBlocking(l_sBuffer, m_ui32StartSkip))
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Unable to skip " << m_ui32StartSkip << " bytes at the beginning\n";
		delete[] l_sBuffer;
		return false;
	}
	delete[] l_sBuffer;

	return true;
}

boolean CDriverGenericRawTelnetReader::close(void)
{
	if(m_pConnection)
	{
		m_pConnection->close();
		m_pConnection->release();
		m_pConnection=NULL;
	}
	return true;
}

boolean CDriverGenericRawTelnetReader::read(void)
{
	if(!m_pConnection)
	{
		return false;
	}
	return m_pConnection->receiveBufferBlocking(m_pDataFrame, m_ui32DataFrameSize);
}

