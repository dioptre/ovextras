#include "ovpCROCCurveDraw.h"

#include <iostream>


using namespace OpenViBE;
using namespace OpenViBEPlugins::Evaluation;

void size_allocate_cb(::GtkWidget *pWidget, ::GdkRectangle *pRectangle, gpointer pUserData)
{
	static_cast<CROCCurveDraw*>(pUserData)->resizeEvent(pRectangle);
}

void area_expose_cb(::GtkWidget* pWidget, ::GdkEventExpose* pEvent, gpointer pUserData)
{
	static_cast<CROCCurveDraw*>(pUserData)->exposeEnvent();
}

CROCCurveDraw::CROCCurveDraw(::GtkNotebook* pNotebook, OpenViBE::uint32 ui32ClassIndex, CString &rClassName)
{
	m_ui32Margin = 50;
	m_ui32ClassIndex = ui32ClassIndex;
	m_bHasBeenInit = false;
	m_bHasBeenExposed = false;
	m_pDrawableArea = gtk_drawing_area_new();
	::gtk_widget_set_size_request(m_pDrawableArea, 700, 600);

	g_signal_connect(G_OBJECT(m_pDrawableArea), "expose_event", G_CALLBACK(area_expose_cb), this);
	g_signal_connect(G_OBJECT(m_pDrawableArea), "size-allocate", G_CALLBACK(size_allocate_cb), this);

	::GtkWidget* l_pLabel = gtk_label_new(rClassName.toASCIIString());
	gtk_notebook_append_page(pNotebook, m_pDrawableArea, l_pLabel);


	//get left ruler widget's font description
	PangoContext * l_pPangoContext = gtk_widget_get_pango_context(m_pDrawableArea);
	PangoFontDescription * l_pFontDescription = pango_context_get_font_description(l_pPangoContext);

	//adapt the allocated height per label to the font's height (plus 4 pixel to add some spacing)
	if(pango_font_description_get_size_is_absolute(l_pFontDescription))
	{
		m_ui64PixelsPerLeftRulerLabel = pango_font_description_get_size(l_pFontDescription) + 4;
	}
	else
	{
		m_ui64PixelsPerLeftRulerLabel = pango_font_description_get_size(l_pFontDescription)/PANGO_SCALE + 4;
	}
}

CROCCurveDraw::~CROCCurveDraw()
{
}

std::vector < CCoordinate> &CROCCurveDraw::getCoordinateVector()
{
	return m_oCoordinateList;
}

void CROCCurveDraw::generateCurve()
{
	GtkAllocation l_pAllocation;
	gtk_widget_get_allocation(m_pDrawableArea, &l_pAllocation);

	uint32 l_ui32ChartWidth = l_pAllocation.width - 2 * m_ui32Margin;
	uint32 l_ui32ChartHeight = l_pAllocation.height - 2 * m_ui32Margin;

	m_oPointList.clear();
	for(size_t i = 0; i < m_oCoordinateList.size(); ++i)
	{
		GdkPoint l_oPoint;
		l_oPoint.x = m_oCoordinateList[i].first * l_ui32ChartWidth + m_ui32Margin;
		l_oPoint.y = (l_pAllocation.height - m_ui32Margin) - m_oCoordinateList[i].second * l_ui32ChartHeight;
		m_oPointList.push_back(l_oPoint);
	}
	m_bHasBeenInit = true;
}

void CROCCurveDraw::exposeEnvent()
{
	m_bHasBeenExposed = true;
	redraw();
}

void CROCCurveDraw::forceRedraw()
{
	redraw();
}

void CROCCurveDraw::resizeEvent(::GdkRectangle *pRectangle)
{
	GtkAllocation alloc;
	gtk_widget_get_allocation(m_pDrawableArea, &alloc);

	if(!m_bHasBeenInit)
	{
		return;
	}

	generateCurve();
	redraw();
}

