#include "glGSymbol.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEApplications;

#define REFFONTSIZE 10

GSymbol::GSymbol(const char * symbol, boost::shared_ptr<FTFont> font, OpenViBE::float32 fontScaleSize) : GLabel("", fontScaleSize) 
{		
	m_sTextLabel = std::string(symbol);
	m_ftglFont = font;
	//std::cout << "Constructing symbol " << symbol << " font use count " << font.use_count() << "\n";
	m_ftglFont->FaceSize(static_cast<uint32>(m_f32LabelScaleSize*m_f32MaxLabelSize));
	//m_ftglFont->UseDisplayList(false);
	generateGLDisplayLists();
}		

GSymbol::GSymbol(const char * symbol, OpenViBE::CString fontPath, OpenViBE::float32 fontScaleSize) : GLabel(fontPath, fontScaleSize) 
{		
	m_sTextLabel = std::string(symbol);	
	m_ftglFont = createFont(fontPath.toASCIIString());
	m_ftglFont->FaceSize(static_cast<uint32>(m_f32LabelScaleSize*m_f32MaxLabelSize));
	//m_ftglFont->UseDisplayList(false);
	generateGLDisplayLists();
}

GSymbol::GSymbol(const GSymbol& gsymbol) : GLabel(gsymbol)
{
	assignHelper(gsymbol);
}

GSymbol::~GSymbol()
{
	m_ftglFont.reset();
	//std::cout << "Use count font " << m_ftglFont.use_count() << "\n";
}

/*GSymbol& GSymbol::operator= (GSymbol const& gsymbol)
{
	//std::cout << this->m_sTextLabel << " will be replaced by " << gsymbol.m_sTextLabel << "\n";
	if(this!=&gsymbol)
	{
		//std::cout << "operator= " << gsymbol.toString() << "\n";
		this->GLabel::operator=(gsymbol);
		assignHelper(gsymbol);
	}
	return *this;
}*/

void GSymbol::draw()
{
	GLabel::draw();
	//rendering font with given foreground color
	//glLoadIdentity();
	glColor3f(m_cForegroundColor.red,m_cForegroundColor.green,m_cForegroundColor.blue);
	//glPushMatrix();	
	glCallList(getGLResourceID(1));
	//glPopMatrix();	
	//std::cout << "Drawing " << m_sTextLabel.c_str() << "\n";
	//std::cout << "Drawing " << m_sTextLabel.c_str() << " foreground color " << m_cForegroundColor.red << "," << m_cForegroundColor.green << "," << m_cForegroundColor.blue
	//<< " background color " << getBackgroundColor().red << "," << getBackgroundColor().green << "," << getBackgroundColor().blue
	//<< " position " << m_pLabelPosition.first << "," << m_pLabelPosition.second << " font size " << (m_f32LabelScaleSize*m_f32MaxLabelSize) << " depth " << this->getDepth() 
	//<< " box x " << getX() << " box y " << getY() << " box width " << getWidth() << " box height " << getHeight() << "\n";	
}

void GSymbol::setDimParameters(BoxDimensions dim)
{
	//std::cout <<  toString() << "GSymbol::setDimParameters\n";
	GLabel::setDimParameters(dim);
	//computeLabelPosition(); //not really necessary as this is called by the computeMaximumFontSize
	//computeMaximumLabelSize();
      //setFontSize();	
	//generateGLDisplayLists();
}

/*void GSymbol::setLabelScaleSize(float32 fontScaleSize)
{
	m_f32LabelScaleSize = fontScaleSize;
	m_ftglFont->FaceSize(m_f32LabelScaleSize*m_f32MaxLabelSize);
	computeLabelPosition();
}	*/

/*void GSymbol::setSourceFile(OpenViBE::CString sourceFile)
{
	if (sourceFile!=m_sSourceFile)
	{
		m_sSourceFile = sourceFile;
		m_ftglFont.reset();
		m_ftglFont = createFont(sourceFile.toASCIIString());
		computeMaximumLabelSize();
		setFontSize();
		generateGLDisplayLists();		
	}
}*/

