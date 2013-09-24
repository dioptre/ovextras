#include "glP300MainContainer.h"
#include "glGButton.h"
#include "glGSymbol.h"

#include <system/Time.h>

#include "../handlers/ovexP300KeyboardHandler.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEApplications;
using namespace std;

SDL_Surface* P300MainContainer::m_oSDLSurface;

P300MainContainer::P300MainContainer(P300InterfacePropertyReader* propertyObject, P300ScreenLayoutReader* layoutPropObject) : 
GContainer(0.0f,0.0f,propertyObject->getWidth(),propertyObject->getHeight()), m_pInterfacePropertyObject(propertyObject),
m_pScreenLayoutObject(layoutPropObject)
{
	#ifdef OUTPUT_TIMING
	timingFile = fopen(OpenViBE::Directories::getDataDir() + "/gl_draw_timing.txt","w");
	#endif
	initialize(layoutPropObject->getNumberOfStandardKeys());
}

P300MainContainer::P300MainContainer(const P300MainContainer& gcontainer) : GContainer(gcontainer)
{
	//std::cout << "Calling copy construction in P300MainContainer on " << gcontainer.toString() << "\n";
	this->m_gResultArea = (GTable*)this->getChild(1);
	this->m_gGridArea = (GContainer*)this->getChild(0);
	this->m_gPredictionArea = (GTable*)this->getChild(2);
	this->m_gDiodeArea = (GContainer*)this->getChild(3);
	this->m_gTargetArea = (GTable*)this->getChild(4);
	
	this->m_pInterfacePropertyObject = gcontainer.m_pInterfacePropertyObject;
	this->m_pScreenLayoutObject = gcontainer.m_pScreenLayoutObject;
	//this->m_oSDLSurface = gcontainer.m_oSDLSurface;	
	
	//this->m_pWordPredictionEngine = gcontainer.m_pWordPredictionEngine;
	this->m_pP300KeyboardHandler = gcontainer.m_pP300KeyboardHandler;
	#ifdef WORDPREDICTION
	this->m_pP300PredictionHandler = gcontainer.m_pP300PredictionHandler;
	#endif
	this->m_pP300UndoHandler = gcontainer.m_pP300UndoHandler;
	this->m_pP300BackspaceHandler = gcontainer.m_pP300BackspaceHandler;
	this->m_pP300ResultAreaHandler = gcontainer.m_pP300ResultAreaHandler;
	this->m_pP300TargetAreaHandler = gcontainer.m_pP300TargetAreaHandler;
}

P300MainContainer::~P300MainContainer()
{
	std::cout << "P300MainContainer deconstructor called\n";
	SDL_FreeSurface(m_oSDLSurface);
	#ifdef WORDPREDICTION
	delete m_pP300PredictionHandler;
	#endif
	delete m_pP300UndoHandler;
	delete m_pP300BackspaceHandler;
	delete m_pP300ResultAreaHandler;
	delete m_pP300TargetAreaHandler;
	delete m_pP300KeyboardHandler;

	#ifdef OUTPUT_TIMING
      fclose(timingFile);
	#endif
}		

