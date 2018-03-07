//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include <mTGtkGLWidget.hpp>

// #include <mensia/advanced-visualizations/mIRuler.hpp>

namespace Mensia
{
namespace AdvancedVisualization
{
	class IRenderer;
	class IRendererContext; 
};
};

class TrackRenderer
{
public:
	struct Renderer {
		Mensia::AdvancedVisualization::IRenderer* m_pRenderer;
		Mensia::AdvancedVisualization::IRendererContext* m_pRendererContext;
	};

	bool initialize(GtkWidget *drawingArea);
	bool uninitialize(void);

	bool draw(void);
	bool drawLeft(void) { std::cout << "DrawLeft requested\n";  return true; };
	bool drawRight(void) { std::cout << "DrawRight requested\n";  return true; };
	bool drawBottom(void) { std::cout << "DrawBottom requested\n";  return true; };

	bool preDraw(void);
	bool postDraw(void);

	bool mouseButton(uint32_t width, uint32_t height, uint32_t button, uint32_t status) { std::cout << "mouseB requested\n";  return true; };
	bool mouseMotion(uint32_t width, uint32_t height) { // std::cout << "MouseMot requested\n";  
		return true; };
	bool keyboard(uint32_t width, uint32_t height, uint32_t value, bool unused) { std::cout << "keyb requested\n";  return true; };

	bool redraw(bool bImmediate=false);
	bool reshape(uint32_t width, uint32_t height);

	bool push(const OpenViBE::CMatrix& chunk);

	Renderer m_pRenderer;

protected:

	uint32_t m_ui32Width = 640;
	uint32_t m_ui32Height = 480;
	uint32_t m_ui32TextureId = 0;

	::GtkWidget* m_pViewport;
	::GtkWidget* m_pTop;
	::GtkWidget* m_pLeft;
	::GtkWidget* m_pRight;
	::GtkWidget* m_pBottom;
	::GtkWidget* m_pCornerLeft;
	::GtkWidget* m_pCornerRight;

//	Mensia::AdvancedVisualization::IRuler* m_pRuler = nullptr;
	void *m_pRuler = nullptr;

	typedef struct
	{
		float r;
		float g;
		float b;
	} TColor;

	TColor m_oColor;

	Mensia::AdvancedVisualization::TGtkGLWidget < TrackRenderer > m_oGtkGLWidget;
};

