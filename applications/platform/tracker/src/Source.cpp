
#include "Source.h"

#include <iostream>
#include <thread>
#include <deque>
#include <vector>

#include "../../../../plugins/processing/file-io/src/ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;

bool Source::initialize(const char *signalFile) 
{ 
	std::cout << "Source: Initializing with "	<< signalFile << "\n";

	m_ChunksSent = 0;
	m_bPending = false;

	m_pFile = FS::Files::open(signalFile, "rb");
	if(!m_pFile)
	{
		return false;
	}

	return true; 
}

bool Source::uninitialize(void) 
{ 
	std::cout << "Source: Uninitializing\n";

	if(m_pFile) 
	{
		::fclose(m_pFile);
		m_pFile = nullptr;
	}

	return true;
}

bool Source::pullChunk(MemoryBufferWithType& output)
{
	if(!m_pFile)
	{
		std::cout << "Error: No source file set\n";
		return false;
	}
//	std::cout << "Source: Trying to pull a chunk\n";

	while(!feof(m_pFile))
	{

		while(!::feof(m_pFile) && m_oReader.getCurrentNodeIdentifier()==EBML::CIdentifier())
		{
			uint8 l_ui8Byte;
			size_t s=::fread(&l_ui8Byte, sizeof(uint8), 1, m_pFile);

			/*
			OV_ERROR_UNLESS_KRF(
				s == 1 || l_bJustStarted,
				"Unexpected EOF in " << m_sFilename,
				OpenViBE::Kernel::ErrorType::BadParsing
			);
			*/

			m_oReader.processData(&l_ui8Byte, sizeof(l_ui8Byte));
		}
		if(!::feof(m_pFile) && m_oReader.getCurrentNodeSize()!=0)
		{
			m_oSwap.setSize(m_oReader.getCurrentNodeSize(), true);
			size_t s= (size_t) ::fread(m_oSwap.getDirectPointer(), sizeof(uint8), (size_t) m_oSwap.getSize(), m_pFile);

			/*
			OV_ERROR_UNLESS_KRF(
				s == m_oSwap.getSize(),
				"Unexpected EOF in " << m_sFilename,
				OpenViBE::Kernel::ErrorType::BadParsing
			);
			*/

			m_oPendingChunk.setSize(0, true);
			m_ChunkStreamIndex = std::numeric_limits<uint32>::max();
			m_ui64StartTime = std::numeric_limits<uint64>::max();
			m_ui64EndTime = std::numeric_limits<uint64>::max();

			m_oReader.processData(m_oSwap.getDirectPointer(), m_oSwap.getSize());
		}

		if(m_bPending)
		{
			// We have dada
			// std::cout << "Source: Found a chunk, queueing\n";

			output.buffer.setSize(0, true);
			output.buffer.append(m_oPendingChunk);
			output.streamIndex = m_ChunkStreamIndex;
			output.streamType = m_ChunkStreamType[m_ChunkStreamIndex];
			output.bufferStart = m_ui64StartTime;
			output.bufferEnd = m_ui64EndTime;
			return true;
		}

	}

	if(feof(m_pFile))
	{
		std::cout << "Source file EOF reached\n";
	}
	else
	{
		std::cout << "Issue with source file\n";
	}
	
	return false;
}


bool Source::stop(void)
{

	return true;
}

EBML::boolean Source::isMasterChild(const EBML::CIdentifier& rIdentifier)
{
	if(rIdentifier==EBML_Identifier_Header                        ) return true;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Header              ) return true;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Header_Compression  ) return false;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Header_StreamType  ) return false;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Buffer              ) return true;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Buffer_StreamIndex ) return false;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Buffer_StartTime    ) return false;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Buffer_EndTime      ) return false;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Buffer_Content      ) return false;
	return false;
}

void Source::openChild(const EBML::CIdentifier& rIdentifier)
{
	m_vNodes.push(rIdentifier);

	EBML::CIdentifier& l_rTop=m_vNodes.top();

	if(l_rTop == EBML_Identifier_Header)
	{
		m_bHasEBMLHeader = true;
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Header)
	{
		if(!m_bHasEBMLHeader)
		{
//			this->getLogManager() << LogLevel_Info << "The file " << m_sFilename << " uses an outdated (but still compatible) version of the .ov file format\n";
		}
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Header)
	{
		m_vStreamIndexToOutputIndex.clear();
		m_vStreamIndexToTypeIdentifier.clear();
	}
}

void Source::processChildData(const void* pBuffer, const uint64 ui64BufferSize)
{
	EBML::CIdentifier& l_rTop=m_vNodes.top();

	// Uncomment this when ebml version will be used
	//if(l_rTop == EBML_Identifier_EBMLVersion)
	//{
	//	const uint64 l_ui64VersionNumber=(uint64)m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize);
	//}

	if(l_rTop==OVP_NodeId_OpenViBEStream_Header_Compression)
	{
		if(m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize) != 0)
		{
//			OV_WARNING_K("Impossible to use compression as it is not yet implemented");
		}
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Header_StreamType)
	{
		m_ChunkStreamType.push_back(m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize));
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Buffer_StreamIndex)
	{
		m_ChunkStreamIndex = (uint32)m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize);
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Buffer_StartTime)
	{
		m_ui64StartTime=m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize);
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Buffer_EndTime)
	{
		m_ui64EndTime=m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize);
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Buffer_Content)
	{
		m_oPendingChunk.setSize(0, true);
		m_oPendingChunk.append(reinterpret_cast<const EBML::uint8*>(pBuffer), ui64BufferSize);
	}
}

void Source::closeChild(void)
{
	EBML::CIdentifier& l_rTop=m_vNodes.top();

	if(l_rTop==OVP_NodeId_OpenViBEStream_Header)
	{
		// Assign file streams to outputs here
	}

	if(l_rTop==OVP_NodeId_OpenViBEStream_Buffer)
	{
		m_bPending = ((m_ChunkStreamIndex != std::numeric_limits<uint32>::max()) &&
					  (m_ui64StartTime != std::numeric_limits<uint64>::max()) &&
					  (m_ui64EndTime != std::numeric_limits<uint64>::max()));
	}

	m_vNodes.pop();
}
