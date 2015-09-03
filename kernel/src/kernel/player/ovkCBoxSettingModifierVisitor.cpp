
#include "ovkCBoxSettingModifierVisitor.h"

#include "ovkCSimulatedBox.h"
#include "ovkCPlayer.h"

#include <openvibe/ovIObjectVisitor.h>
#include <xml/IReader.h>

#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <limits>

#define OVD_AttributeId_SettingOverrideFilename             OpenViBE::CIdentifier(0x8D21FF41, 0xDF6AFE7E)

//___________________________________________________________________//
//                                                                   //

using namespace std;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

void CBoxSettingModifierVisitor::openChild(const char* sName, const char** sAttributeName, const char** sAttributeValue, XML::uint64 ui64AttributeCount)
{
	if(!m_bIsParsingSettingOverride)
	{
		if(string(sName)==string("OpenViBE-SettingsOverride"))
		{
			m_bIsParsingSettingOverride=true;
		}
	}
	else if(string(sName)==string("SettingValue"))
	{
		m_bIsParsingSettingValue=true;
	}
	else
	{
		m_bIsParsingSettingValue=false;
	}
}

void CBoxSettingModifierVisitor::processChildData(const char* sData)
{
	if(m_bIsParsingSettingValue)
	{
		m_pObjectVisitorContext->getLogManager() << LogLevel_Debug << "Using [" << CString(sData) << "] as setting " << m_ui32SettingIndex << "...\n";
		m_pBox->setSettingValue(m_ui32SettingIndex, sData);
		m_ui32SettingIndex++;
	}
}

void CBoxSettingModifierVisitor::closeChild(void)
{
	m_bIsParsingSettingValue=false;
}

boolean CBoxSettingModifierVisitor::processBegin(IObjectVisitorContext& rObjectVisitorContext, IBox& rBox)
{
	boolean l_bReturnValue = true;

	m_pObjectVisitorContext=&rObjectVisitorContext;

	// checks if this box should override
	// settings from external file
	if(rBox.hasAttribute(OVD_AttributeId_SettingOverrideFilename))
	{
		CString l_sSettingOverrideFilename=rBox.getAttributeValue(OVD_AttributeId_SettingOverrideFilename);
		CString l_sSettingOverrideFilenameFinal;
		if (m_pConfigurationManager == NULL)
		{
			l_sSettingOverrideFilenameFinal=rObjectVisitorContext.getConfigurationManager().expand(l_sSettingOverrideFilename);
		}
		else
		{
			l_sSettingOverrideFilenameFinal = m_pConfigurationManager->expand(l_sSettingOverrideFilename);
		}

		// message
		rObjectVisitorContext.getLogManager() << LogLevel_Trace << "Trying to override [" << rBox.getName() << "] box settings with file [" << l_sSettingOverrideFilename << " which expands to " << l_sSettingOverrideFilenameFinal << "] !\n";

		// creates XML reader
		XML::IReader* l_pReader=XML::createReader(*this);

		// adds new box settings
		m_pBox=&rBox;
		m_ui32SettingIndex=0;
		m_bIsParsingSettingValue=false;
		m_bIsParsingSettingOverride=false;

		// 1. Open settings file (binary because read would conflict with tellg for text files)
		// 2. Loop until end of file, reading it
		//    and sending what is read to the XML parser
		// 3. Close the settings file
		ifstream l_oFile(l_sSettingOverrideFilenameFinal.toASCIIString(), ios::binary);
		if(l_oFile.is_open())
		{
			char l_sBuffer[1024];
			std::streamoff l_iBufferLen=0;
			bool l_bStatusOk=true;
			l_oFile.seekg(0, ios::end);
			std::streamoff l_iFileLen=l_oFile.tellg();
			l_oFile.seekg(0, ios::beg);
			while(l_iFileLen && l_bStatusOk)
			{
				// File length is always positive so this is safe
				l_iBufferLen=(unsigned(l_iFileLen)>sizeof(l_sBuffer)?sizeof(l_sBuffer):l_iFileLen);
				l_oFile.read(l_sBuffer, l_iBufferLen);
				l_iFileLen-=l_iBufferLen;
				l_bStatusOk=l_pReader->processData(l_sBuffer, l_iBufferLen);
			}
			l_oFile.close();

			// message
			if(m_ui32SettingIndex == rBox.getSettingCount())
			{
				rObjectVisitorContext.getLogManager() << LogLevel_Trace << "Overrode " << m_ui32SettingIndex << " setting(s) with this configuration file...\n";
			}
			else
			{
				rObjectVisitorContext.getLogManager() << LogLevel_Warning << "Overrode " << m_ui32SettingIndex << " setting(s) with configuration file [" << l_sSettingOverrideFilenameFinal << "]. That does not match the box setting count " << rBox.getSettingCount() << "...\n";
			}
		}
		else
		{
			// override file was not found
			rObjectVisitorContext.getLogManager() << LogLevel_Error << "Could not override [" << rBox.getName() << "] settings because configuration file [" << l_sSettingOverrideFilenameFinal << "] could not be opened\n";
			l_bReturnValue = false;
		}

		// cleans up internal state
		m_pBox=NULL;
		m_ui32SettingIndex=0;
		m_bIsParsingSettingValue=false;
		m_bIsParsingSettingOverride=false;

		// releases XML reader
		l_pReader->release();
		l_pReader=NULL;
	}

	return l_bReturnValue;
}

boolean CBoxSettingModifierVisitor::processEnd(IObjectVisitorContext& rObjectVisitorContext, IBox& rBox)
{
	m_pObjectVisitorContext=&rObjectVisitorContext;
	return true;
}


