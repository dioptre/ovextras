
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "TypeMatrix.h"

bool TypeMatrix::Header::getEBML(EBML::IWriterHelper& target) const
{ 
	target.openChild(OVTK_NodeId_Header_StreamedMatrix);
	{
		target.openChild(OVTK_NodeId_Header_StreamedMatrix_DimensionCount);
		{
			target.setUIntegerAsChildData(2);
			target.closeChild();
		}
		target.openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension);
		{
			target.openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension_Size);
			{
				target.setUIntegerAsChildData(m_header.getDimensionSize(0));
				target.closeChild();
			}
			target.closeChild();
		}
		target.openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension);
		{
			target.openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension_Size);
			{
				target.setUIntegerAsChildData(m_header.getDimensionSize(1));
				target.closeChild();
			}
			target.closeChild();
		}
		target.closeChild();
	}
	target.closeChild();

	return TypeBase::Header::getEBML(target);
};

bool TypeMatrix::Buffer::getEBML(EBML::IWriterHelper& target) const
{ 
	target.openChild(OVTK_NodeId_Buffer);
	{
		target.openChild(OVTK_NodeId_Buffer_StreamedMatrix);
		{
			target.openChild(OVTK_NodeId_Buffer_StreamedMatrix_RawBuffer);
			{
				target.setBinaryAsChildData(m_buffer.getBuffer(), m_buffer.getBufferElementCount() * sizeof(OpenViBE::float64));
				target.closeChild();
			}
			target.closeChild();
		}
		target.closeChild();
	}
	return TypeBase::Buffer::getEBML(target);
};

