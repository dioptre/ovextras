
#pragma once

#include <openvibe/ov_all.h>

#include "TypeBase.h"

class TypeMatrix
{
public:
	static OpenViBE::CIdentifier getTypeIdentifier(void) { return OV_TypeId_StreamedMatrix; };

	class Header : public TypeBase::Header
	{
	public:
		// Header
		OpenViBE::CMatrix m_header;	
	};

	class Buffer : public TypeBase::Buffer
	{
	public:
		// Payload
		OpenViBE::CMatrix m_buffer;	

	};

	// Prevent constructing
	TypeMatrix()=delete;
	TypeMatrix(const TypeMatrix&)=delete;
	TypeMatrix(TypeMatrix&&)=delete;
};
