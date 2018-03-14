
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
		virtual bool getEBML(EBML::IWriterHelper& target) const override; 

		// Header
		OpenViBE::CMatrix m_header;	
	};

	class Buffer : public TypeBase::Buffer
	{
	public:
		virtual bool getEBML(EBML::IWriterHelper& target) const override;

		// Payload
		OpenViBE::CMatrix m_buffer;	

	};

	// Prevent constructing
	TypeMatrix()=delete;
	TypeMatrix(const TypeMatrix&)=delete;
	TypeMatrix(TypeMatrix&&)=delete;
};