void P300MainContainer::initialize(uint32 nGridCells)
{
	//initializeGL();//should be initialized first (weird things happen if it isn't)
	initializeGridArea(nGridCells);
	initializeResultArea();
	initializePredictionArea();	
	initializeDiodeArea();
	initializeTargetArea();
	
	m_pP300KeyboardHandler = new P300KeyboardHandler(m_gGridArea, m_gPredictionArea, m_pInterfacePropertyObject, m_pScreenLayoutObject);
	m_pP300KeyboardHandler->addObserver(m_pP300KeyboardHandler);
	m_pP300TargetAreaHandler = new P300TargetAreaHandler(m_gTargetArea, m_pScreenLayoutObject);	
	m_pP300KeyboardHandler->addActionObserver(CString("write"), m_pP300TargetAreaHandler);	
	
	m_pP300ResultAreaHandler = new P300ResultAreaHandler(m_gResultArea, m_pScreenLayoutObject);	
	m_pP300KeyboardHandler->addActionObserver(CString("write"), m_pP300ResultAreaHandler);
	m_pP300KeyboardHandler->addActionObserver(CString("wordprediction"), m_pP300ResultAreaHandler);
	m_pP300TargetAreaHandler->addObserver(m_pP300ResultAreaHandler);

	m_pP300UndoHandler = new P300UndoHandler();
	m_pP300KeyboardHandler->addActionObserver(CString("undo"), m_pP300UndoHandler);
	m_pP300KeyboardHandler->addActionObserver(CString("redo"), m_pP300UndoHandler);
	m_pP300ResultAreaHandler->addObserver(m_pP300UndoHandler);
	m_pP300UndoHandler->addObserver(m_pP300ResultAreaHandler);

	m_pP300BackspaceHandler = new P300BackspaceHandler();
	m_pP300KeyboardHandler->addActionObserver(CString("backspace"), m_pP300BackspaceHandler);
	m_pP300ResultAreaHandler->addObserver(m_pP300BackspaceHandler);
	m_pP300BackspaceHandler->addObserver(m_pP300ResultAreaHandler);
	
	//ENABLE WORD PREDICTION HERE
	#ifdef WORDPREDICTION
	m_pP300PredictionHandler = new P300PredictionboardHandler(m_gPredictionArea, m_pP300ResultAreaHandler->getResultBuffer(), m_pInterfacePropertyObject->getNGramDatabaseName());
	m_pP300ResultAreaHandler->addObserver(m_pP300PredictionHandler);
	m_pP300UndoHandler->addObserver(m_pP300PredictionHandler);
	m_pP300BackspaceHandler->addObserver(m_pP300PredictionHandler);
	m_pP300PredictionHandler->addObserver(m_pP300KeyboardHandler);
	#endif
}

void P300MainContainer::initializeGL(OpenViBE::boolean fullScreen, OpenViBE::float32 width, OpenViBE::float32 height)
{
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) 
	{
		std::cout << "Failed to init SDL\n";
		//return false;
	}

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,    	    8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,  	    8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,   	    8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,  	    8);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  	    16);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,		    32);

	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,	    8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,	8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,	    8);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,	8);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,  1);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,  2);

	uint32 l_ui32SDLFlags = SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL;
	if (fullScreen)
		l_ui32SDLFlags |= SDL_FULLSCREEN;

 	if((m_oSDLSurface = SDL_SetVideoMode(static_cast<int>(width), static_cast<int>(height), 32, l_ui32SDLFlags)) == NULL) 
	{
		//return false;
		std::cout << "Failed to create SDL surface\n";
	}

	glClearColor(0, 0, 0, 0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	do_ortho(width, height);	
}

void P300MainContainer::do_ortho(OpenViBE::float32 width, OpenViBE::float32 height)
{
	/*GLdouble size;
	GLdouble aspect;
	uint32 w = m_ui32WindowWidth;
	uint32 h = m_ui32WindowHeight;
	aspect = (GLdouble)w / (GLdouble)h;m_ConditionVariable.wait(lock);
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	size = (GLdouble)((w >= h) ? w : h) / 2.0;
	if (w <= h) 
	{
		aspect = (GLdouble)h/(GLdouble)w;
		gluOrtho2D(0, 2*size, 0, 2*size*aspect);
	}
	else {
		aspect = (GLdouble)w/(GLdouble)h;
		gluOrtho2D(0, 2*size*aspect, 0, 2*size);
	}
	glScaled(aspect, aspect, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_TEXTURE_2D);
	glLoadIdentity();*/
	glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, width, 0, height, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);

	//glEnable(GL_TEXTURE_2D);

	glLoadIdentity();
	
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);	
}

