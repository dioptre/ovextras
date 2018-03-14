
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeSignal.h"

bool TypeSignal::Header::getEBML(EBML::IWriterHelper& target) const 
{
	target.openChild(OVTK_NodeId_Header_Signal);
	{
		target.openChild(OVTK_NodeId_Header_Signal_SamplingRate);
		{
			target.setUIntegerAsChildData(m_samplingFrequency);
			target.closeChild();
		}
		target.closeChild();
	}
	return TypeMatrix::Header::getEBML(target);
};

