#include "ovdCAbstractSettingView.h"

using namespace OpenViBE;
using namespace OpenViBEDesigner;
using namespace OpenViBEDesigner::Setting;

OpenViBE::CString CAbstractSettingView::getSettingWidgetName(void)
{
	return m_sSettingWidgetName;
}

CAbstractSettingView::~CAbstractSettingView()
{
	if(m_pNameWidget){
		gtk_widget_destroy(m_pNameWidget);
	}
	if(m_pEntryWidget){
		gtk_widget_destroy(m_pEntryWidget);
	}
}

CAbstractSettingView::CAbstractSettingView(OpenViBE::Kernel::IBox& rBox, OpenViBE::uint32 ui32Index):
	m_rBox(rBox),
	m_ui32Index(ui32Index),
	m_sSettingWidgetName(""),
	m_pNameWidget(NULL),
	m_pEntryWidget(NULL)
{

}



OpenViBE::Kernel::IBox& CAbstractSettingView::getBox(void)
{
	return m_rBox;
}

OpenViBE::uint32 CAbstractSettingView::getSettingsIndex(void)
{
	return m_ui32Index;
}

void CAbstractSettingView::setSettingWidgetName(const CString &rWidgetName)
{
	m_sSettingWidgetName = rWidgetName;
}



void CAbstractSettingView::setNameWidget(::GtkWidget* pWidget)
{
	if(m_pNameWidget){
		gtk_widget_destroy(m_pNameWidget);
	}
	m_pNameWidget = pWidget;
}

::GtkWidget* CAbstractSettingView::getNameWidget(void)
{
	return m_pNameWidget;
}

void CAbstractSettingView::setEntryWidget(::GtkWidget* pWidget)
{
	if(m_pEntryWidget){
		gtk_widget_destroy(m_pEntryWidget);
	}
	m_pEntryWidget = pWidget;
}

::GtkWidget* CAbstractSettingView::getEntryWidget(void)
{
	return m_pEntryWidget;
}
