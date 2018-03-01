
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
		// Header
		uint32_t samplingFrequency = 0;
	};

	class Buffer : public TypeMatrix::Buffer
	{


	};

	// Prevent constructing
	TypeSignal()=delete;
	TypeSignal(const TypeSignal&)=delete;
	TypeSignal(TypeSignal&&)=delete;
};


