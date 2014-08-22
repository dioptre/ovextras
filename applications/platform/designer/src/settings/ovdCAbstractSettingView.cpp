#include "ovdCAbstractSettingView.h"

using namespace OpenViBE;
using namespace OpenViBEDesigner;
using namespace OpenViBEDesigner::Setting;

OpenViBE::CString CAbstractSettingView::OpenWigetSettingWidgetName(void)
{
	return m_sSettingWidgetName;
}

CAbstractSettingView::CAbstractSettingView(OpenViBE::Kernel::IBox& rBox, OpenViBE::uint32 ui32Index):
	m_rBox(rBox), m_ui32Index(ui32Index), m_sSettingWidgetName("")
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

void CAbstractSettingView::setSettingWidgetName(OpenViBE::CString & rWidgetName)
{
	m_sSettingWidgetName = rWidgetName;
}
