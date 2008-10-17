#include "ovdCInterfacedScenario.h"
#include "ovdCBoxProxy.h"
#include "ovdCLinkProxy.h"
#include "ovdCConnectorEditor.h"
#include "ovdCBoxConfigurationDialog.h"
#include "ovdCInterfacedObject.h"
#include "ovdTAttributeHandler.h"
#include "ovdCDesignerVisualisation.h"
#include "ovdCPlayerVisualisation.h"
#include "ovdCRenameDialog.h"
#include "ovdCAboutPluginDialog.h"
#include "ovdCSettingEditorDialog.h"

#include <vector>
#include <iostream>

#include <gdk/gdkkeysyms.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;
using namespace OpenViBEDesigner;
using namespace std;

extern map<uint32, ::GdkColor> g_vColors;

static ::GtkTargetEntry g_vTargetEntry[]= {
	{ "STRING", 0, 0 },
	{ "text/plain", 0, 0 } };

static ::GdkColor colorFromIdentifier(const CIdentifier& rIdentifier)
{
	::GdkColor l_oGdkColor;
	unsigned int l_ui32Value1=0;
	unsigned int l_ui32Value2=0;
	uint64 l_ui64Result=0;

	sscanf(rIdentifier.toString(), "(0x%08X, 0x%08X)", &l_ui32Value1, &l_ui32Value2);
	l_ui64Result+=l_ui32Value1;
	l_ui64Result<<=32;
	l_ui64Result+=l_ui32Value2;

	l_oGdkColor.pixel=(guint16)0;
	l_oGdkColor.red  =(guint16)(((l_ui64Result    )&0xffff)|0x8000);
	l_oGdkColor.green=(guint16)(((l_ui64Result>>16)&0xffff)|0x8000);
	l_oGdkColor.blue =(guint16)(((l_ui64Result>>32)&0xffff)|0x8000);

	return l_oGdkColor;
}

static void scenario_drawing_area_expose_cb(::GtkWidget* pWidget, ::GdkEventExpose* pEvent, gpointer pUserData)
{
	static_cast<CInterfacedScenario*>(pUserData)->scenarioDrawingAreaExposeCB(pEvent);
}
static void scenario_drawing_area_drag_data_received_cb(::GtkWidget* pWidget, ::GdkDragContext* pDragContext, gint iX, gint iY, ::GtkSelectionData* pSelectionData, guint uiInfo, guint uiT, gpointer pUserData)
{
	static_cast<CInterfacedScenario*>(pUserData)->scenarioDrawingAreaDragDataReceivedCB(pDragContext, iX, iY, pSelectionData, uiInfo, uiT);
}
static gboolean scenario_drawing_area_motion_notify_cb(::GtkWidget* pWidget, ::GdkEventMotion* pEvent, gpointer pUserData)
{
	static_cast<CInterfacedScenario*>(pUserData)->scenarioDrawingAreaMotionNotifyCB(pWidget, pEvent);
	return FALSE;
}
static void scenario_drawing_area_button_pressed_cb(::GtkWidget* pWidget, ::GdkEventButton* pEvent, gpointer pUserData)
{
	static_cast<CInterfacedScenario*>(pUserData)->scenarioDrawingAreaButtonPressedCB(pWidget, pEvent);
}
static void scenario_drawing_area_button_released_cb(::GtkWidget* pWidget, ::GdkEventButton* pEvent, gpointer pUserData)
{
	static_cast<CInterfacedScenario*>(pUserData)->scenarioDrawingAreaButtonReleasedCB(pWidget, pEvent);
}
static void scenario_drawing_area_key_press_event_cb(::GtkWidget* pWidget, ::GdkEventKey* pEvent, gpointer pUserData)
{
	static_cast<CInterfacedScenario*>(pUserData)->scenarioDrawingAreaKeyPressEventCB(pWidget, pEvent);
}
static void scenario_drawing_area_key_release_event_cb(::GtkWidget* pWidget, ::GdkEventKey* pEvent, gpointer pUserData)
{
	static_cast<CInterfacedScenario*>(pUserData)->scenarioDrawingAreaKeyReleaseEventCB(pWidget, pEvent);
}

static void context_menu_cb(::GtkMenuItem* pMenuItem, gpointer pUserData)
{
	CInterfacedScenario::BoxContextMenuCB* l_pContextMenuCB=static_cast < CInterfacedScenario::BoxContextMenuCB* >(pUserData);
	switch(l_pContextMenuCB->ui32Command)
	{
		case BoxContextMenu_Rename:        l_pContextMenuCB->pInterfacedScenario->contextMenuRenameCB(*l_pContextMenuCB->pBox); break;
		case BoxContextMenu_AddInput:      l_pContextMenuCB->pInterfacedScenario->contextMenuAddInputCB(*l_pContextMenuCB->pBox); break;
		case BoxContextMenu_EditInput:     l_pContextMenuCB->pInterfacedScenario->contextMenuEditInputCB(*l_pContextMenuCB->pBox, l_pContextMenuCB->ui32Index); break;
		case BoxContextMenu_RemoveInput:   l_pContextMenuCB->pInterfacedScenario->contextMenuRemoveInputCB(*l_pContextMenuCB->pBox, l_pContextMenuCB->ui32Index); break;
		case BoxContextMenu_AddOutput:     l_pContextMenuCB->pInterfacedScenario->contextMenuAddOutputCB(*l_pContextMenuCB->pBox); break;
		case BoxContextMenu_EditOutput:    l_pContextMenuCB->pInterfacedScenario->contextMenuEditOutputCB(*l_pContextMenuCB->pBox, l_pContextMenuCB->ui32Index); break;
		case BoxContextMenu_RemoveOutput:  l_pContextMenuCB->pInterfacedScenario->contextMenuRemoveOutputCB(*l_pContextMenuCB->pBox, l_pContextMenuCB->ui32Index); break;
		case BoxContextMenu_AddSetting:    l_pContextMenuCB->pInterfacedScenario->contextMenuAddSettingCB(*l_pContextMenuCB->pBox); break;
		case BoxContextMenu_EditSetting:   l_pContextMenuCB->pInterfacedScenario->contextMenuEditSettingCB(*l_pContextMenuCB->pBox, l_pContextMenuCB->ui32Index); break;
		case BoxContextMenu_RemoveSetting: l_pContextMenuCB->pInterfacedScenario->contextMenuRemoveSettingCB(*l_pContextMenuCB->pBox, l_pContextMenuCB->ui32Index); break;
		case BoxContextMenu_Configure:     l_pContextMenuCB->pInterfacedScenario->contextMenuConfigureCB(*l_pContextMenuCB->pBox); break;
		case BoxContextMenu_About:         l_pContextMenuCB->pInterfacedScenario->contextMenuAboutCB(*l_pContextMenuCB->pBox); break;
	}
}