/*void GSymbol::setLabelScaleSize(OpenViBE::float32 labelScaleSize) 
{
	GLabel::setLabelScaleSize(labelScaleSize);
	deleteAndCreateGLResources();
	generateGLDisplayLists(); //should be in GLabel then we probably don't need this specialisation
}*/

void GSymbol::setTextLabel(const char * symbol)
{
	m_sTextLabel = std::string(symbol); 
	computeMaximumLabelSize();
      //setFontSize();
	deleteAndCreateGLResources();
	generateGLDisplayLists();
}

void GSymbol::computeMaximumLabelSize()
{
	//using a reference string and reference font size because even with monospace 
	//fonts the bounding box width of each character is still different
	m_ftglFont->FaceSize(REFFONTSIZE);
	std::string l_sRefString = "";
	for (uint32 i=0; i<m_sTextLabel.length(); i++)
		l_sRefString += "X";
	
	FTBBox l_oBoundingBox = m_ftglFont->BBox(l_sRefString.c_str());
	float32 l_fBoundingWidth = l_oBoundingBox.Upper().Xf()-l_oBoundingBox.Lower().Xf();
	float32 l_fWidthRatio = getWidth()/l_fBoundingWidth;
	m_f32MaxLabelSize = 0.9f*REFFONTSIZE*l_fWidthRatio;
	
	//for texture and vector fonts one opengl unit should be one point (1 pixel - 1 point, point is unit for font size)
	//see FAQ FTGL, notice "should", it seems it is only approximate
	m_ftglFont->FaceSize(static_cast<uint32>(m_f32MaxLabelSize));
	l_oBoundingBox = m_ftglFont->BBox(l_sRefString.c_str());
	float32 l_fBoundingHeight = l_oBoundingBox.Upper().Yf()-l_oBoundingBox.Lower().Yf();
	float32 l_fHeightRatio = getHeight()/l_fBoundingHeight;
	if (l_fHeightRatio<1)
		m_f32MaxLabelSize = 0.9f*m_f32MaxLabelSize*l_fHeightRatio;
}

void GSymbol::computeLabelPosition()
{
	FTBBox l_oBoundingBox = m_ftglFont->BBox(m_sTextLabel.c_str());
	float32 l_fBoundingWidth = l_oBoundingBox.Upper().Xf()-l_oBoundingBox.Lower().Xf();
	float32 l_fxOffset = (getWidth()-l_fBoundingWidth)/2.0f - l_oBoundingBox.Lower().Xf();	
	float32 l_fBoundingHeight = l_oBoundingBox.Upper().Yf()-l_oBoundingBox.Lower().Yf();
	float32 l_fyOffset = (getHeight()-l_fBoundingHeight)/2.0f - l_oBoundingBox.Lower().Yf();	
	m_pLabelPosition.first = getX()+l_fxOffset;
	m_pLabelPosition.second = getY()+l_fyOffset;
}

/*void GSymbol::setFontSize()
{
	m_ftglFont->FaceSize(static_cast<uint32>(m_f32LabelScaleSize*m_f32MaxLabelSize));
	computeLabelPosition();
}*/

void GSymbol::generateGLDisplayLists()
{
	//std::cout <<  toString() << "GSymbol::generateGLDisplayLists\n";
	GLabel::generateGLDisplayLists();
	m_ftglFont->FaceSize(static_cast<uint32>(m_f32LabelScaleSize*m_f32MaxLabelSize));
	computeLabelPosition();
      FTPoint l_ftPoint(m_pLabelPosition.first, m_pLabelPosition.second, this->getDepth()+0.01);
      glNewList(getGLResourceID(1),GL_COMPILE); 
		glLoadIdentity();
		m_ftglFont->Render(m_sTextLabel.c_str(), -1, l_ftPoint);
      glEndList();
}

void GSymbol::assignHelper(GSymbol const& gsymbol)
{
	this->m_sTextLabel = std::string(gsymbol.m_sTextLabel);
	this->m_ftglFont = gsymbol.m_ftglFont;
}

std::string GSymbol::toString() const
{
	std::string text;
	text = GLabel::toString(); 
	text += m_sTextLabel; 
	text += std::string(" ");
	return text;
}
