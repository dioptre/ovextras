
#pragma once

#include <openvibe/ov_all.h>

#include "TypeMatrix.h"

class TypeSignal
{
public:
	static OpenViBE::CIdentifier getTypeIdentifier(void) { return OV_TypeId_Signal; };

	class Header : public TypeMatrix::Header
	{
	public:
		virtual bool getEBML(EBML::IWriterHelper& target) const override;

		// Header
		uint64_t m_samplingFrequency = 0;
	};

	class Buffer : public TypeMatrix::Buffer
	{

	};


	// Prevent constructing
	TypeSignal()=delete;
	TypeSignal(const TypeSignal&)=delete;
	TypeSignal(TypeSignal&&)=delete;
};


