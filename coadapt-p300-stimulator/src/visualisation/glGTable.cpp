#include "glGTable.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEApplications;

void GTable::draw()
{
	GContainer::draw();
}

void GTable::addChild(GObject* child, float32 depth)
{
	float32 l_f32CellWidth = (float32)1.0f/m_ui32ColumnDimension;
	float32 l_f32CellHeight = (float32)1.0f/m_ui32RowDimension;
	float32 l_f32UpperLeftStartPointY = 1.0f-l_f32CellHeight;
	
	uint32 l_ui32ChildIndex = m_lChildren->size();
	uint32 i = l_ui32ChildIndex/m_ui32ColumnDimension;
	uint32 j = l_ui32ChildIndex%m_ui32ColumnDimension;
	
	GContainer::addChild(child, j*l_f32CellWidth, l_f32UpperLeftStartPointY-i*l_f32CellHeight, l_f32CellWidth, l_f32CellHeight, depth);
}

std::string GTable::toString() const
{
	return std::string("GTable");
}