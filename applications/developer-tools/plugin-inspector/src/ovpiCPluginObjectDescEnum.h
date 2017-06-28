#ifndef __OpenViBEPluginInspector_CPluginObjectDescEnum_H__
#define __OpenViBEPluginInspector_CPluginObjectDescEnum_H__

#include "ovpi_base.h"

#include <string>

class CPluginObjectDescEnum
{
public:

	CPluginObjectDescEnum(const OpenViBE::Kernel::IKernelContext& rKernelContext);
	virtual ~CPluginObjectDescEnum(void);

	virtual bool enumeratePluginObjectDesc(void);
	virtual bool enumeratePluginObjectDesc(
		const OpenViBE::CIdentifier& rParentClassIdentifier);

	virtual bool callback(
		const OpenViBE::Plugins::IPluginObjectDesc& rPluginObjectDesc)=0;

	static std::string transform(const std::string& sInput, const bool bRemoveSlash=false);

protected:

	virtual OpenViBE::Kernel::ILogManager& getLogManager(void) const { return m_rKernelContext.getLogManager(); }
	virtual OpenViBE::Kernel::IErrorManager& getErrorManager(void) const { return m_rKernelContext.getErrorManager(); }
	const OpenViBE::Kernel::IKernelContext& m_rKernelContext;
};

#endif // __OpenViBEPluginInspector_CPluginObjectDescEnum_H__
