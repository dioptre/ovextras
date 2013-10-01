#ifndef __GPictureSymbol_H__
#define __GPictureSymbol_H__

#include <string>
#include <boost/shared_ptr.hpp>

#include "glGLabel.h"
#include "OpenGLManagers/OpenGLTextureManager.h"

namespace OpenViBEApplications
{
	class GPictureSymbol : public GLabel
	{
	public:	
		
		//GPictureSymbol(boost::shared_ptr<SDL_Surface> surface, OpenViBE::float32 scaleSize) : GLabel("", scaleSize);			
		GPictureSymbol(OpenViBE::CString sourcePath, OpenViBE::float32 scaleSize);	
		GPictureSymbol(const GPictureSymbol& gsymbol);
		
		virtual ~GPictureSymbol();
		
		//virtual GPictureSymbol& operator= (GPictureSymbol const& gsymbol);
		
		//inherited functions
		virtual GPictureSymbol* clone() const { return new GPictureSymbol(*this);}			
		
		virtual void draw();
		virtual std::string toString() const;
		
		//inherited setters
		//virtual void setSourceFile(OpenViBE::CString sourceFile);
		virtual void setDimParameters(BoxDimensions dim);
		
		//additional setters
		
		//additional getters	
		
	protected:
		virtual void computeLabelPosition();
		virtual void generateGLDisplayLists();
		
	private:
		void initializeOpenGLTexture();
		void assignHelper(GPictureSymbol const& gpicturesymbol);
		
	protected:
		
		//boost::shared_ptr<SDL_Surface> m_pSurface;
		OpenViBE::boolean m_bImageLoaded;
		int m_iMode;	
		OpenGLTextureManager* m_pTextureManager;
	};
};

#endif
