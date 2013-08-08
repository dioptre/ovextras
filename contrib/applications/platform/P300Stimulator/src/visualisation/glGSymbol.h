#ifndef __GSymbol_H__
#define __GSymbol_H__

#include <cstring>
#include <boost/shared_ptr.hpp>

#include "glGLabel.h"

namespace OpenViBEApplications
{
	class GSymbol : public GLabel
	{
	public:
		
		GSymbol(const char * symbol, boost::shared_ptr<FTFont> font, OpenViBE::float32 fontScaleSize);		
		GSymbol(const char * symbol, OpenViBE::CString fontPath, OpenViBE::float32 fontScaleSize);	
		GSymbol(const GSymbol& gsymbol);
		virtual ~GSymbol();
		
		//virtual GSymbol& operator= (GSymbol const& gsymbol);	
		
		//inherited functions
		virtual GSymbol* clone() const
		{
			return new GSymbol(*this);
		}			
		
		virtual void draw();
		virtual std::string toString() const;
		
		//inherited setters
		virtual void setDimParameters(BoxDimensions dim);
		//virtual void setSourceFile(OpenViBE::CString sourceFile);
		//virtual void setLabelScaleSize(OpenViBE::float32 labelScaleSize);
		
		//additional setters
		//virtual void setFont(boost::shared_ptr<FTFont> font) { this->m_ftglFont = font; }
		virtual void setTextLabel(const char * symbol);
		
		//additional getters
		virtual std::string getTextLabel() const { return m_sTextLabel; }	
		virtual boost::shared_ptr<FTFont> getFont() const { return m_ftglFont; }	
		
	protected:
		//inherited functions
		virtual void computeLabelPosition();
		virtual void computeMaximumLabelSize();
		virtual void generateGLDisplayLists();
		
	private:
		void assignHelper(GSymbol const& gsymbol);
		boost::shared_ptr<FTFont> createFont(const char* fontPath)
		{
			//return boost::shared_ptr<FTFont>(new FTGLPixmapFont(fontPath)); 
			return boost::shared_ptr<FTFont>(new FTGLPolygonFont(fontPath));
		}
		
		//void setFontSize();
		
	protected:
		std::string m_sTextLabel;
		boost::shared_ptr<FTFont> m_ftglFont;	
	};
};

#endif