static void gdk_draw_rounded_rectangle(::GdkDrawable* pDrawable, ::GdkGC* pDrawGC, ::gboolean bFill, gint x, gint y, gint width, gint height, gint radius=8)
{
	if(bFill)
	{
		gdk_draw_rectangle(
			pDrawable,
			pDrawGC,
			TRUE,
			x+radius, y, width-2*radius, height);
		gdk_draw_rectangle(
			pDrawable,
			pDrawGC,
			TRUE,
			x, y+radius, width, height-2*radius);
	}
	else
	{
		gdk_draw_line(
			pDrawable,
			pDrawGC,
			x+radius, y, x+width-radius, y);
		gdk_draw_line(
			pDrawable,
			pDrawGC,
			x+radius, y+height, x+width-radius, y+height);
		gdk_draw_line(
			pDrawable,
			pDrawGC,
			x, y+radius, x, y+height-radius);
		gdk_draw_line(
			pDrawable,
			pDrawGC,
			x+width, y+radius, x+width, y+height-radius);
	}
	gdk_draw_arc(
		pDrawable,
		pDrawGC,
		bFill,
		x+width-radius*2, y, radius*2, radius*2, 0*64, 90*64);
	gdk_draw_arc(
		pDrawable,
		pDrawGC,
		bFill,
		x, y, radius*2, radius*2, 90*64, 90*64);
	gdk_draw_arc(
		pDrawable,
		pDrawGC,
		bFill,
		x, y+height-radius*2, radius*2, radius*2, 180*64, 90*64);
	gdk_draw_arc(
		pDrawable,
		pDrawGC,
		bFill,
		x+width-radius*2, y+height-radius*2, radius*2, radius*2, 270*64, 90*64);
}

	CInterfacedScenario::CInterfacedScenario(IKernel& rKernel, IScenario& rScenario, CIdentifier& rScenarioIdentifier, ::GtkNotebook& rNotebook, const char* sGUIFilename)
		:m_oScenarioIdentifier(rScenarioIdentifier)
		,m_rKernel(rKernel)
		,m_rScenario(rScenario)
		,m_pPlayer(NULL)
		,m_rNotebook(rNotebook)
		,m_pVisualisationTree(NULL)
		,m_bDesignerVisualisationToggled(false)
		,m_pDesignerVisualisation(NULL)
		,m_pPlayerVisualisation(NULL)
		,m_pGladeDummyScenarioNotebookTitle(NULL)
		,m_pGladeDummyScenarioNotebookClient(NULL)
		,m_pGladeTooltip(NULL)
		,m_pNotebookPageTitle(NULL)
		,m_pNotebookPageContent(NULL)
		,m_pScenarioViewport(NULL)
		,m_pScenarioDrawingArea(NULL)
		,m_pStencilBuffer(NULL)
		,m_bHasFileName(false)
		,m_bHasBeenModified(false)
		,m_bButtonPressed(false)
		,m_bShiftPressed(false)
		,m_bControlPressed(false)
		,m_bAltPressed(false)
		,m_bDebugCPUUsage(false)
		,m_sGUIFilename(sGUIFilename)
		,m_i32ViewOffsetX(0)
		,m_i32ViewOffsetY(0)
		,m_ui32CurrentMode(Mode_None)
	{
		m_pGladeDummyScenarioNotebookTitle=glade_xml_new(m_sGUIFilename.c_str(), "openvibe_scenario_notebook_title", NULL);
		m_pGladeDummyScenarioNotebookClient=glade_xml_new(m_sGUIFilename.c_str(), "openvibe_scenario_notebook_scrolledwindow", NULL);
		m_pGladeTooltip=glade_xml_new(m_sGUIFilename.c_str(), "tooltip", NULL);

		m_pNotebookPageTitle=glade_xml_get_widget(m_pGladeDummyScenarioNotebookTitle, "openvibe_scenario_notebook_title");
		m_pNotebookPageContent=glade_xml_get_widget(m_pGladeDummyScenarioNotebookClient, "openvibe_scenario_notebook_scrolledwindow");
		gtk_widget_ref(m_pNotebookPageTitle);
		gtk_widget_ref(m_pNotebookPageContent);
		gtk_widget_unparent(m_pNotebookPageTitle);
		gtk_widget_unparent(m_pNotebookPageContent);
		gtk_notebook_append_page(&m_rNotebook, m_pNotebookPageContent, m_pNotebookPageTitle);
		gtk_widget_unref(m_pNotebookPageContent);
		gtk_widget_unref(m_pNotebookPageTitle);

		m_pScenarioDrawingArea=GTK_DRAWING_AREA(glade_xml_get_widget(m_pGladeDummyScenarioNotebookClient, "openvibe-scenario_drawing_area"));
		m_pScenarioViewport=GTK_VIEWPORT(glade_xml_get_widget(m_pGladeDummyScenarioNotebookClient, "openvibe-scenario_viewport"));
		gtk_drag_dest_set(GTK_WIDGET(m_pScenarioDrawingArea), GTK_DEST_DEFAULT_ALL, g_vTargetEntry, sizeof(g_vTargetEntry)/sizeof(::GtkTargetEntry), GDK_ACTION_COPY);
		g_signal_connect(G_OBJECT(m_pScenarioDrawingArea), "expose_event", G_CALLBACK(scenario_drawing_area_expose_cb), this);
		g_signal_connect(G_OBJECT(m_pScenarioDrawingArea), "drag_data_received", G_CALLBACK(scenario_drawing_area_drag_data_received_cb), this);
		g_signal_connect(G_OBJECT(m_pScenarioDrawingArea), "motion_notify_event", G_CALLBACK(scenario_drawing_area_motion_notify_cb), this);
		g_signal_connect(G_OBJECT(m_pScenarioDrawingArea), "button_press_event", G_CALLBACK(scenario_drawing_area_button_pressed_cb), this);
		g_signal_connect(G_OBJECT(m_pScenarioDrawingArea), "button_release_event", G_CALLBACK(scenario_drawing_area_button_released_cb), this);
		g_signal_connect(G_OBJECT(m_pScenarioDrawingArea), "key-press-event", G_CALLBACK(scenario_drawing_area_key_press_event_cb), this);
		g_signal_connect(G_OBJECT(m_pScenarioDrawingArea), "key-release-event", G_CALLBACK(scenario_drawing_area_key_release_event_cb), this);

		//retrieve visualisation tree
		m_oVisualisationTreeIdentifier = m_rScenario.getVisualisationTreeIdentifier();
		m_pVisualisationTree = &m_rKernel.getContext()->getVisualisationManager().getVisualisationTree(m_oVisualisationTreeIdentifier);

		//create window manager
		m_pDesignerVisualisation = new CDesignerVisualisation(*m_rKernel.getContext(), *m_pVisualisationTree, *this);
		m_pDesignerVisualisation->init(string(sGUIFilename));
	}

	CInterfacedScenario::~CInterfacedScenario(void)
	{
		//delete window manager
		if(m_pDesignerVisualisation)
		{
			delete m_pDesignerVisualisation;
		}

		if(m_pStencilBuffer)
		{
			g_object_unref(m_pStencilBuffer);
		}

		g_object_unref(m_pGladeDummyScenarioNotebookTitle);
		g_object_unref(m_pGladeDummyScenarioNotebookClient);
		g_object_unref(m_pGladeTooltip);

		gtk_notebook_remove_page(
			&m_rNotebook,
			gtk_notebook_page_num(&m_rNotebook, m_pNotebookPageContent));
	}

	boolean CInterfacedScenario::isLocked(void) const
	{
		return m_pPlayer!=NULL?true:false;
	}

	void CInterfacedScenario::redraw(void)
	{
		gdk_window_invalidate_rect(
			GTK_WIDGET(m_pScenarioDrawingArea)->window,
			NULL,
			true);
	}

	void CInterfacedScenario::updateScenarioLabel(void)
	{
		::GtkLabel* l_pTitleLabel=GTK_LABEL(glade_xml_get_widget(m_pGladeDummyScenarioNotebookTitle, "openvibe-scenario_label"));
		string l_sLabel;
		l_sLabel+=m_bHasBeenModified?"*":"";
		l_sLabel+=" ";
		l_sLabel+=m_bHasFileName?m_sFileName.substr(m_sFileName.rfind('/')+1):"(untitled)";
		l_sLabel+=" ";
		l_sLabel+=m_bHasBeenModified?"*":"";
		gtk_label_set_text(l_pTitleLabel, l_sLabel.c_str());
	}

