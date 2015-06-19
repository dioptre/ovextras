#include "ovpCROCCurveDraw.h"

#include <iostream>


using namespace OpenViBE;
using namespace OpenViBEPlugins::Measurement;

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

	//We need to draw the rulers
	gdk_draw_line((m_pDrawableArea)->window, l_pGc,  m_ui32Margin, m_ui32Margin, m_ui32Margin, l_pAllocation.height- m_ui32Margin);
	gdk_draw_line((m_pDrawableArea)->window, l_pGc,  m_ui32Margin, l_pAllocation.height- m_ui32Margin, l_pAllocation.width - m_ui32Margin, l_pAllocation.height- m_ui32Margin);

	if(m_oPointList.size() != 0)
	{
		gdk_draw_lines((m_pDrawableArea)->window, GTK_WIDGET(m_pDrawableArea)->style->black_gc, &(m_oPointList[0]), m_oPointList.size());
	}

	gdk_gc_set_line_attributes(l_pGc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
	gdk_draw_line((m_pDrawableArea)->window, l_pGc, m_ui32Margin, l_pAllocation.height- m_ui32Margin, l_pAllocation.width - m_ui32Margin, m_ui32Margin);
}
