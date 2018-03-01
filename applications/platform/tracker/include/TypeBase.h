
#pragma once

#include <openvibe/ov_all.h>

class TypeBase {
public:
	static OpenViBE::CIdentifier getTypeIdentifier(void) { return OV_UndefinedIdentifier; };

	class Header {
	public:

	};

	class Buffer
	{
	public:
		// Payload
		uint64_t m_bufferStart = 0;
		uint64_t m_bufferEnd = 0;

	};

	// Prevent constructing
	TypeBase()=delete;
	TypeBase(const TypeBase&)=delete;
	TypeBase(TypeBase&&)=delete;
};
