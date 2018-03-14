
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeStimulation.h"

bool TypeStimulation::Buffer::getEBML(EBML::IWriterHelper& target) const
{ 
	target.openChild(OVTK_NodeId_Buffer);
	{
		target.openChild(OVTK_NodeId_Buffer_Stimulation);
		{
			target.openChild(OVTK_NodeId_Buffer_Stimulation_NumberOfStimulations);
			{
				target.setUIntegerAsChildData(m_buffer.getStimulationCount());
				target.closeChild();
			}
			for(uint32_t i=0;i<m_buffer.getStimulationCount();i++)
			{
				target.openChild(OVTK_NodeId_Buffer_Stimulation_Stimulation_Identifier);
				{
					target.setUIntegerAsChildData(m_buffer.getStimulationIdentifier(i));
					target.closeChild();
				}
				target.openChild(OVTK_NodeId_Buffer_Stimulation_Stimulation_Date);
				{
					target.setUIntegerAsChildData(m_buffer.getStimulationDate(i));
					target.closeChild();
				}
				target.openChild(OVTK_NodeId_Buffer_Stimulation_Stimulation_Duration);
				{
					target.setUIntegerAsChildData(m_buffer.getStimulationDuration(i));
					target.closeChild();
				}
			}
			target.closeChild();
		}
		target.closeChild();
	}
	return TypeBase::Buffer::getEBML(target);
};