#define updateStencilIndex(id,stencilgc) { id++; ::GdkColor sc={0, (guint16)((id&0xff0000)>>8), (guint16)(id&0xff00), (guint16)((id&0xff)<<8) }; gdk_gc_set_rgb_fg_color(stencilgc, &sc); }

	void CInterfacedScenario::redraw(IBox& rBox)
	{
		::GtkWidget* l_pWidget=GTK_WIDGET(m_pScenarioDrawingArea);
		::GdkGC* l_pStencilGC=gdk_gc_new(GDK_DRAWABLE(m_pStencilBuffer));
		::GdkGC* l_pDrawGC=gdk_gc_new(l_pWidget->window);

		vector<pair<int32, int32> > l_vInputPosition;
		vector<pair<int32, int32> > l_vOutputPosition;

		uint32 i;
		const int xMargin=5;
		const int yMargin=5;
		const int iCircleSize=11;
		const int iCircleSpace=4;

		CBoxProxy l_oBoxProxy(rBox);
		int xSize=l_oBoxProxy.getWidth(GTK_WIDGET(m_pScenarioDrawingArea))+xMargin*2;
		int ySize=l_oBoxProxy.getHeight(GTK_WIDGET(m_pScenarioDrawingArea))+yMargin*2;
		int xStart=l_oBoxProxy.getXCenter()+m_i32ViewOffsetX-(xSize>>1);
		int yStart=l_oBoxProxy.getYCenter()+m_i32ViewOffsetY-(ySize>>1);

		updateStencilIndex(m_ui32InterfacedObjectId, l_pStencilGC);
		gdk_draw_rounded_rectangle(
			GDK_DRAWABLE(m_pStencilBuffer),
			l_pStencilGC,
			TRUE,
			xStart, yStart, xSize, ySize);
		m_vInterfacedObject[m_ui32InterfacedObjectId]=CInterfacedObject(rBox.getIdentifier());

		boolean l_bCanCreate=m_rKernel.getContext()->getPluginManager().canCreatePluginObject(rBox.getAlgorithmClassIdentifier());
		boolean l_bDeprecated=rBox.hasAttribute(OV_AttributeId_Box_FlagIsDeprecated);
		if(!this->isLocked() || !m_bDebugCPUUsage)
		{
			if(m_vCurrentObject[rBox.getIdentifier()])
			{
				gdk_gc_set_rgb_fg_color(l_pDrawGC, &g_vColors[Color_BoxBackgroundSelected]);
			}
			else if(!l_bCanCreate)
			{
				gdk_gc_set_rgb_fg_color(l_pDrawGC, &g_vColors[Color_BoxBackgroundMissing]);
			}
			else if(l_bDeprecated)
			{
				gdk_gc_set_rgb_fg_color(l_pDrawGC, &g_vColors[Color_BoxBackgroundDeprecated]);
			}
			else
			{
				gdk_gc_set_rgb_fg_color(l_pDrawGC, &g_vColors[Color_BoxBackground]);
			}
		}
		else
		{
			CIdentifier l_oComputationTime;
			l_oComputationTime.fromString(rBox.getAttributeValue(OV_AttributeId_Box_ComputationTimeLastSecond));
			uint64 l_ui64ComputationTime=(l_oComputationTime==OV_UndefinedIdentifier?0:l_oComputationTime.toUInteger());
			uint64 l_ui64ComputationTimeReference=(1LL<<32)/(m_ui32BoxCount==0?1:m_ui32BoxCount);

			::GdkColor l_oColor;
			if(l_ui64ComputationTime<l_ui64ComputationTimeReference)
			{
				l_oColor.pixel=0;
				l_oColor.red  =(l_ui64ComputationTime<<16)/l_ui64ComputationTimeReference;
				l_oColor.green=32768;
				l_oColor.blue =0;
			}
			else
			{
				if(l_ui64ComputationTime<l_ui64ComputationTimeReference*4)
				{
					l_oColor.pixel=0;
					l_oColor.red  =65535;
					l_oColor.green=32768-((l_ui64ComputationTime<<15)/(l_ui64ComputationTimeReference*4));
					l_oColor.blue =0;
				}
				else
				{
					l_oColor.pixel=0;
					l_oColor.red  =65535;
					l_oColor.green=0;
					l_oColor.blue =0;
				}
			}
			gdk_gc_set_rgb_fg_color(l_pDrawGC, &l_oColor);
		}
		gdk_draw_rounded_rectangle(
			l_pWidget->window,
			l_pDrawGC,
			TRUE,
			xStart, yStart, xSize, ySize);
		gdk_gc_set_rgb_fg_color(l_pDrawGC, &g_vColors[m_vCurrentObject[rBox.getIdentifier()]?Color_BoxBorderSelected:Color_BoxBorder]);
		gdk_draw_rounded_rectangle(
			l_pWidget->window,
			l_pDrawGC,
			FALSE,
			xStart, yStart, xSize, ySize);

		TAttributeHandler l_oAttributeHandler(rBox);

		if(!l_oAttributeHandler.hasAttribute(OV_AttributeId_Box_XSize))
			l_oAttributeHandler.addAttribute<int>(OV_AttributeId_Box_XSize, xSize);
		else
			l_oAttributeHandler.setAttributeValue<int>(OV_AttributeId_Box_XSize, xSize);

		if(!l_oAttributeHandler.hasAttribute(OV_AttributeId_Box_YSize))
			l_oAttributeHandler.addAttribute<int>(OV_AttributeId_Box_YSize, ySize);
		else
			l_oAttributeHandler.setAttributeValue<int>(OV_AttributeId_Box_YSize, ySize);

		int l_iInputOffset=(xSize-rBox.getInputCount()*(iCircleSpace+iCircleSize)+iCircleSize/2)/2;
		for(i=0; i<rBox.getInputCount(); i++)
		{
			CIdentifier l_oInputIdentifier;
			rBox.getInputType(i, l_oInputIdentifier);
			::GdkColor l_oInputColor=colorFromIdentifier(l_oInputIdentifier);

			::GdkPoint l_vPoint[4];
			l_vPoint[0].x=iCircleSize>>1;
			l_vPoint[0].y=iCircleSize;
			l_vPoint[1].x=0;
			l_vPoint[1].y=0;
			l_vPoint[2].x=iCircleSize-1;
			l_vPoint[2].y=0;
			for(int j=0; j<3; j++)
			{
				l_vPoint[j].x+=xStart+i*(iCircleSpace+iCircleSize)+l_iInputOffset;
				l_vPoint[j].y+=yStart-(iCircleSize>>1);
			}

			updateStencilIndex(m_ui32InterfacedObjectId, l_pStencilGC);
			gdk_draw_polygon(
				GDK_DRAWABLE(m_pStencilBuffer),
				l_pStencilGC,
				TRUE,
				l_vPoint,
				3);
			m_vInterfacedObject[m_ui32InterfacedObjectId]=CInterfacedObject(rBox.getIdentifier(), Connector_Input, i);

			gdk_gc_set_rgb_fg_color(l_pDrawGC, &l_oInputColor);
			gdk_draw_polygon(
				l_pWidget->window,
				l_pDrawGC,
				TRUE,
				l_vPoint,
				3);
			gdk_gc_set_rgb_fg_color(l_pDrawGC, &g_vColors[Color_BoxInputBorder]);
			gdk_draw_polygon(
				l_pWidget->window,
				l_pDrawGC,
				FALSE,
				l_vPoint,
				3);

			int32 x=xStart+i*(iCircleSpace+iCircleSize)+(iCircleSize>>1)-m_i32ViewOffsetX+l_iInputOffset;
			int32 y=yStart-(iCircleSize>>1)-m_i32ViewOffsetY;
			CIdentifier l_oLinkIdentifier=m_rScenario.getNextLinkIdentifierToBoxInput(OV_UndefinedIdentifier, rBox.getIdentifier(), i);
			while(l_oLinkIdentifier!=OV_UndefinedIdentifier)
			{
				ILink* l_pLink=m_rScenario.getLinkDetails(l_oLinkIdentifier);
				if(l_pLink)
				{
					TAttributeHandler l_oAttributeHandler(*l_pLink);

					if(!l_oAttributeHandler.hasAttribute(OV_AttributeId_Link_XTargetPosition))
						l_oAttributeHandler.addAttribute<int>(OV_AttributeId_Link_XTargetPosition, x);
					else
						l_oAttributeHandler.setAttributeValue<int>(OV_AttributeId_Link_XTargetPosition, x);

					if(!l_oAttributeHandler.hasAttribute(OV_AttributeId_Link_YTargetPosition))
						l_oAttributeHandler.addAttribute<int>(OV_AttributeId_Link_YTargetPosition, y);
					else
						l_oAttributeHandler.setAttributeValue<int>(OV_AttributeId_Link_YTargetPosition, y);
				}
				l_oLinkIdentifier=m_rScenario.getNextLinkIdentifierToBoxInput(l_oLinkIdentifier, rBox.getIdentifier(), i);
			}
		}

		int l_iOutputOffset=(xSize-rBox.getOutputCount()*(iCircleSpace+iCircleSize)+iCircleSize/2)/2;
		for(i=0; i<rBox.getOutputCount(); i++)
		{
			CIdentifier l_oOutputIdentifier;
			rBox.getOutputType(i, l_oOutputIdentifier);
			::GdkColor l_oOutputColor=colorFromIdentifier(l_oOutputIdentifier);

			::GdkPoint l_vPoint[4];
			l_vPoint[0].x=iCircleSize>>1;
			l_vPoint[0].y=iCircleSize;
			l_vPoint[1].x=0;
			l_vPoint[1].y=0;
			l_vPoint[2].x=iCircleSize-1;
			l_vPoint[2].y=0;
			for(int j=0; j<3; j++)
			{
				l_vPoint[j].x+=xStart+i*(iCircleSpace+iCircleSize)+l_iOutputOffset;
				l_vPoint[j].y+=yStart-(iCircleSize>>1)+ySize;
			}

			updateStencilIndex(m_ui32InterfacedObjectId, l_pStencilGC);
			gdk_draw_polygon(
				GDK_DRAWABLE(m_pStencilBuffer),
				l_pStencilGC,
				TRUE,
				l_vPoint,
				3);
			m_vInterfacedObject[m_ui32InterfacedObjectId]=CInterfacedObject(rBox.getIdentifier(), Connector_Output, i);

			gdk_gc_set_rgb_fg_color(l_pDrawGC, &l_oOutputColor);
			gdk_draw_polygon(
				l_pWidget->window,
				l_pDrawGC,
				TRUE,
				l_vPoint,
				3);
			gdk_gc_set_rgb_fg_color(l_pDrawGC, &g_vColors[Color_BoxOutputBorder]);
			gdk_draw_polygon(
				l_pWidget->window,
				l_pDrawGC,
				FALSE,
				l_vPoint,
				3);

			int32 x=xStart+i*(iCircleSpace+iCircleSize)+(iCircleSize>>1)-m_i32ViewOffsetX+l_iOutputOffset;
			int32 y=yStart+ySize+(iCircleSize>>1)+1-m_i32ViewOffsetY;
			CIdentifier l_oLinkIdentifier=m_rScenario.getNextLinkIdentifierFromBoxOutput(OV_UndefinedIdentifier, rBox.getIdentifier(), i);
			while(l_oLinkIdentifier!=OV_UndefinedIdentifier)
			{
				ILink* l_pLink=m_rScenario.getLinkDetails(l_oLinkIdentifier);
				if(l_pLink)
				{
					TAttributeHandler l_oAttributeHandler(*l_pLink);

					if(!l_oAttributeHandler.hasAttribute(OV_AttributeId_Link_XSourcePosition))
						l_oAttributeHandler.addAttribute<int>(OV_AttributeId_Link_XSourcePosition, x);
					else
						l_oAttributeHandler.setAttributeValue<int>(OV_AttributeId_Link_XSourcePosition, x);

					if(!l_oAttributeHandler.hasAttribute(OV_AttributeId_Link_YSourcePosition))
						l_oAttributeHandler.addAttribute<int>(OV_AttributeId_Link_YSourcePosition, y);
					else
						l_oAttributeHandler.setAttributeValue<int>(OV_AttributeId_Link_YSourcePosition, y);
				}
				l_oLinkIdentifier=m_rScenario.getNextLinkIdentifierFromBoxOutput(l_oLinkIdentifier, rBox.getIdentifier(), i);
			}
		}

/*
		::GdkPixbuf* l_pPixbuf=gtk_widget_render_icon(l_pWidget, GTK_STOCK_EXECUTE, GTK_ICON_SIZE_SMALL_TOOLBAR, "openvibe");
		if(l_pPixbuf)
		{
			gdk_draw_pixbuf(l_pWidget->window, l_pDrawGC, l_pPixbuf, 0, 0, 10, 10, 64, 64, GDK_RGB_DITHER_NONE, 0, 0);
			g_object_unref(l_pPixbuf);
		}
*/

		::PangoContext* l_pPangoContext=NULL;
		::PangoLayout* l_pPangoLayout=NULL;
		l_pPangoContext=gtk_widget_get_pango_context(l_pWidget);
		l_pPangoLayout=pango_layout_new(l_pPangoContext);
		pango_layout_set_alignment(l_pPangoLayout, PANGO_ALIGN_CENTER);
		pango_layout_set_markup(l_pPangoLayout, l_oBoxProxy.getLabel(), -1);
		gdk_draw_layout(
			l_pWidget->window,
			l_pWidget->style->text_gc[GTK_WIDGET_STATE(l_pWidget)],
			xStart+xMargin, yStart+yMargin, l_pPangoLayout);
		g_object_unref(l_pPangoLayout);

		g_object_unref(l_pDrawGC);
		g_object_unref(l_pStencilGC);

/*
		CLinkPositionSetterEnum l_oLinkPositionSetterInput(Connector_Input, l_vInputPosition);
		CLinkPositionSetterEnum l_oLinkPositionSetterOutput(Connector_Output, l_vOutputPosition);
		rScenario.enumerateLinksToBox(l_oLinkPositionSetterInput, rBox.getIdentifier());
		rScenario.enumerateLinksFromBox(l_oLinkPositionSetterOutput, rBox.getIdentifier());
*/
	}

	void CInterfacedScenario::redraw(ILink& rLink)
	{
		::GtkWidget* l_pWidget=GTK_WIDGET(m_pScenarioDrawingArea);
		::GdkGC* l_pStencilGC=gdk_gc_new(GDK_DRAWABLE(m_pStencilBuffer));
		::GdkGC* l_pDrawGC=gdk_gc_new(l_pWidget->window);

		CLinkProxy l_oLinkProxy(rLink);

		updateStencilIndex(m_ui32InterfacedObjectId, l_pStencilGC);
		gdk_gc_set_rgb_fg_color(l_pDrawGC, &g_vColors[m_vCurrentObject[rLink.getIdentifier()]?Color_LinkSelected:Color_Link]);
		gdk_draw_line(
			GDK_DRAWABLE(m_pStencilBuffer),
			l_pStencilGC,
			l_oLinkProxy.getXSource()+m_i32ViewOffsetX, l_oLinkProxy.getYSource()+m_i32ViewOffsetY,
			l_oLinkProxy.getXTarget()+m_i32ViewOffsetX, l_oLinkProxy.getYTarget()+m_i32ViewOffsetY);
		gdk_draw_line(
			l_pWidget->window,
			l_pDrawGC,
			l_oLinkProxy.getXSource()+m_i32ViewOffsetX, l_oLinkProxy.getYSource()+m_i32ViewOffsetY,
			l_oLinkProxy.getXTarget()+m_i32ViewOffsetX, l_oLinkProxy.getYTarget()+m_i32ViewOffsetY);
		m_vInterfacedObject[m_ui32InterfacedObjectId]=CInterfacedObject(rLink.getIdentifier(), Connector_Link, 0);

		g_object_unref(l_pDrawGC);
		g_object_unref(l_pStencilGC);
	}