void P300MainContainer::initializeGridArea(uint32 nCells)
{
	//std::cout << "initializeGridArea\n";
	//m_gGridArea = new GTable(nCells);	
	if (m_pScreenLayoutObject->isKeyboardTable())
		m_gGridArea = new GTable(nCells);
	else
		m_gGridArea = new GContainer();
	m_gGridArea->setBackgroundColor(m_pScreenLayoutObject->getDefaultBackgroundColor(NOFLASH));
	BoxDimensions dim = m_pScreenLayoutObject->getP300KeyboardDimensions();
	this->addChild(m_gGridArea, dim.x, dim.y, dim.width, dim.height, 0.05f);	
}
void P300MainContainer::initializeResultArea()
{
	//std::cout << "initializeResultArea\n";
	m_gResultArea = new GTable((uint32)1,(uint32)30);	
	m_gResultArea->setBackgroundColor(m_pScreenLayoutObject->getDefaultBackgroundColor(NOFLASH));
	BoxDimensions dim = m_pScreenLayoutObject->getResultAreaDimensions();
	this->addChild(m_gResultArea, dim.x, dim.y, dim.width, dim.height, 0.05f);
	/*for (uint32 i=0; i<m_gResultArea->getColumnDimension();i++)
	{
		GLabel* l_pLabel = new GLabel();//ATTENTION this causes a memory leak as these objects won't be clean up by the container (these pointers will point somewhere else because of the target handler)
		l_pLabel->setBackgroundColor(m_pScreenLayoutObject->getDefaultBackgroundColor(NOFLASH));
		m_gResultArea->addChild(l_pLabel, 0.05f);
	}*/
}
void P300MainContainer::initializeTargetArea()
{
	m_gTargetArea = new GTable((uint32)1,(uint32)30);	
	m_gTargetArea->setBackgroundColor(m_pScreenLayoutObject->getDefaultBackgroundColor(NOFLASH));
	BoxDimensions dim = m_pScreenLayoutObject->getTargetAreaDimensions();
	this->addChild(m_gTargetArea, dim.x, dim.y, dim.width, dim.height, 0.05f);
}
void P300MainContainer::initializePredictionArea()
{
	//std::cout << "initializePredictionArea\n";
	m_gPredictionArea = new GTable(m_pScreenLayoutObject->getPredictionAreaRows(),m_pScreenLayoutObject->getPredictionAreaColumns());	
	m_gPredictionArea->setBackgroundColor(m_pScreenLayoutObject->getDefaultBackgroundColor(NOFLASH));
	BoxDimensions dim = m_pScreenLayoutObject->getPredictionAreaDimensions();
	this->addChild(m_gPredictionArea, dim.x, dim.y, dim.width, dim.height, 0.05f);		
}

void P300MainContainer::initializeDiodeArea()
{
	//std::cout << "initializePredictionArea\n";
	m_gDiodeArea = new GContainer();	
	m_gDiodeArea->setBackgroundColor(m_pScreenLayoutObject->getDefaultBackgroundColor(NOFLASH));
	this->addChild(m_gDiodeArea, 0.75f, 0.05f, 0.2f, 0.15f, 0.05f);		
}

void P300MainContainer::changeBackgroundColorDiodeArea(GColor bColor)
{
	m_gDiodeArea->setBackgroundColor(bColor);
}

void P300MainContainer::drawAndSync(ITagger * tagger, queue<OpenViBE::uint32> &l_qEventQueue)
{
	this->draw();

	while (!l_qEventQueue.empty())
	{
		//m_pInterfacePropertyObject->getKernelContext()->getLogManager() << LogLevel_Info << "Writing stimulus " << l_qEventQueue.front() << " to parallel port\n";
		tagger->write(l_qEventQueue.front());
		//std::cout << "Finished writing to tagger\n";
		l_qEventQueue.pop();
	}
}

void P300MainContainer::draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	#ifdef OUTPUT_TIMING
      float64 l_f64TimeBefore = float64((System::Time::zgetTime()>>22)/1024.0);
	fprintf(timingFile, "%f\n",l_f64TimeBefore);	
	#endif
	
	GContainer::draw();
	
	#ifdef OUTPUT_TIMING
	float64 l_f64TimeAfter = float64((System::Time::zgetTime()>>22)/1024.0);
	fprintf(timingFile, "%f\n",l_f64TimeAfter);
	#endif
	
	SDL_GL_SwapBuffers();
	glFinish();
}

P300KeyboardHandler* P300MainContainer::getKeyboardHandler()
{
	return m_pP300KeyboardHandler;
}

std::string P300MainContainer::toString() const
{
	return std::string("P300MainContainer");
}
