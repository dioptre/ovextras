//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 
// @todo add horizontal scaling support
// @todo add event handlers
// @todo add ruler, stimulations, channel names, a million of other things

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include <mensia/advanced-visualization.h>
#include <m_GtkGL.hpp>

#include "TrackRenderer.h"

#include <system/ovCTime.h>

using namespace Mensia;

AdvancedVisualization::IRendererContext& getSingletonRendererContext(void);

bool TrackRenderer::initialize(GtkWidget *drawingArea)
{
	AdvancedVisualization::IRenderer* l_pRenderer = AdvancedVisualization::IRenderer::create(AdvancedVisualization::IRenderer::RendererType_Line, false);
	if(l_pRenderer == NULL) return false;

	// Creates renderer context
	AdvancedVisualization::IRendererContext* l_pRendererContext = AdvancedVisualization::IRendererContext::create(&getSingletonRendererContext());

	l_pRendererContext->clear();
//	m_pRendererContext->setTranslucency(float(m_f64Translucency));
//	m_pRendererContext->setFlowerRingCount(m_ui64FlowerRingCount);
	l_pRendererContext->setTimeScale(1.0);
	l_pRendererContext->setElementCount(11);
	l_pRendererContext->scaleBy(1);
	l_pRendererContext->setTranslucency(1.0);
//	m_pRendererContext->setAxisDisplay(m_bShowAxis);
//	m_pRendererContext->setPositiveOnly(m_bIsPositive);
//	m_pRendererContext->setParentRendererContext(&getContext());
//	m_pRendererContext->setTimeLocked(m_bIsTimeLocked);
//	m_pRendererContext->setXYZPlotDepth(m_bXYZPlotHasDepth);

	m_pRenderer.m_pRenderer = l_pRenderer;
	m_pRenderer.m_pRendererContext = l_pRendererContext;

	GtkBuilder* pBuilder=gtk_builder_new(); 
	const OpenViBE::CString l_sFilename = OpenViBE::Directories::getDataDir() + "/applications/tracker/advanced-visualization.ui";
	if(!gtk_builder_add_from_file(pBuilder, l_sFilename, NULL)) {
		std::cout << "Problem loading [" << l_sFilename << "]\n";
		return false;
	}

	::GtkWidget* l_pWindow=GTK_WIDGET(::gtk_builder_get_object(pBuilder, "window"));
	::GtkWidget* l_pMain=GTK_WIDGET(::gtk_builder_get_object(pBuilder, "table"));
	::GtkWidget* l_pToolbar=GTK_WIDGET(::gtk_builder_get_object(pBuilder, "toolbar-window"));

	m_pViewport = drawingArea;

	// m_pViewport=GTK_WIDGET(::gtk_builder_get_object(pBuilder, "viewport"));
	m_pTop=GTK_WIDGET(::gtk_builder_get_object(pBuilder, "label_top"));
	m_pLeft=GTK_WIDGET(::gtk_builder_get_object(pBuilder, "drawingarea_left"));
	m_pRight=GTK_WIDGET(::gtk_builder_get_object(pBuilder, "drawingarea_right"));
	m_pBottom=GTK_WIDGET(::gtk_builder_get_object(pBuilder, "drawingarea_bottom"));
	m_pCornerLeft=GTK_WIDGET(::gtk_builder_get_object(pBuilder, "label_corner_left"));
	m_pCornerRight=GTK_WIDGET(::gtk_builder_get_object(pBuilder, "label_corner_right"));
	
	m_oGtkGLWidget.initialize(*this, m_pViewport, m_pLeft, m_pRight, m_pBottom);
	m_oGtkGLWidget.setPointSmoothingActive(false);

	// @fixme note that some of these widgets are not shown at the moment, but we realize
	// them all the same so we don't get errors.
	gtk_widget_realize(m_pTop);
	gtk_widget_realize(m_pLeft);
	gtk_widget_realize(m_pRight);
	gtk_widget_realize(m_pBottom);
	gtk_widget_realize(m_pViewport);
	gtk_widget_realize(l_pMain);
	gtk_widget_realize(l_pWindow);
	gtk_widget_realize(l_pToolbar);

	m_oColor.r = 1.0;
	m_oColor.g = 1.0;
	m_oColor.b = 1.0;

	g_object_unref(pBuilder);
	pBuilder=NULL;

	return true;
}

bool TrackRenderer::uninitialize(void)
{

	AdvancedVisualization::IRenderer::release(m_pRenderer.m_pRenderer);
	AdvancedVisualization::IRendererContext::release(m_pRenderer.m_pRendererContext);
	m_pRenderer.m_pRenderer = NULL;
	m_pRenderer.m_pRendererContext = NULL;

	return true;
}