void CROCCurveDraw::redraw()
{
	if(!m_bHasBeenInit || !m_bHasBeenExposed)
	{
		return;
	}

	GtkAllocation l_pAllocation;
	gtk_widget_get_allocation(m_pDrawableArea, &l_pAllocation);

	gdk_draw_rectangle (m_pDrawableArea->window, GTK_WIDGET(m_pDrawableArea)->style->white_gc, TRUE, 0, 0,l_pAllocation.width, l_pAllocation.height);


	GdkColor l_oLineColor = {0, 35000, 35000, 35000};
	GdkGC *l_pGc = gdk_gc_new((m_pDrawableArea)->window);
	gdk_gc_set_rgb_fg_color(l_pGc, &l_oLineColor);


	//Left ruler
	gdk_draw_line((m_pDrawableArea)->window, l_pGc,  m_ui32Margin, m_ui32Margin, m_ui32Margin, l_pAllocation.height- m_ui32Margin);
	drawLeftMark(m_ui32Margin, m_ui32Margin, "1");
	drawLeftMark(m_ui32Margin, l_pAllocation.height/2, "0.5");
	drawLeftMark(m_ui32Margin, l_pAllocation.height- m_ui32Margin, "0");

	//*** Black magic section to rotate the text of the left ruler. The solution comes from the internet (gtk doc), it works so
	// don't touch it unless you are sure of what you are doing
	PangoContext *l_pContext = gtk_widget_get_pango_context(m_pDrawableArea);
	PangoLayout *l_pLayout;
	PangoFontDescription *desc;
	GdkScreen *l_pScreen = gdk_drawable_get_screen (m_pDrawableArea->window);
	PangoRenderer *l_pRenderer = gdk_pango_renderer_get_default (l_pScreen);
	gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (l_pRenderer), m_pDrawableArea->window);
	GdkGC* l_pRotationGc = gdk_gc_new (m_pDrawableArea->window);
	gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (l_pRenderer), l_pRotationGc);
	int l_iWidth, l_iHeight;
	PangoMatrix l_oMatrix = PANGO_MATRIX_INIT;
	pango_matrix_translate (&l_oMatrix, 0,(l_pAllocation.height +100)/ 2);
	l_pLayout = pango_layout_new (l_pContext);
	pango_layout_set_text (l_pLayout, "True Positive Rate", -1);
	desc = pango_context_get_font_description(l_pContext);
	pango_layout_set_font_description (l_pLayout, desc);
	GdkColor l_oColor = {0, 0, 0, 0};
	gdk_pango_renderer_set_override_color (GDK_PANGO_RENDERER (l_pRenderer), PANGO_RENDER_PART_FOREGROUND, &l_oColor);

	pango_matrix_rotate (&l_oMatrix, 90);
	pango_context_set_matrix (l_pContext, &l_oMatrix);
	pango_layout_context_changed (l_pLayout);
	pango_layout_get_size (l_pLayout, &l_iWidth, &l_iHeight);
	pango_renderer_draw_layout (l_pRenderer, l_pLayout, 15, (l_pAllocation.height	+ l_iHeight)/ 2);

	gdk_pango_renderer_set_override_color (GDK_PANGO_RENDERER (l_pRenderer), PANGO_RENDER_PART_FOREGROUND, NULL);
	gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (l_pRenderer), NULL);
	gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (l_pRenderer), NULL);

	pango_matrix_rotate (&l_oMatrix, -90);
	pango_context_set_matrix (l_pContext, &l_oMatrix);
	pango_layout_context_changed (l_pLayout);

	g_object_unref (l_pLayout);
	g_object_unref (l_pContext);
	g_object_unref (l_pRotationGc);
	//** End of black magic section

	//Bottom ruler
	gdk_draw_line((m_pDrawableArea)->window, l_pGc,  m_ui32Margin, l_pAllocation.height- m_ui32Margin, l_pAllocation.width - m_ui32Margin, l_pAllocation.height- m_ui32Margin);
	drawBottomMark(m_ui32Margin,  l_pAllocation.height- m_ui32Margin, "0");
	drawBottomMark(l_pAllocation.width/2,  l_pAllocation.height- m_ui32Margin, "0.5");
	drawBottomMark(l_pAllocation.width - m_ui32Margin,  l_pAllocation.height- m_ui32Margin, "1");

	int l_iTextW;
	int l_iTextH;
	PangoLayout * l_pText = gtk_widget_create_pango_layout(m_pDrawableArea, "False positive rate");
	pango_layout_set_justify(l_pText, PANGO_ALIGN_CENTER);
	pango_layout_get_pixel_size(l_pText, &l_iTextW, &l_iTextH);
	gdk_draw_layout(m_pDrawableArea->window, GTK_WIDGET(m_pDrawableArea)->style->black_gc,	l_pAllocation.width/2 - l_iTextW/2, l_pAllocation.height-15 , l_pText);
	g_object_unref(l_pText);


	if(m_oPointList.size() != 0)
	{
		gdk_draw_lines((m_pDrawableArea)->window, GTK_WIDGET(m_pDrawableArea)->style->black_gc, &(m_oPointList[0]), m_oPointList.size());
	}

	gdk_gc_set_line_attributes(l_pGc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
	gdk_draw_line((m_pDrawableArea)->window, l_pGc, m_ui32Margin, l_pAllocation.height- m_ui32Margin, l_pAllocation.width - m_ui32Margin, m_ui32Margin);

	g_object_unref(l_pGc);
}

