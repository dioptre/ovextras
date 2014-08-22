#include "ovdCSettingViewFactory.h"

using namespace OpenViBEDesigner;
using namespace OpenViBE;
using namespace OpenViBEDesigner::Setting;

CAbstractSettingView *CSettingViewFactory::getSettingView(Kernel::IBox &rBox,
														  uint32 ui32Index,
														  Kernel::IKernelContext& kernelContext)
{
	CIdentifier l_oSettingType;
	rBox.getSettingType(ui32Index, l_oSettingType);

	if(l_oSettingType==OV_TypeId_Boolean)
		return NULL;
	if(l_oSettingType==OV_TypeId_Integer)
		return NULL;
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
	if(kernelContext.getTypeManager().isEnumeration(l_oSettingType))
		return NULL;
	if(kernelContext.getTypeManager().isBitMask(l_oSettingType))
		return NULL;


	return NULL;
}