#undef updateStencilIndex

	uint32 CInterfacedScenario::pickInterfacedObject(int x, int y)
	{
		if(!GDK_DRAWABLE(m_pStencilBuffer))
		{
			return 0xffffffff;
		}

		int l_iMaxX;
		int l_iMaxY;
		uint32 l_ui32InterfacedObjectId=0xffffffff;
		gdk_drawable_get_size(GDK_DRAWABLE(m_pStencilBuffer), &l_iMaxX, &l_iMaxY);
		if(x>=0 && y>=0 && x<l_iMaxX && y<l_iMaxY)
		{
			::GdkPixbuf* l_pPixbuf=gdk_pixbuf_get_from_drawable(NULL, GDK_DRAWABLE(m_pStencilBuffer), NULL, x, y, 0, 0, 1, 1);
			l_ui32InterfacedObjectId=0;
			l_ui32InterfacedObjectId+=(gdk_pixbuf_get_pixels(l_pPixbuf)[0]<<16);
			l_ui32InterfacedObjectId+=(gdk_pixbuf_get_pixels(l_pPixbuf)[1]<<8);
			l_ui32InterfacedObjectId+=(gdk_pixbuf_get_pixels(l_pPixbuf)[2]);
			g_object_unref(l_pPixbuf);
		}
		return l_ui32InterfacedObjectId;
	}

	boolean CInterfacedScenario::pickInterfacedObject(int x, int y, int iSizeX, int iSizeY)
	{
		if(!GDK_DRAWABLE(m_pStencilBuffer))
		{
			return false;
		}

		int i,j;
		int l_iMaxX;
		int l_iMaxY;
		gdk_drawable_get_size(GDK_DRAWABLE(m_pStencilBuffer), &l_iMaxX, &l_iMaxY);

		int iStartX=x;
		int iStartY=y;
		int iEndX=x+iSizeX;
		int iEndY=y+iSizeY;

		// crops according to drawing area boundings
		if(iStartX<0) iStartX=0;
		if(iStartY<0) iStartY=0;
		if(iEndX<0) iEndX=0;
		if(iEndY<0) iEndY=0;
		if(iStartX>=l_iMaxX-1) iStartX=l_iMaxX-1;
		if(iStartY>=l_iMaxY-1) iStartY=l_iMaxY-1;
		if(iEndX>=l_iMaxX-1) iEndX=l_iMaxX-1;
		if(iEndY>=l_iMaxY-1) iEndY=l_iMaxY-1;

		// recompute new size
		iSizeX=iEndX-iStartX+1;
		iSizeY=iEndY-iStartY+1;

		::GdkPixbuf* l_pPixbuf=gdk_pixbuf_get_from_drawable(NULL, GDK_DRAWABLE(m_pStencilBuffer), NULL, iStartX, iStartY, 0, 0, iSizeX, iSizeY);
		guchar* l_pPixels=gdk_pixbuf_get_pixels(l_pPixbuf);
		int l_iRowBytesCount=gdk_pixbuf_get_rowstride(l_pPixbuf);
		int l_iChannelCount=gdk_pixbuf_get_n_channels(l_pPixbuf);
		for(j=0; j<iSizeY; j++)
		{
			for(i=0; i<iSizeX; i++)
			{
				uint32 l_ui32InterfacedObjectId=0;
				l_ui32InterfacedObjectId+=(l_pPixels[j*l_iRowBytesCount+i*l_iChannelCount+0]<<16);
				l_ui32InterfacedObjectId+=(l_pPixels[j*l_iRowBytesCount+i*l_iChannelCount+1]<<8);
				l_ui32InterfacedObjectId+=(l_pPixels[j*l_iRowBytesCount+i*l_iChannelCount+2]);
				if(m_vInterfacedObject[l_ui32InterfacedObjectId].m_oIdentifier!=OV_UndefinedIdentifier)
				{
					if(!m_vCurrentObject[m_vInterfacedObject[l_ui32InterfacedObjectId].m_oIdentifier])
					{
						m_vCurrentObject[m_vInterfacedObject[l_ui32InterfacedObjectId].m_oIdentifier]=true;
					}
				}
			}
		}
		g_object_unref(l_pPixbuf);
		return true;
	}

	void CInterfacedScenario::scenarioDrawingAreaExposeCB(::GdkEventExpose* pEvent)
	{
		if(m_ui32CurrentMode==Mode_None)
		{
			gint l_iViewportX=-1;
			gint l_iViewportY=-1;

			gint l_iMinX= 0x7fff;
			gint l_iMaxX=-0x7fff;
			gint l_iMinY= 0x7fff;
			gint l_iMaxY=-0x7fff;

			gint l_iMarginX=128;
			gint l_iMarginY=64;

			CIdentifier l_oBoxIdentifier;
			while((l_oBoxIdentifier=m_rScenario.getNextBoxIdentifier(l_oBoxIdentifier))!=OV_UndefinedIdentifier)
			{
				CBoxProxy l_oBoxProxy(*m_rScenario.getBoxDetails(l_oBoxIdentifier));
				if(l_iMinX>l_oBoxProxy.getXCenter()) l_iMinX=l_oBoxProxy.getXCenter();
				if(l_iMaxX<l_oBoxProxy.getXCenter()) l_iMaxX=l_oBoxProxy.getXCenter();
				if(l_iMinY>l_oBoxProxy.getYCenter()) l_iMinY=l_oBoxProxy.getYCenter();
				if(l_iMaxY<l_oBoxProxy.getYCenter()) l_iMaxY=l_oBoxProxy.getYCenter();
			}

			gint l_iNewScenarioSizeX=l_iMaxX-l_iMinX;
			gint l_iNewScenarioSizeY=l_iMaxY-l_iMinY;
			gint l_iOldScenarioSizeX=-1;
			gint l_iOldScenarioSizeY=-1;

			gdk_window_get_size(GTK_WIDGET(m_pScenarioViewport)->window, &l_iViewportX, &l_iViewportY);
			gtk_widget_get_size_request(GTK_WIDGET(m_pScenarioDrawingArea), &l_iOldScenarioSizeX, &l_iOldScenarioSizeY);

			if(l_iNewScenarioSizeX>=0 && l_iNewScenarioSizeY>=0)
			{
				if(l_iMarginX-m_i32ViewOffsetX                                       >l_iMinX) { m_i32ViewOffsetX=l_iMarginX-l_iMinX; }
				if(l_iMarginX-m_i32ViewOffsetX+max(l_iViewportX, l_iNewScenarioSizeX)<l_iMaxX) { m_i32ViewOffsetX=l_iMarginX-l_iMaxX+max(l_iViewportX, l_iNewScenarioSizeX); }
				if(l_iMarginY-m_i32ViewOffsetY                                       >l_iMinY) { m_i32ViewOffsetY=l_iMarginY-l_iMinY; }
				if(l_iMarginY-m_i32ViewOffsetY+max(l_iViewportY, l_iNewScenarioSizeY)<l_iMaxY) { m_i32ViewOffsetY=l_iMarginY-l_iMaxY+max(l_iViewportY, l_iNewScenarioSizeY); }
				if(l_iOldScenarioSizeX!=l_iNewScenarioSizeX+2*l_iMarginX || l_iOldScenarioSizeY!=l_iNewScenarioSizeY+2*l_iMarginY)
				{
					gtk_widget_set_size_request(GTK_WIDGET(m_pScenarioDrawingArea), l_iNewScenarioSizeX+2*l_iMarginX, l_iNewScenarioSizeY+2*l_iMarginY);
/*
					::GtkAdjustment* l_pHAdjustment=gtk_viewport_get_hadjustment(m_pScenarioViewport);
					::GtkAdjustment* l_pVAdjustment=gtk_viewport_get_vadjustment(m_pScenarioViewport);
					// gtk_adjustment_set_value(l_pHAdjustment, l_pHAdjustment->lower);
					// gtk_adjustment_set_value(l_pHAdjustment, l_pHAdjustment->upper);
					// gtk_adjustment_set_value(l_pVAdjustment, l_pVAdjustment->lower);
					// gtk_adjustment_set_value(l_pVAdjustment, l_pVAdjustment->upper);
					gtk_viewport_set_hadjustment(m_pScenarioViewport, l_pHAdjustment);
					gtk_viewport_set_vadjustment(m_pScenarioViewport, l_pVAdjustment);
*/
				}
			}
		}

		gint x,y;

		gdk_window_get_size(GTK_WIDGET(m_pScenarioDrawingArea)->window, &x, &y);
		if(m_pStencilBuffer) g_object_unref(m_pStencilBuffer);
		m_pStencilBuffer=gdk_pixmap_new(NULL, x, y, 24);

		::GdkGC* l_pStencilGC=gdk_gc_new(m_pStencilBuffer);
		::GdkColor l_oColor={0, 0, 0, 0};
		gdk_gc_set_rgb_fg_color(l_pStencilGC, &l_oColor);
		gdk_draw_rectangle(
			GDK_DRAWABLE(m_pStencilBuffer),
			l_pStencilGC,
			TRUE,
			0, 0, x, y);
		g_object_unref(l_pStencilGC);

		if(this->isLocked())
		{
			::GdkColor l_oColor;
			l_oColor.pixel=0;
			l_oColor.red  =0x0f00;
			l_oColor.green=0x0f00;
			l_oColor.blue =0x0f00;

			::GdkGC* l_pDrawGC=gdk_gc_new(GTK_WIDGET(m_pScenarioDrawingArea)->window);
			gdk_gc_set_rgb_fg_color(l_pDrawGC, &l_oColor);
			gdk_gc_set_function(l_pDrawGC, GDK_XOR);
			gdk_draw_rectangle(
				GTK_WIDGET(m_pScenarioDrawingArea)->window,
				l_pDrawGC,
				TRUE,
				0, 0, x, y);
			g_object_unref(l_pDrawGC);
		}

		m_ui32InterfacedObjectId=0;
		m_vInterfacedObject.clear();

		uint32 l_ui32BoxCount=0;
		CIdentifier l_oBoxIdentifier;
		while((l_oBoxIdentifier=m_rScenario.getNextBoxIdentifier(l_oBoxIdentifier))!=OV_UndefinedIdentifier)
		{
			redraw(*m_rScenario.getBoxDetails(l_oBoxIdentifier));
			l_ui32BoxCount++;
		}
		m_ui32BoxCount=l_ui32BoxCount;

		uint32 l_ui32LinkCount=0;
		CIdentifier l_oLinkIdentifier;
		while((l_oLinkIdentifier=m_rScenario.getNextLinkIdentifier(l_oLinkIdentifier))!=OV_UndefinedIdentifier)
		{
			redraw(*m_rScenario.getLinkDetails(l_oLinkIdentifier));
			l_ui32LinkCount++;
		}
		m_ui32LinkCount=l_ui32LinkCount;

		if(m_ui32CurrentMode==Mode_Selection || m_ui32CurrentMode==Mode_SelectionAdd)
		{
			int l_iStartX=(int)min(m_f64PressMouseX, m_f64CurrentMouseX);
			int l_iStartY=(int)min(m_f64PressMouseY, m_f64CurrentMouseY);
			int l_iSizeX=(int)max(m_f64PressMouseX-m_f64CurrentMouseX, m_f64CurrentMouseX-m_f64PressMouseX);
			int l_iSizeY=(int)max(m_f64PressMouseY-m_f64CurrentMouseY, m_f64CurrentMouseY-m_f64PressMouseY);

			::GtkWidget* l_pWidget=GTK_WIDGET(m_pScenarioDrawingArea);
			::GdkGC* l_pDrawGC=gdk_gc_new(l_pWidget->window);
			gdk_gc_set_function(l_pDrawGC, GDK_OR);
			gdk_gc_set_rgb_fg_color(l_pDrawGC, &g_vColors[Color_SelectionArea]);
			gdk_draw_rectangle(
				l_pWidget->window,
				l_pDrawGC,
				TRUE,
				l_iStartX, l_iStartY, l_iSizeX, l_iSizeY);
			gdk_gc_set_function(l_pDrawGC, GDK_COPY);
			gdk_gc_set_rgb_fg_color(l_pDrawGC, &g_vColors[Color_SelectionAreaBorder]);
			gdk_draw_rectangle(
				l_pWidget->window,
				l_pDrawGC,
				FALSE,
				l_iStartX, l_iStartY, l_iSizeX, l_iSizeY);
			g_object_unref(l_pDrawGC);
		}

		if(m_ui32CurrentMode==Mode_Connect)
		{
			::GtkWidget* l_pWidget=GTK_WIDGET(m_pScenarioDrawingArea);
			::GdkGC* l_pDrawGC=gdk_gc_new(l_pWidget->window);

			gdk_gc_set_rgb_fg_color(l_pDrawGC, &g_vColors[Color_Link]);
			gdk_draw_line(
				l_pWidget->window,
				l_pDrawGC,
				(int)m_f64PressMouseX, (int)m_f64PressMouseY,
				(int)m_f64CurrentMouseX, (int)m_f64CurrentMouseY);
			g_object_unref(l_pDrawGC);
		}
	}
	void CInterfacedScenario::scenarioDrawingAreaDragDataReceivedCB(::GdkDragContext* pDragContext, gint iX, gint iY, ::GtkSelectionData* pSelectionData, guint uiInfo, guint uiT)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "scenarioDrawingAreaDragDataReceivedCB [" << (const char*)gtk_selection_data_get_text(pSelectionData) << "]\n";

		if(this->isLocked()) return;

		CIdentifier l_oBoxIdentifier;
		CIdentifier l_oBoxAlgorithmClassIdentifier;
		l_oBoxAlgorithmClassIdentifier.fromString((const char*)gtk_selection_data_get_text(pSelectionData));
		if(l_oBoxAlgorithmClassIdentifier!=OV_UndefinedIdentifier)
		{
			m_rScenario.addBox(l_oBoxAlgorithmClassIdentifier, l_oBoxIdentifier);

			IBox* l_pBox = m_rScenario.getBoxDetails(l_oBoxIdentifier);
			CIdentifier l_oId = l_pBox->getAlgorithmClassIdentifier();
			const IPluginObjectDesc* l_pPOD = m_rKernel.getContext()->getPluginManager().getPluginObjectDescCreating(l_oId);

			//if a visualisation box was dropped, add it in window manager
			if(l_pPOD && l_pPOD->hasFunctionality(Kernel::PluginFunctionality_Visualization))
			{
				//generate a unique name so that it can be identified unambiguously
				CString l_oBoxName;
				generateDisplayPluginName(l_pBox, l_oBoxName);
				l_pBox->setName(l_oBoxName);

				//let window manager know about new box
				m_pDesignerVisualisation->onVisualisationBoxAdded(l_pBox);
			}

			CBoxProxy l_oBoxProxy(m_rScenario, l_oBoxIdentifier);
			l_oBoxProxy.setCenter(iX-m_i32ViewOffsetX, iY-m_i32ViewOffsetY);
			m_bHasBeenModified=true;
			updateScenarioLabel();
		}

		m_f64CurrentMouseX=iX;
		m_f64CurrentMouseY=iY;
	}
	void CInterfacedScenario::scenarioDrawingAreaMotionNotifyCB(::GtkWidget* pWidget, ::GdkEventMotion* pEvent)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "scenarioDrawingAreaMotionNotifyCB\n";

		if(this->isLocked()) return;

		::GtkWidget* l_pTooltip=glade_xml_get_widget(m_pGladeTooltip, "tooltip");
		gtk_widget_set_name(l_pTooltip, "gtk-tooltips");
		uint32 l_ui32InterfacedObjectId=pickInterfacedObject((int)pEvent->x, (int)pEvent->y);
		CInterfacedObject& l_rObject=m_vInterfacedObject[l_ui32InterfacedObjectId];
		if(l_rObject.m_oIdentifier!=OV_UndefinedIdentifier
		&& l_rObject.m_ui32ConnectorType!=Connector_Link
		&& l_rObject.m_ui32ConnectorType!=Connector_None)
		{
			IBox* l_pBoxDetails=m_rScenario.getBoxDetails(l_rObject.m_oIdentifier);
			if(l_pBoxDetails)
			{
				CString l_sName;
				CString l_sType;
				if(l_rObject.m_ui32ConnectorType==Connector_Input)
				{
					CIdentifier l_oType;
					l_pBoxDetails->getInputName(l_rObject.m_ui32ConnectorIndex, l_sName);
					l_pBoxDetails->getInputType(l_rObject.m_ui32ConnectorIndex, l_oType);
					l_sType=m_rKernel.getContext()->getTypeManager().getTypeName(l_oType);
				}
				if(l_rObject.m_ui32ConnectorType==Connector_Output)
				{
					CIdentifier l_oType;
					l_pBoxDetails->getOutputName(l_rObject.m_ui32ConnectorIndex, l_sName);
					l_pBoxDetails->getOutputType(l_rObject.m_ui32ConnectorIndex, l_oType);
					l_sType=m_rKernel.getContext()->getTypeManager().getTypeName(l_oType);
				}
				l_sType=CString("[")+l_sType+CString("]");
				gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(m_pGladeTooltip, "tooltip-label_name_content")), l_sName);
				gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(m_pGladeTooltip, "tooltip-label_type_content")), l_sType);
				gtk_window_move(GTK_WINDOW(l_pTooltip), (gint)pEvent->x_root, (gint)pEvent->y_root+40);
				gtk_widget_show(l_pTooltip);
			}
		}
		else
		{
			gtk_widget_hide(l_pTooltip);
		}

		if(m_ui32CurrentMode!=Mode_None)
		{
			if(m_ui32CurrentMode==Mode_MoveScenario)
			{
				m_i32ViewOffsetX+=(int32)(pEvent->x-m_f64CurrentMouseX);
				m_i32ViewOffsetY+=(int32)(pEvent->y-m_f64CurrentMouseY);
			}
			if(m_ui32CurrentMode==Mode_MoveSelection)
			{
				map<CIdentifier, boolean>::const_iterator i;
				for(i=m_vCurrentObject.begin(); i!=m_vCurrentObject.end(); i++)
				{
					if(i->second && m_rScenario.isBox(i->first))
					{
						CBoxProxy l_oBoxProxy(m_rScenario, i->first);
						l_oBoxProxy.setCenter(
							l_oBoxProxy.getXCenter()+(int32)(pEvent->x-m_f64CurrentMouseX),
							l_oBoxProxy.getYCenter()+(int32)(pEvent->y-m_f64CurrentMouseY));
					}
				}
			}

			this->redraw();
		}
		m_f64CurrentMouseX=pEvent->x;
		m_f64CurrentMouseY=pEvent->y;
	}
	void CInterfacedScenario::scenarioDrawingAreaButtonPressedCB(::GtkWidget* pWidget, ::GdkEventButton* pEvent)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "scenarioDrawingAreaButtonPressedCB\n";

		if(this->isLocked()) return;

		::GtkWidget* l_pTooltip=glade_xml_get_widget(m_pGladeTooltip, "tooltip");
		gtk_widget_hide(l_pTooltip);
		gtk_widget_grab_focus(pWidget);

		m_bButtonPressed|=((pEvent->type==GDK_BUTTON_PRESS)&&(pEvent->button==1));
		m_f64PressMouseX=pEvent->x;
		m_f64PressMouseY=pEvent->y;

		uint32 l_ui32InterfacedObjectId=pickInterfacedObject((int)m_f64PressMouseX, (int)m_f64PressMouseY);
		m_oCurrentObject=m_vInterfacedObject[l_ui32InterfacedObjectId];

		switch(pEvent->button)
		{
			case 1:
				switch(pEvent->type)
				{
					case GDK_BUTTON_PRESS:
						if(m_oCurrentObject.m_oIdentifier==OV_UndefinedIdentifier)
						{
							if(m_bShiftPressed)
							{
								m_ui32CurrentMode=Mode_MoveScenario;
							}
							else
							{
								if(m_bControlPressed)
								{
									m_ui32CurrentMode=Mode_SelectionAdd;
								}
								else
								{
									m_ui32CurrentMode=Mode_Selection;
								}
							}
						}
						else
						{
							if(m_oCurrentObject.m_ui32ConnectorType==Connector_Input || m_oCurrentObject.m_ui32ConnectorType==Connector_Output)
							{
								m_ui32CurrentMode=Mode_Connect;
							}
							else
							{
								m_ui32CurrentMode=Mode_MoveSelection;
								if(!m_vCurrentObject[m_oCurrentObject.m_oIdentifier])
								{
									if(!m_bControlPressed)
									{
										m_vCurrentObject.clear();
									}
									m_vCurrentObject[m_oCurrentObject.m_oIdentifier]=true;
								}
							}
						}
						break;
					case GDK_2BUTTON_PRESS:
						if(m_oCurrentObject.m_oIdentifier!=OV_UndefinedIdentifier)
						{
							m_ui32CurrentMode=Mode_EditSettings;
							m_bShiftPressed=false;
							m_bControlPressed=false;
							m_bAltPressed=false;

							if(m_oCurrentObject.m_ui32ConnectorType==Connector_Input || m_oCurrentObject.m_ui32ConnectorType==Connector_Output)
							{
								IBox* l_pBox=m_rScenario.getBoxDetails(m_oCurrentObject.m_oIdentifier);
								if(l_pBox)
								{
									CConnectorEditor l_oConnectorEditor(m_rKernel, *l_pBox, m_oCurrentObject.m_ui32ConnectorType, m_oCurrentObject.m_ui32ConnectorIndex, m_sGUIFilename.c_str());
									l_oConnectorEditor.run();
								}
							}
							else
							{
								IBox* l_pBox=m_rScenario.getBoxDetails(m_oCurrentObject.m_oIdentifier);
								if(l_pBox)
								{
									CBoxConfigurationDialog l_oBoxConfigurationDialog(m_rKernel, *l_pBox, m_sGUIFilename.c_str());
									l_oBoxConfigurationDialog.run();
								}
							}
						}
						break;
					default:
						break;
				}
				break;

			case 2:
				break;

			case 3:
#if 1
				switch(pEvent->type)
				{
					case GDK_BUTTON_PRESS:
						{
							if(m_oCurrentObject.m_oIdentifier!=OV_UndefinedIdentifier)
							{
								IBox* l_pBox=m_rScenario.getBoxDetails(m_oCurrentObject.m_oIdentifier);
								if(l_pBox)
								{
									uint32 i;
									char l_sCompleteName[1024];
									m_vBoxContextMenuCB.clear();
									::GtkMenu* l_pMenu=GTK_MENU(gtk_menu_new());

									#define gtk_menu_add_new_image_menu_item(menu, menuitem, icon, label) \
										::GtkImageMenuItem* menuitem=NULL; \
										{ \
											menuitem=GTK_IMAGE_MENU_ITEM(gtk_image_menu_item_new_with_label(label)); \
											gtk_image_menu_item_set_image(menuitem, gtk_image_new_from_stock(icon, GTK_ICON_SIZE_MENU)); \
											gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem)); \
										}
									#define gtk_menu_add_new_image_menu_item_with_cb(menu, menuitem, icon, label, cb, cb_box, cb_command, cb_index) \
										gtk_menu_add_new_image_menu_item(menu, menuitem, icon, label); \
										{ \
											CInterfacedScenario::BoxContextMenuCB l_oBoxContextMenuCB; \
											l_oBoxContextMenuCB.ui32Command=cb_command; \
											l_oBoxContextMenuCB.ui32Index=cb_index; \
											l_oBoxContextMenuCB.pBox=cb_box; \
											l_oBoxContextMenuCB.pInterfacedScenario=this; \
											uint32 l_ui32MapIndex=m_vBoxContextMenuCB.size(); \
											m_vBoxContextMenuCB[l_ui32MapIndex]=l_oBoxContextMenuCB; \
											g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(cb), &m_vBoxContextMenuCB[l_ui32MapIndex]); \
										}
									#define gtk_menu_add_new_image_menu_item_with_cb_condition(condition, menu, menuitem, icon, label, cb, cb_box, cb_command, cb_index) \
										if(condition) \
										{ \
											gtk_menu_add_new_image_menu_item_with_cb(menu, menuitem, icon, label, cb, cb_box, cb_command, cb_index); \
										}
									#define gtk_menu_add_separator_menu_item(menu) \
										{ \
											::GtkSeparatorMenuItem* menuitem=GTK_SEPARATOR_MENU_ITEM(gtk_separator_menu_item_new()); \
											gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem)); \
										}

									// -------------- INPUTS --------------

									boolean l_bFlagCanAddInput=l_pBox->hasAttribute(OV_AttributeId_Box_FlagCanAddInput);
									boolean l_bFlagCanModifyInput=l_pBox->hasAttribute(OV_AttributeId_Box_FlagCanModifyInput);
									if(l_bFlagCanAddInput || l_bFlagCanModifyInput)
									{
										::GtkMenu* l_pMenuInput=GTK_MENU(gtk_menu_new());
										gtk_menu_add_new_image_menu_item(l_pMenu, l_pMenuItemInput, GTK_STOCK_PROPERTIES, "modify inputs");
										for(i=0; i<l_pBox->getInputCount(); i++)
										{
											CString l_sName;
											CIdentifier l_oType;
											l_pBox->getInputName(i, l_sName);
											l_pBox->getInputType(i, l_oType);
											sprintf(l_sCompleteName, "%i : %s", (int)i+1, l_sName.toASCIIString());
											gtk_menu_add_new_image_menu_item(l_pMenuInput, l_pMenuInputMenuItem, GTK_STOCK_PROPERTIES, l_sCompleteName);

											::GtkMenu* l_pMenuInputMenuAction=GTK_MENU(gtk_menu_new());
											gtk_menu_add_new_image_menu_item_with_cb_condition(l_bFlagCanModifyInput, l_pMenuInputMenuAction, l_pMenuInputInputMenuItemConfigure, GTK_STOCK_EDIT, "configure...", context_menu_cb, l_pBox, BoxContextMenu_EditInput, i);
											gtk_menu_add_new_image_menu_item_with_cb_condition(l_bFlagCanAddInput, l_pMenuInputMenuAction, l_pMenuInputInputMenuItemRemove, GTK_STOCK_REMOVE, "delete...", context_menu_cb, l_pBox, BoxContextMenu_RemoveInput, i);
											gtk_menu_item_set_submenu(GTK_MENU_ITEM(l_pMenuInputMenuItem), GTK_WIDGET(l_pMenuInputMenuAction));
										}
										gtk_menu_add_separator_menu_item(l_pMenuInput);
										gtk_menu_add_new_image_menu_item_with_cb_condition(l_bFlagCanAddInput, l_pMenuInput, l_pMenuInputMenuItemAdd, GTK_STOCK_ADD, "new...", context_menu_cb, l_pBox, BoxContextMenu_AddInput, -1);
										gtk_menu_item_set_submenu(GTK_MENU_ITEM(l_pMenuItemInput), GTK_WIDGET(l_pMenuInput));
									}

									// -------------- OUTPUTS --------------

									boolean l_bFlagCanAddOutput=l_pBox->hasAttribute(OV_AttributeId_Box_FlagCanAddOutput);
									boolean l_bFlagCanModifyOutput=l_pBox->hasAttribute(OV_AttributeId_Box_FlagCanModifyOutput);
									if(l_bFlagCanAddOutput || l_bFlagCanModifyOutput)
									{
										gtk_menu_add_new_image_menu_item(l_pMenu, l_pMenuItemOutput, GTK_STOCK_PROPERTIES, "modify outputs");
										::GtkMenu* l_pMenuOutput=GTK_MENU(gtk_menu_new());
										for(i=0; i<l_pBox->getOutputCount(); i++)
										{
											CString l_sName;
											CIdentifier l_oType;
											l_pBox->getOutputName(i, l_sName);
											l_pBox->getOutputType(i, l_oType);
											sprintf(l_sCompleteName, "%i : %s", (int)i+1, l_sName.toASCIIString());
											gtk_menu_add_new_image_menu_item(l_pMenuOutput, l_pMenuOutputMenuItem, GTK_STOCK_PROPERTIES, l_sCompleteName);

											::GtkMenu* l_pMenuOutputMenuAction=GTK_MENU(gtk_menu_new());
											gtk_menu_add_new_image_menu_item_with_cb_condition(l_bFlagCanModifyOutput, l_pMenuOutputMenuAction, l_pMenuOutputInputMenuItemConfigure, GTK_STOCK_EDIT, "configure...", context_menu_cb, l_pBox, BoxContextMenu_EditOutput, i);
											gtk_menu_add_new_image_menu_item_with_cb_condition(l_bFlagCanAddOutput, l_pMenuOutputMenuAction, l_pMenuOutputInputMenuItemRemove, GTK_STOCK_REMOVE, "delete...", context_menu_cb, l_pBox, BoxContextMenu_RemoveOutput, i);
											gtk_menu_item_set_submenu(GTK_MENU_ITEM(l_pMenuOutputMenuItem), GTK_WIDGET(l_pMenuOutputMenuAction));
										}
										gtk_menu_add_separator_menu_item(l_pMenuOutput);
										gtk_menu_add_new_image_menu_item_with_cb_condition(l_bFlagCanAddOutput, l_pMenuOutput, l_pMenuOutputMenuItemAdd, GTK_STOCK_ADD, "new...", context_menu_cb, l_pBox, BoxContextMenu_AddOutput, -1);
										gtk_menu_item_set_submenu(GTK_MENU_ITEM(l_pMenuItemOutput), GTK_WIDGET(l_pMenuOutput));
									}

									// -------------- SETTINGS --------------

									boolean l_bFlagCanAddSetting=l_pBox->hasAttribute(OV_AttributeId_Box_FlagCanAddSetting);
									boolean l_bFlagCanModifySetting=l_pBox->hasAttribute(OV_AttributeId_Box_FlagCanModifySetting);
									if(l_bFlagCanAddSetting || l_bFlagCanModifySetting)
									{
										gtk_menu_add_new_image_menu_item(l_pMenu, l_pMenuItemSetting, GTK_STOCK_PROPERTIES, "modify settings");
										::GtkMenu* l_pMenuSetting=GTK_MENU(gtk_menu_new());
										for(i=0; i<l_pBox->getSettingCount(); i++)
										{
											CString l_sName;
											CIdentifier l_oType;
											l_pBox->getSettingName(i, l_sName);
											l_pBox->getSettingType(i, l_oType);
											sprintf(l_sCompleteName, "%i : %s", (int)i+1, l_sName.toASCIIString());
											gtk_menu_add_new_image_menu_item(l_pMenuSetting, l_pMenuSettingMenuItem, GTK_STOCK_PROPERTIES, l_sCompleteName);

											::GtkMenu* l_pMenuSettingMenuAction=GTK_MENU(gtk_menu_new());
											gtk_menu_add_new_image_menu_item_with_cb_condition(l_bFlagCanModifySetting, l_pMenuSettingMenuAction, l_pMenuSettingInputMenuItemConfigure, GTK_STOCK_EDIT, "configure...", context_menu_cb, l_pBox, BoxContextMenu_EditSetting, i);
											gtk_menu_add_new_image_menu_item_with_cb_condition(l_bFlagCanAddSetting, l_pMenuSettingMenuAction, l_pMenuSettingInputMenuItemRemove, GTK_STOCK_REMOVE, "delete...", context_menu_cb, l_pBox, BoxContextMenu_RemoveSetting, i);
											gtk_menu_item_set_submenu(GTK_MENU_ITEM(l_pMenuSettingMenuItem), GTK_WIDGET(l_pMenuSettingMenuAction));
										}
										gtk_menu_add_separator_menu_item(l_pMenuSetting);
										gtk_menu_add_new_image_menu_item_with_cb_condition(l_bFlagCanAddSetting, l_pMenuSetting, l_pMenuSettingMenuItemAdd, GTK_STOCK_ADD, "new...", context_menu_cb, l_pBox, BoxContextMenu_AddSetting, -1);
										gtk_menu_item_set_submenu(GTK_MENU_ITEM(l_pMenuItemSetting), GTK_WIDGET(l_pMenuSetting));
									}

									// -------------- PROCESSING UNIT --------------
