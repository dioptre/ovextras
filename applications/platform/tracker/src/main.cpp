//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include "Tracker.h"

#include "GUI.h"

#include "Testclass.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

class KernelWrapper
{
public:
	KernelWrapper() : m_KernelContext(nullptr), m_KernelDesc(nullptr)
	{

	}

	~KernelWrapper()
	{

		if(m_KernelContext)
		{
			std::cout << "Unloading kernel" << endl;
			OpenViBEToolkit::uninitialize(*m_KernelContext);
			m_KernelDesc->releaseKernel(m_KernelContext);
			m_KernelContext = nullptr;
		}

		std::cout << "Unloading loader" << endl;
		m_KernelLoader.uninitialize();
		m_KernelLoader.unload();	
	}

	bool initialize(void)
	{

		std::cout << "[  INF  ] Created kernel loader, trying to load kernel module" << std::endl;
		CString l_sError;
#if defined TARGET_OS_Windows
		CString l_sKernelFile = OpenViBE::Directories::getLibDir() + "/openvibe-kernel.dll";
#else
		CString l_sKernelFile = OpenViBE::Directories::getLibDir() + "/libopenvibe-kernel.so";
#endif
		if(!m_KernelLoader.load(l_sKernelFile, &l_sError))
		{
			std::cout << "[ FAILED ] Error loading kernel from [" << l_sKernelFile << "]: " << l_sError << "\n";
			return false;
		}

		std::cout << "[  INF  ] Kernel module loaded, trying to get kernel descriptor" << std::endl;
		m_KernelLoader.initialize();
		m_KernelLoader.getKernelDesc(m_KernelDesc);
		if(!m_KernelDesc)
		{
			cout<<"[ FAILED ] No kernel descriptor"<<endl;
			return false;
		}

		cout<<"[  INF  ] Got kernel descriptor, trying to create kernel"<<endl;

		CString l_sConfigFile = CString(OpenViBE::Directories::getDataDir() + "/kernel/openvibe.conf");

		m_KernelContext=m_KernelDesc->createKernel("tracker", l_sConfigFile);
		if(!m_KernelContext)
		{
			cout<<"[ FAILED ] No kernel created by kernel descriptor"<<endl;
			return false;
		}

		m_KernelContext->initialize();

		IConfigurationManager& l_rConfigurationManager=m_KernelContext->getConfigurationManager();

		l_rConfigurationManager.addConfigurationFromFile(l_rConfigurationManager.expand("${Path_Data}/applications/acquisition-server/acquisition-server-defaults.conf"));

		// User configuration mods
		l_rConfigurationManager.addConfigurationFromFile(l_rConfigurationManager.expand("${Path_UserData}/openvibe-tracker.conf"));

		m_KernelContext->getPluginManager().addPluginsFromFiles(l_rConfigurationManager.expand("${AcquisitionServer_Plugins}"));

		return true;
	}
	
	IKernelDesc* m_KernelDesc;
	IKernelContext* m_KernelContext;
	CKernelLoader m_KernelLoader;

};

int main(int argc, char *argv[])
{
	KernelWrapper kernelWrapper;

	if(!kernelWrapper.initialize())
	{
		return 1;
	}

	Tracker app(*kernelWrapper.m_KernelContext);

	GUI gui(argc, argv, app);

	return gui.run();
}

