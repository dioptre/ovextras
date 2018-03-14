
#pragma once

#include <openvibe/ov_all.h>

#include <ebml/IWriterHelper.h>
#include <ebml/IWriter.h>
#include <ebml/TWriterCallbackProxy.h>

class TypeBase {
public:
	static OpenViBE::CIdentifier getTypeIdentifier(void) { return OV_UndefinedIdentifier; };

	class Header {
	public:
		virtual bool getEBML(EBML::IWriterHelper& target) const { return true; };
	};

	class Buffer
	{
	public:
		virtual bool getEBML(EBML::IWriterHelper& target) const { return true; };

		// Payload
		uint64_t m_bufferStart = 0;
		uint64_t m_bufferEnd = 0;

	};

	class End
	{
		virtual bool getEBML(EBML::IWriterHelper& target) const { return true; };	
	};

	// Prevent constructing
	TypeBase()=delete;
	TypeBase(const TypeBase&)=delete;
	TypeBase(TypeBase&&)=delete;
};