void CROCCurveDraw::drawLeftMark(OpenViBE::uint32 ui32W, OpenViBE::uint32 ui32H, const char *sLabel)
{
	int l_iTextW;
	int l_iTextH;
	PangoLayout * l_pText = gtk_widget_create_pango_layout(m_pDrawableArea, sLabel);
	pango_layout_set_width(l_pText, 28);
	pango_layout_set_justify(l_pText, PANGO_ALIGN_LEFT);

	pango_layout_get_pixel_size(l_pText, &l_iTextW, &l_iTextH);

	gdk_draw_layout(m_pDrawableArea->window, GTK_WIDGET(m_pDrawableArea)->style->black_gc,	(ui32W-20)-(l_iTextW/2), ui32H-(l_iTextH/2), l_pText);

	GdkColor l_oLineColor = {0, 35000, 35000, 35000};
	GdkGC *l_pGc = gdk_gc_new((m_pDrawableArea)->window);
	gdk_gc_set_rgb_fg_color(l_pGc, &l_oLineColor);
	gdk_draw_line(m_pDrawableArea->window, l_pGc, ui32W - 5, ui32H, ui32W  , ui32H);

	g_object_unref(l_pGc);
}

void CROCCurveDraw::drawBottomMark(OpenViBE::uint32 ui32W, OpenViBE::uint32 ui32H, const char *sLabel)
{
	int l_iTextW;
	int l_iTextH;
	PangoLayout * l_pText = gtk_widget_create_pango_layout(m_pDrawableArea, sLabel);
	pango_layout_set_width(l_pText, 28);
	pango_layout_set_justify(l_pText, PANGO_ALIGN_LEFT);

	pango_layout_get_pixel_size(l_pText, &l_iTextW, &l_iTextH);

	gdk_draw_layout(m_pDrawableArea->window, GTK_WIDGET(m_pDrawableArea)->style->black_gc,	ui32W-(l_iTextW/2), ui32H + 14 , l_pText);

	GdkColor l_oLineColor = {0, 35000, 35000, 35000};
	GdkGC *l_pGc = gdk_gc_new((m_pDrawableArea)->window);
	gdk_gc_set_rgb_fg_color(l_pGc, &l_oLineColor);
	gdk_draw_line(m_pDrawableArea->window, l_pGc, ui32W , ui32H+5, ui32W  , ui32H);

	g_object_unref(l_pGc);
}
