#include "ovdCSettingViewFactory.h"

#include "ovdCBooleanSettingView.h"
#include "ovdCIntegerSettingView.h"

using namespace OpenViBEDesigner;
using namespace OpenViBE;
using namespace OpenViBEDesigner::Setting;

CSettingViewFactory::CSettingViewFactory(const CString &rBuilderName): m_sBuilderName(rBuilderName)
{
	m_pBuilder = gtk_builder_new();
	gtk_builder_add_from_file(m_pBuilder, rBuilderName.toASCIIString(), NULL);
	gtk_builder_connect_signals(m_pBuilder, NULL);

}

CAbstractSettingView *CSettingViewFactory::getSettingView(Kernel::IBox &rBox,
														  uint32 ui32Index,
														  const Kernel::IKernelContext& rKernelContext)
{
	CIdentifier l_oSettingType;
	rBox.getSettingType(ui32Index, l_oSettingType);

	if(l_oSettingType==OV_TypeId_Boolean)
		return new CBooleanSettingView(rBox, ui32Index, m_sBuilderName);
	if(l_oSettingType==OV_TypeId_Integer)
		return new CIntegerSettingView(rBox, ui32Index, m_sBuilderName, rKernelContext);
	if(l_oSettingType==OV_TypeId_Float)
		return NULL;
	if(l_oSettingType==OV_TypeId_String)
		return NULL;
	if(l_oSettingType==OV_TypeId_Filename)
		return NULL;
	if(l_oSettingType==OV_TypeId_Script)
		return NULL;
	if(l_oSettingType==OV_TypeId_Color)
		return NULL;
	if(l_oSettingType==OV_TypeId_ColorGradient)
		return NULL;
	if(rKernelContext.getTypeManager().isEnumeration(l_oSettingType))
		return NULL;
	if(rKernelContext.getTypeManager().isBitMask(l_oSettingType))
		return NULL;


	return NULL;
}
