
#pragma once

#include <openvibe/ov_all.h>

#include "TypeBase.h"

class TypeStimulation
{
public:
	static OpenViBE::CIdentifier getTypeIdentifier(void) { return OV_TypeId_Stimulations; };

	class Header : public TypeBase::Header
	{
	public:
	};

	class Buffer : public TypeBase::Buffer
	{
	public:

		virtual bool getEBML(EBML::IWriterHelper& target) const override;

		// Payload
		OpenViBE::CStimulationSet m_buffer;

	};

	// Prevent constructing
	TypeStimulation()=delete;
	TypeStimulation(const TypeStimulation&)=delete;
	TypeStimulation(TypeStimulation&&)=delete;
};

