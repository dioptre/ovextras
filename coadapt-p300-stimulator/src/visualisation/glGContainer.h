#ifndef __GContainer_OV_H__
#define __GContainer_OV_H__

#include "glGObject.h"

#include <vector>

namespace OpenViBEApplications
{
	class GContainer : public GObject
	{
		
	static const OpenViBE::uint32 VertexCount = 4;	
	static const OpenViBE::uint32 VBOSize = 3*VertexCount; 
	
	public:
		GContainer();
		/*GContainer(OpenViBE::float32 w, OpenViBE::float32 h) : GObject(w,h) 
		{
			m_lChildren = new std::vector<GObject*>();
		}	*/		
		GContainer(OpenViBE::float32 x, OpenViBE::float32 y, OpenViBE::float32 w, OpenViBE::float32 h);
		GContainer(const GContainer& gcontainer);
		virtual ~GContainer();
		virtual GContainer& operator= (GContainer const& mainContainer);	
		
		//inherited functions	
		virtual GContainer* clone() const
		{
			return new GContainer(*this);
		}
		virtual void draw();
		virtual std::string toString() const;

		//inherited setters
		virtual void setDimParameters(BoxDimensions dim);	
		virtual void setBackgroundColor(GColor color) { GObject::setBackgroundColor(color); }	
		
		//inherited getters
		virtual OpenViBE::float32 getX() const { return GObject::getX(); }
		virtual OpenViBE::float32 getY() const { return GObject::getY(); }
		virtual OpenViBE::float32 getWidth() const { return GObject::getWidth(); }
		virtual OpenViBE::float32 getHeight() const { return GObject::getHeight(); }
		virtual OpenViBE::float32 getDepth() const { return GObject::getDepth(); }
		virtual GColor getBackgroundColor() const { return GObject::getBackgroundColor(); }	
		
		//additional functions
		virtual void addChild(GObject* child, OpenViBE::float32 offsetX, OpenViBE::float32 offsetY, 
					OpenViBE::float32 width, OpenViBE::float32 height, OpenViBE::float32 depth);		
		virtual void removeChild(OpenViBE::uint32 childIndex);
		virtual void removeAllChildren();
		
		//additional getters
		virtual GObject*& getChild(OpenViBE::uint32 childIndex) const { return m_lChildren->at(childIndex); }
		virtual OpenViBE::uint32 getNumberOfChildren() const { return m_lChildren->size(); }
		
		/*virtual void setChild(GObject * gobject, OpenViBE::uint32 childIndex)
		{
			
		}*/
	protected:
		virtual void generateVBOBuffers();
		virtual void generateGLDisplayLists();
            virtual void fillVertexBuffer(GLfloat* buffer, OpenViBE::float32 depth);
		virtual void fillColorBuffer(GLfloat* buffer, const GColor& l_color);
		
	protected:	
		std::vector<GObject*> * m_lChildren;
		//OpenViBE::uint32 m_ui32NumberOfChildren;
		
	};
	
};

#endif