bool TrackRenderer::reset(uint32_t totalChannelCount, uint32_t totalSampleCount)
{
	m_pRenderer.m_pRenderer->clear(0);
	m_pRenderer.m_pRendererContext->clear();

	for(uint32_t i=0;i<totalChannelCount;i++)
	{
		char name[512];
		sprintf(name, "%d",i);
		m_pRenderer.m_pRendererContext->addChannel(name);
	}

	m_pRenderer.m_pRenderer->setChannelCount(totalChannelCount);
	m_pRenderer.m_pRenderer->setSampleCount(totalSampleCount);

	m_pRenderer.m_pRenderer->rebuild(*m_pRenderer.m_pRendererContext);

	return true;
}

bool TrackRenderer::push(const OpenViBE::CMatrix& chunk)
{

	/*
	if(chunk.getDimensionCount() == 2 && 
		(chunk.getDimensionSize(0) != m_pRenderer.m_pRenderer->getChannelCount() 
		   || m_pRenderer.m_pRenderer->getSampleCount() % chunk.getDimensionSize(1) !=0))
	{

	}
	*/

	// @todo refactor to separate 64bit->32bit float conversion routine; use fixed memory buffer
	uint64_t numFloats = chunk.getBufferElementCount();
	float* floatBuf = new float[numFloats];
	for(uint32_t j=0;j<chunk.getBufferElementCount();j++)
	{
		floatBuf[j]=static_cast<float>(chunk.getBuffer()[j]);
		// floatBuf[j]=(System::Math::randomFloat32BetweenZeroAndOne()-0.5)*2.0;
	}

	m_pRenderer.m_pRenderer->feed(floatBuf, chunk.getDimensionSize(1));

	delete[] floatBuf;

	return true;
}

bool TrackRenderer::redraw(bool bImmediate /* = false */)
{
	const uint32_t numSamples =  m_pRenderer.m_pRenderer->getSampleCount();
	const float pixelsPerSample = 0.01;
	const float pixelsPerChannel = 100;

	// const float widthNeeded =  numSamples * pixelsPerSample;
	// const float heightNeeded = m_pRenderer.m_pRenderer->getChannelCount() * pixelsPerChannel;
	// gtk_drawing_area_size(GTK_DRAWING_AREA(m_pViewport), uint32_t(widthNeeded), uint32_t(heightNeeded));

	bool m_bRedrawNeeded = true;
	uint64_t m_ui64LastRenderTime = 0;

	bool l_bImmediate = bImmediate;
	uint64_t l_ui64CurrentTime = System::Time::zgetTime();
	if(m_bRedrawNeeded || l_ui64CurrentTime - m_ui64LastRenderTime > ((1LL<<32)/16))
	{
//					l_bImmediate |= (l_ui64CurrentTime - m_ui64LastRenderTime > ((1LL<<32)/4));
		m_oGtkGLWidget.redraw(l_bImmediate);
		m_oGtkGLWidget.redrawLeft(l_bImmediate);
		m_oGtkGLWidget.redrawRight(l_bImmediate);
		m_oGtkGLWidget.redrawBottom(l_bImmediate);
		m_ui64LastRenderTime = l_ui64CurrentTime;
		m_bRedrawNeeded = false;
	}



	return true;
}

bool TrackRenderer::reshape(uint32_t width, uint32_t height)
{
	m_ui32Width=uint32_t(width);
	m_ui32Height=uint32_t(height);
	m_pRenderer.m_pRendererContext->setAspect(width*1.f/height);

	return true;
}

bool TrackRenderer::draw(void)
{
	TrackRenderer::preDraw();

	::glPushAttrib(GL_ALL_ATTRIB_BITS);
	::glColor4f(m_oColor.r, m_oColor.g, m_oColor.b, m_pRenderer.m_pRendererContext->getTranslucency());
	m_pRenderer.m_pRenderer->render(*m_pRenderer.m_pRendererContext);
	::glPopAttrib();

	TrackRenderer::postDraw();

	return true;
}

bool TrackRenderer::preDraw(void)
{
// 	this->updateRulerVisibility();

	auto m_sColorGradient=OpenViBE::CString("0:0,0,0; 100:100,100,100");

	if(!m_ui32TextureId)
	{
		m_ui32TextureId=m_oGtkGLWidget.createTexture(m_sColorGradient.toASCIIString());
	}
	::glBindTexture(GL_TEXTURE_1D, m_ui32TextureId);

	m_pRenderer.m_pRendererContext->setAspect(m_pViewport->allocation.width * 1.f / m_pViewport->allocation.height);

	return true;
}


bool TrackRenderer::postDraw(void)
{
	::glPushAttrib(GL_ALL_ATTRIB_BITS);
//	if(m_pRuler) m_pRuler->doRender();
	::glPopAttrib();

	return true;
}


AdvancedVisualization::IRendererContext& getSingletonRendererContext(void)
{
	static AdvancedVisualization::IRendererContext* l_pMasterRendererContext = NULL;
	if(!l_pMasterRendererContext)
	{
		l_pMasterRendererContext = AdvancedVisualization::IRendererContext::create();
	}
	return *l_pMasterRendererContext;
}