#if 0
									gtk_menu_add_new_image_menu_item(l_pMenu, l_pMenuItemProcessUnit, GTK_STOCK_EXECUTE, "process unit", NULL);
									::GtkMenu* l_pMenuProcessingUnit=GTK_MENU(gtk_menu_new());
									gtk_menu_add_new_image_menu_item(l_pMenuProcessingUnit, l_pMenuProcessingUnitDefault, GTK_STOCK_HOME, "default", NULL);
									gtk_menu_add_separator_menu_item(l_pMenuProcessingUnit);
									gtk_menu_add_new_image_menu_item(l_pMenuProcessingUnit, l_pMenuProcessingUnitAdd, GTK_STOCK_ADD, "new...", NULL);
									gtk_menu_item_set_submenu(GTK_MENU_ITEM(l_pMenuItemProcessUnit), GTK_WIDGET(l_pMenuProcessingUnit));
#endif
									// -------------- ABOUT / RENAME --------------

									gtk_menu_add_new_image_menu_item_with_cb(l_pMenu, l_pMenuItemEdit, GTK_STOCK_EDIT, "configure box...", context_menu_cb, l_pBox, BoxContextMenu_Configure, -1);
									gtk_menu_add_new_image_menu_item_with_cb(l_pMenu, l_pMenuItemRename, GTK_STOCK_EDIT, "rename box...", context_menu_cb, l_pBox, BoxContextMenu_Rename, -1);
									gtk_menu_add_new_image_menu_item_with_cb(l_pMenu, l_pMenuItemAbout, GTK_STOCK_ABOUT, "about...", context_menu_cb, l_pBox, BoxContextMenu_About, -1);

									// -------------- RUN --------------

									#undef gtk_menu_add_separator_menu_item
									#undef gtk_menu_add_new_image_menu_item_with_cb
									#undef gtk_menu_add_new_image_menu_item

									gtk_widget_show_all(GTK_WIDGET(l_pMenu));
									gtk_menu_popup(l_pMenu, NULL, NULL, NULL, NULL, 3, pEvent->time);
								}
							}
						}
						break;
					default:
						break;
				}
