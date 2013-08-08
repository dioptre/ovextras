#ifndef __GLabel_H__
#define __GLabel_H__

#include <cstring>

#include "glGObject.h"

namespace OpenViBEApplications
{
	class GLabel : public GObject
	{
	public:
		GLabel(); //can serve as a dummy label
		GLabel(OpenViBE::CString sourceFile, OpenViBE::float32 scaleSize);//should be made protected
		GLabel(const GLabel& glabel);
		virtual ~GLabel()
		{
			//std::cout << "GLabel destructor called on " << this->toString() << "\n";
		}		
		
		virtual GLabel& operator= (GLabel const& glabel);
		
		virtual GLabel* clone() const
		{
			return new GLabel(*this);
		}			
		
		virtual void draw();
		virtual std::string toString() const;
		
		//inherited setters
		virtual void setDimParameters(BoxDimensions dim);
		
		//additional setters and getters
		virtual void setForegroundColor(GColor color) { m_cForegroundColor = color; }
		virtual void setLabelScaleSize(OpenViBE::float32 labelScaleSize);
		//virtual void setSourceFile(OpenViBE::CString sourceFile) { m_sSourceFile = sourceFile; }
		
		virtual GColor getForegroundColor() const { return m_cForegroundColor; }
		virtual OpenViBE::float32 getLabelScaleSize() const { return m_f32LabelScaleSize; }
		virtual OpenViBE::float32 getMaxLabelSize() const { return m_f32MaxLabelSize; }
		virtual OpenViBE::CString getSourceFile() const { return m_sSourceFile; }
		
	protected:		
		virtual void computeLabelPosition();	
		virtual void computeMaximumLabelSize();
		virtual void generateGLDisplayLists();
		
	private:
		void assignHelper(const GLabel& glabel);
		
	protected:
		GColor m_cForegroundColor;
		OpenViBE::float32 m_f32LabelScaleSize;
		OpenViBE::float32 m_f32MaxLabelSize;		
		std::pair<OpenViBE::float32, OpenViBE::float32> m_pLabelPosition;
		OpenViBE::CString m_sSourceFile;
	};
};

#endif
