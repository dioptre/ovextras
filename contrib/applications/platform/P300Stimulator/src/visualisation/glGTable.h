#ifndef __GTable_H__
#define __GTable_H__

#include <cmath>

#include "glGContainer.h"

namespace OpenViBEApplications
{
	class GTable : public GContainer
	{
	public:
		GTable() : GContainer() {}
		/*GTable(OpenViBE::float32 w, OpenViBE::float32 h) : GContainer(w,h) {}			
		GTable(OpenViBE::float32 x, OpenViBE::float32 y, OpenViBE::float32 w, OpenViBE::float32 h) : GContainer(x,y,w,h) {}	*/		
		
		GTable(OpenViBE::uint32 nElements) : GContainer()
		{
			m_ui32RowDimension = (OpenViBE::uint32)std::floor(0.5+std::sqrt((double)nElements));
			m_ui32ColumnDimension = (OpenViBE::uint32)std::ceil((double)nElements/(double)m_ui32RowDimension);	
		}
		
		GTable(OpenViBE::uint32 nRows, OpenViBE::uint32 nCols) : GContainer()
		{
			m_ui32RowDimension = nRows;
			m_ui32ColumnDimension = nCols;	
		}
		
		GTable(const GTable& gtable) : GContainer(gtable)
		{		
			//std::cout << "Calling copy construction in GTable on " << gtable.toString() << "\n";
			m_ui32RowDimension = gtable.m_ui32RowDimension;
			m_ui32ColumnDimension = gtable.m_ui32ColumnDimension;
		}
		
		virtual GTable& operator= (GTable const& mainTable)
		{
			if(this!=&mainTable)
			{
				this->GContainer::operator=(mainTable);
				this->m_ui32RowDimension = mainTable.m_ui32RowDimension;
				this->m_ui32ColumnDimension = mainTable.m_ui32ColumnDimension;
			}
			return *this;
		}		
		
		virtual ~GTable()
		{
			//std::cout << "GTable deconstructor called\n";
		}		
		
		virtual void addChild(GObject* child, OpenViBE::float32 depth);
		virtual GObject*& getChild(OpenViBE::uint32 rowIndex, OpenViBE::uint32 colIndex) const 
		{ 
			OpenViBE::uint32 childIndex = rowIndex*m_ui32ColumnDimension+colIndex;
			return this->getChild(childIndex); 
		}
		virtual OpenViBE::uint32 getRowDimension() { return m_ui32RowDimension; }
		virtual OpenViBE::uint32 getColumnDimension() { return m_ui32ColumnDimension; }
		
		//inherited functions
		virtual GTable* clone() const
		{
			return new GTable(*this);
		}		
		virtual void draw();
		virtual std::string toString() const;
		
		//inherited setters
		virtual void setDimParameters(BoxDimensions dim)
		{
			GContainer::setDimParameters(dim);
		}		
		virtual void setBackgroundColor(GColor color) { GContainer::setBackgroundColor(color); }	
		
		//inherited getters
		virtual OpenViBE::float32 getX() const { return GContainer::getX(); }
		virtual OpenViBE::float32 getY() const { return GContainer::getY(); }
		virtual OpenViBE::float32 getWidth() const { return GContainer::getWidth(); }
		virtual OpenViBE::float32 getHeight() const { return GContainer::getHeight(); }
		virtual OpenViBE::float32 getDepth() const { return GContainer::getDepth(); }
		virtual GColor getBackgroundColor() const { return GContainer::getBackgroundColor(); }			
		virtual GObject*& getChild(OpenViBE::uint32 childIndex) const { return GContainer::getChild(childIndex); }
		virtual OpenViBE::uint32 getNumberOfChildren() const { return GContainer::getNumberOfChildren(); }
		
	protected:
		OpenViBE::uint32 m_ui32RowDimension;
		OpenViBE::uint32 m_ui32ColumnDimension;
	};
};

#endif