#endif
				break;

			default:
				break;
		}

		this->redraw();
	}
	void CInterfacedScenario::scenarioDrawingAreaButtonReleasedCB(::GtkWidget* pWidget, ::GdkEventButton* pEvent)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "scenarioDrawingAreaButtonReleasedCB\n";

		if(this->isLocked()) return;

		m_bButtonPressed&=!((pEvent->type==GDK_BUTTON_RELEASE)&&(pEvent->button==1));
		m_f64ReleaseMouseX=pEvent->x;
		m_f64ReleaseMouseY=pEvent->y;

		if(m_ui32CurrentMode!=Mode_None)
		{
			if(m_ui32CurrentMode==Mode_Selection || m_ui32CurrentMode==Mode_SelectionAdd)
			{
				if(m_ui32CurrentMode==Mode_Selection)
				{
					m_vCurrentObject.clear();
				}
				int l_iStartX=(int)min(m_f64PressMouseX, m_f64CurrentMouseX);
				int l_iStartY=(int)min(m_f64PressMouseY, m_f64CurrentMouseY);
				int l_iSizeX=(int)max(m_f64PressMouseX-m_f64CurrentMouseX, m_f64CurrentMouseX-m_f64PressMouseX);
				int l_iSizeY=(int)max(m_f64PressMouseY-m_f64CurrentMouseY, m_f64CurrentMouseY-m_f64PressMouseY);
				pickInterfacedObject(l_iStartX, l_iStartY, l_iSizeX, l_iSizeY);
			}
			if(m_ui32CurrentMode==Mode_Connect)
			{
				uint32 l_ui32InterfacedObjectId=pickInterfacedObject((int)m_f64ReleaseMouseX, (int)m_f64ReleaseMouseY);
				CInterfacedObject l_oCurrentObject=m_vInterfacedObject[l_ui32InterfacedObjectId];
				if(l_oCurrentObject.m_ui32ConnectorType==Connector_Output && m_oCurrentObject.m_ui32ConnectorType==Connector_Input)
				{
					CIdentifier l_oLinkIdentifier;
					m_rScenario.connect(
						l_oCurrentObject.m_oIdentifier,
						l_oCurrentObject.m_ui32ConnectorIndex,
						m_oCurrentObject.m_oIdentifier,
						m_oCurrentObject.m_ui32ConnectorIndex,
						l_oLinkIdentifier);
				}
				if(l_oCurrentObject.m_ui32ConnectorType==Connector_Input && m_oCurrentObject.m_ui32ConnectorType==Connector_Output)
				{
					CIdentifier l_oLinkIdentifier;
					m_rScenario.connect(
						m_oCurrentObject.m_oIdentifier,
						m_oCurrentObject.m_ui32ConnectorIndex,
						l_oCurrentObject.m_oIdentifier,
						l_oCurrentObject.m_ui32ConnectorIndex,
						l_oLinkIdentifier);
				}
			}
			if(m_ui32CurrentMode==Mode_MoveSelection)
			{
				map<CIdentifier, boolean>::const_iterator i;
				for(i=m_vCurrentObject.begin(); i!=m_vCurrentObject.end(); i++)
				{
					if(i->second && m_rScenario.isBox(i->first))
					{
						CBoxProxy l_oBoxProxy(m_rScenario, i->first);
						l_oBoxProxy.setCenter(
							((l_oBoxProxy.getXCenter()+8)&0xfffffff0),
							((l_oBoxProxy.getYCenter()+8)&0xfffffff0));
					}
				}
			}

			this->redraw();
		}

		m_ui32CurrentMode=Mode_None;
	}
	void CInterfacedScenario::scenarioDrawingAreaKeyPressEventCB(::GtkWidget* pWidget, ::GdkEventKey* pEvent)
	{
		m_bShiftPressed  |=(pEvent->keyval==GDK_Shift_L   || pEvent->keyval==GDK_Shift_R);
		m_bControlPressed|=(pEvent->keyval==GDK_Control_L || pEvent->keyval==GDK_Control_R);
		m_bAltPressed    |=(pEvent->keyval==GDK_Alt_L     || pEvent->keyval==GDK_Alt_R);

		m_rKernel.getContext()->getLogManager() << LogLevel_Debug
			<< "scenarioDrawingAreaKeyPressEventCB ("
			<< (m_bShiftPressed?"true":"false") << "|"
			<< (m_bControlPressed?"true":"false") << "|"
			<< (m_bAltPressed?"true":"false") << "|"
			<< ")\n";

		if(this->isLocked()) return;

		if(pEvent->keyval==GDK_Delete || pEvent->keyval==GDK_KP_Delete)
		{
			map<CIdentifier, boolean>::const_iterator i;
			for(i=m_vCurrentObject.begin(); i!=m_vCurrentObject.end(); i++)
			{
				if(i->second)
				{
					if(m_rScenario.isBox(i->first))
					{
						//remove visualisation box from window manager
						m_pDesignerVisualisation->onVisualisationBoxRemoved(i->first);

						//remove box from scenario
						m_rScenario.removeBox(i->first);
					}
					else
					{
						m_rScenario.disconnect(i->first);
					}
				}
			}
			m_vCurrentObject.clear();

			this->redraw();
		}
	}
	void CInterfacedScenario::scenarioDrawingAreaKeyReleaseEventCB(::GtkWidget* pWidget, ::GdkEventKey* pEvent)
	{
		m_bShiftPressed  &=!(pEvent->keyval==GDK_Shift_L   || pEvent->keyval==GDK_Shift_R);
		m_bControlPressed&=!(pEvent->keyval==GDK_Control_L || pEvent->keyval==GDK_Control_R);
		m_bAltPressed    &=!(pEvent->keyval==GDK_Alt_L     || pEvent->keyval==GDK_Alt_R);

		m_rKernel.getContext()->getLogManager() << LogLevel_Debug
			<< "scenarioDrawingAreaKeyReleaseEventCB ("
			<< (m_bShiftPressed?"true":"false") << "|"
			<< (m_bControlPressed?"true":"false") << "|"
			<< (m_bAltPressed?"true":"false") << "|"
			<< ")\n";

		if(this->isLocked()) return;

		// ...
	}

	void CInterfacedScenario::contextMenuRenameCB(IBox& rBox)
	{
		const IPluginObjectDesc* l_pPluginObjectDescriptor=m_rKernel.getContext()->getPluginManager().getPluginObjectDescCreating(rBox.getAlgorithmClassIdentifier());
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuRenameCB\n";
		CRenameDialog l_oRename(m_rKernel, rBox.getName(), l_pPluginObjectDescriptor?l_pPluginObjectDescriptor->getName():rBox.getName(), m_sGUIFilename.c_str());
		if(l_oRename.run())
		{
			rBox.setName(l_oRename.getResult());
		}
	}
	void CInterfacedScenario::contextMenuAddInputCB(IBox& rBox)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuAddInputCB\n";
		rBox.addInput("", OV_UndefinedIdentifier);
		if(rBox.hasAttribute(OV_AttributeId_Box_FlagCanModifyInput))
		{
			CConnectorEditor l_oConnectorEditor(m_rKernel, rBox, Connector_Input, rBox.getInputCount()-1, m_sGUIFilename.c_str());
			if(!l_oConnectorEditor.run())
			{
				rBox.removeInput(rBox.getInputCount()-1);
			}
		}
	}
	void CInterfacedScenario::contextMenuEditInputCB(IBox& rBox, uint32 ui32Index)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuEditInputCB\n";

		CConnectorEditor l_oConnectorEditor(m_rKernel, rBox, Connector_Input, ui32Index, m_sGUIFilename.c_str());
		l_oConnectorEditor.run();
	}
	void CInterfacedScenario::contextMenuRemoveInputCB(IBox& rBox, uint32 ui32Index)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuRemoveInputCB\n";
		rBox.removeInput(ui32Index);
	}
	void CInterfacedScenario::contextMenuAddOutputCB(IBox& rBox)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuAddOutputCB\n";
		rBox.addOutput("", OV_UndefinedIdentifier);
		if(rBox.hasAttribute(OV_AttributeId_Box_FlagCanModifyOutput))
		{
			CConnectorEditor l_oConnectorEditor(m_rKernel, rBox, Connector_Output, rBox.getOutputCount()-1, m_sGUIFilename.c_str());
			if(!l_oConnectorEditor.run())
			{
				rBox.removeOutput(rBox.getOutputCount()-1);
			}
		}
	}
	void CInterfacedScenario::contextMenuEditOutputCB(IBox& rBox, uint32 ui32Index)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuEditOutputCB\n";

		CConnectorEditor l_oConnectorEditor(m_rKernel, rBox, Connector_Output, ui32Index, m_sGUIFilename.c_str());
		l_oConnectorEditor.run();
	}
	void CInterfacedScenario::contextMenuRemoveOutputCB(IBox& rBox, uint32 ui32Index)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuRemoveOutputCB\n";
		rBox.removeOutput(ui32Index);
	}
	void CInterfacedScenario::contextMenuAddSettingCB(IBox& rBox)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuAddSettingCB\n";
		rBox.addSetting("", OV_UndefinedIdentifier, "");
		if(rBox.hasAttribute(OV_AttributeId_Box_FlagCanModifySetting))
		{
			CSettingEditorDialog l_oSettingEditorDialog(m_rKernel, rBox, rBox.getSettingCount()-1, m_sGUIFilename.c_str());
			if(!l_oSettingEditorDialog.run())
			{
				rBox.removeSetting(rBox.getSettingCount()-1);
			}
		}
	}
	void CInterfacedScenario::contextMenuEditSettingCB(IBox& rBox, uint32 ui32Index)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuEditSettingCB\n";
		CSettingEditorDialog l_oSettingEditorDialog(m_rKernel, rBox, ui32Index, m_sGUIFilename.c_str());
		l_oSettingEditorDialog.run();
	}
	void CInterfacedScenario::contextMenuRemoveSettingCB(IBox& rBox, uint32 ui32Index)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuRemoveSettingCB\n";
		rBox.removeSetting(ui32Index);
	}
	void CInterfacedScenario::contextMenuConfigureCB(IBox& rBox)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuConfigureCB\n";
		CBoxConfigurationDialog l_oBoxConfigurationDialog(m_rKernel, rBox, m_sGUIFilename.c_str());
		l_oBoxConfigurationDialog.run();
	}
	void CInterfacedScenario::contextMenuAboutCB(IBox& rBox)
	{
		m_rKernel.getContext()->getLogManager() << LogLevel_Debug << "contextMenuAboutCB\n";
		CAboutPluginDialog l_oAboutPluginDialog(m_rKernel, rBox.getAlgorithmClassIdentifier(), m_sGUIFilename.c_str());
		l_oAboutPluginDialog.run();
	}

	void CInterfacedScenario::toggleDesignerVisualisation()
	{
		m_bDesignerVisualisationToggled = !m_bDesignerVisualisationToggled;

		if(m_bDesignerVisualisationToggled)
		{
			m_pDesignerVisualisation->show();
		}
		else
		{
			m_pDesignerVisualisation->hide();
		}
	}

	boolean CInterfacedScenario::isDesignerVisualisationToggled()
	{
		return m_bDesignerVisualisationToggled;
	}

	void CInterfacedScenario::showCurrentVisualisation()
	{
		if(isLocked())
		{
			if(m_pPlayerVisualisation != NULL)
			{
				m_pPlayerVisualisation->showTopLevelWindows();
			}
		}
		else
		{
			if(m_pDesignerVisualisation != NULL)
			{
				m_pDesignerVisualisation->show();
			}
		}
	}

	void CInterfacedScenario::hideCurrentVisualisation()
	{
		if(isLocked())
		{
			if(m_pPlayerVisualisation != NULL)
			{
				m_pPlayerVisualisation->hideTopLevelWindows();
			}
		}
		else
		{
			if(m_pDesignerVisualisation != NULL)
			{
				m_pDesignerVisualisation->hide();
			}
		}

	}

	void CInterfacedScenario::createPlayerVisualisation()
	{
		//hide window manager
		m_pDesignerVisualisation->hide();

		if(m_pPlayerVisualisation == NULL)
		{
			m_pPlayerVisualisation = new CPlayerVisualisation(*m_rKernel.getContext(), m_rScenario, *m_pVisualisationTree);
		}

		//initialize and show windows
		m_pPlayerVisualisation->init();
	}

	void CInterfacedScenario::releasePlayerVisualisation(void)
	{
		if(m_pPlayerVisualisation != NULL)
		{
			delete m_pPlayerVisualisation;
			m_pPlayerVisualisation = NULL;
		}

		//reset designer visualisation
		m_pDesignerVisualisation->reset();

		//show it if it was toggled on
		if(m_bDesignerVisualisationToggled == true)
		{
			m_pDesignerVisualisation->show();
		}
	}

	void CInterfacedScenario::generateDisplayPluginName(IBox* pDisplayBox, CString& rDisplayBoxName)
	{
		rDisplayBoxName = pDisplayBox->getName();
		char buf[10];
		int num = 2;

		CIdentifier l_oBoxIdentifier = m_rScenario.getNextBoxIdentifier(OV_UndefinedIdentifier);

		//for all boxes contained in scenario
		while(l_oBoxIdentifier!=OV_UndefinedIdentifier)
		{
			const IBox* l_pBox2=m_rScenario.getBoxDetails(l_oBoxIdentifier);

			if(l_pBox2 != NULL && l_pBox2 != pDisplayBox)
			{
				//a box already has the same name
				if(l_pBox2->getName() == rDisplayBoxName)
				{
					//generate a new name and ensure it is not used yet
					sprintf(buf, " %d", num++);
					rDisplayBoxName = pDisplayBox->getName() + CString(buf);
					l_oBoxIdentifier = OV_UndefinedIdentifier;
				}
			}
			l_oBoxIdentifier = m_rScenario.getNextBoxIdentifier(l_oBoxIdentifier);
		}
	}
