#include "ovasCAcquisitionServerGUI.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <gtk/gtk.h>

#include <iostream>

#include <system/ovCTime.h>

#if defined(TARGET_OS_Windows)
  #include <Windows.h>
  #include <MMSystem.h>
#endif

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

typedef struct _SConfiguration
{
	_SConfiguration(void)
	{
	}

	// <name, value>
	std::map < std::string, std::string > m_oFlag;
	std::map < std::string, std::string > m_oTokenMap;
} SConfiguration;

boolean parse_arguments(int argc, char** argv, SConfiguration& rConfiguration)
{
	SConfiguration l_oConfiguration;

	std::vector < std::string > l_vArgValue;
	for(int i=1; i<argc; i++)
	{
		l_vArgValue.push_back(argv[i]);
	}
	l_vArgValue.push_back("");

	for(auto it=l_vArgValue.begin(); it!=l_vArgValue.end(); it++)
	{
		if(*it=="")
		{
		}
		else if(*it=="-c" || *it=="--config")
		{
			if(*++it=="") { std::cout << "Error: Switch --config needs an argument\n"; return false; }
			l_oConfiguration.m_oFlag["config"] = *it;
		}
		else if(*it=="-d" || *it=="--define")
		{
			if(*++it=="") {
				std::cout << "Error: Need two arguments after -d / --define.\n";
				return false;
			}

			// Were not using = as a separator for token/value, as on Windows its a problem passing = to the cmd interpreter 
			// which is used to launch the actual designer exe.
			const std::string& l_rToken = *it;
			if(*++it=="") {
				std::cout << "Error: Need two arguments after -d / --define.\n";
				return false;
			}

			const std::string& l_rValue = *it;	// iterator will increment later
			
			l_oConfiguration.m_oTokenMap[l_rToken] = l_rValue;

		}
		else if(*it=="-k" || *it=="--kernel")
		{
			if(*++it=="") { std::cout << "Error: Switch --kernel needs an argument\n"; return false; }
			l_oConfiguration.m_oFlag["kernel"] = *it;
		}
		else if(*it=="-h" || *it=="--help")
		{
			return false;
		}
		else
		{
			// The argument may be relevant to GTK, do not stop here
			std::cout << "Note: Unknown argument [" << *it << "], passing it on to gtk...\n";
		}
	}

	rConfiguration=l_oConfiguration;

	return true;
}


int main(int argc, char ** argv)
{
//___________________________________________________________________//
//                                                                   //

	SConfiguration l_oConfiguration;
	if(!parse_arguments(argc, argv, l_oConfiguration))
	{
		cout << "Syntax : " << argv[0] << " [ switches ]\n";
		cout << "Possible switches :\n";
		cout << "  --config filename       : path to config file\n";
		cout << "  --define token value    : specify configuration token with a given value\n";
		cout << "  --help                  : displays this help message and exits\n";
		cout << "  --kernel filename       : path to openvibe kernel library\n";
		return -1;
	}

#if defined(TARGET_OS_Windows)
	HANDLE l_oProcess = GetCurrentProcess();

	// Some sources claim this is needed for accurate timing. Microsoft disagrees, so we do not use it. You can try, or try google. 
	//SetThreadAffinityMask(hProcess, threadMask);

	// Set the clock interval to 1ms (default on Win7: 15ms). This is needed to get under 15ms accurate sleeps,
	// and improves the precision of non-QPC clocks. Note that since boost 1.58, the sleeps no longer seem
	// to be 1ms accurate on Windows (as they seemed to be on 1.55), and sleep can oversleep even 10ms even with 
	// timeBeginPeriod(1) called. @todo in the future, make sure nothing relies on sleep accuracy in openvibe
	timeBeginPeriod(1); 

	// Since AS is just sleeping when its not acquiring, a high priority should not be a problem. 
	// As a result of these calls, the server should have a 'normal' priority INSIDE the 'realtime' priority class.
	// However, unless you run AS with admin priviledges, Windows probably will truncate these priorities lower.
	// n.b. For correct timing, it may be preferable to set the priority here globally and not mess with it in the drivers;
	// any child threads should inherit this automagically.
	SetPriorityClass(l_oProcess, REALTIME_PRIORITY_CLASS);		// The highest priority class
	SetThreadPriority(l_oProcess, THREAD_PRIORITY_NORMAL);		// Even higher options: THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_TIME_CRITICAL
#endif

	CKernelLoader l_oKernelLoader;

	cout<<"[  INF  ] Created kernel loader, trying to load kernel module"<<endl;
	CString l_sError;
#if defined TARGET_OS_Windows
	CString l_sKernelFile = OpenViBE::Directories::getLibDir() + "/openvibe-kernel.dll";
#else
	CString l_sKernelFile = OpenViBE::Directories::getLibDir() + "/libopenvibe-kernel.so";
#endif
	if(l_oConfiguration.m_oFlag.count("kernel")) 
	{
		l_sKernelFile = CString(l_oConfiguration.m_oFlag["kernel"].c_str());
	}
	if(!l_oKernelLoader.load(l_sKernelFile, &l_sError))
	{
		cout<<"[ FAILED ] Error loading kernel from [" << l_sKernelFile << "]: " << l_sError << "\n";
	}
	else
	{
		cout<<"[  INF  ] Kernel module loaded, trying to get kernel descriptor"<<endl;
		IKernelDesc* l_pKernelDesc=NULL;
		IKernelContext* l_pKernelContext=NULL;
		l_oKernelLoader.initialize();
		l_oKernelLoader.getKernelDesc(l_pKernelDesc);
		if(!l_pKernelDesc)
		{
			cout<<"[ FAILED ] No kernel descriptor"<<endl;
		}
		else
		{
			cout<<"[  INF  ] Got kernel descriptor, trying to create kernel"<<endl;


			CString l_sConfigFile = CString(OpenViBE::Directories::getDataDir() + "/kernel/openvibe.conf");
			if(l_oConfiguration.m_oFlag.count("config")) 
			{
				l_sConfigFile = CString(l_oConfiguration.m_oFlag["config"].c_str());
			}

			l_pKernelContext=l_pKernelDesc->createKernel("acquisition-server", l_sConfigFile);
			if(!l_pKernelContext)
			{
				cout<<"[ FAILED ] No kernel created by kernel descriptor"<<endl;
			}
			else
			{
				l_pKernelContext->initialize();

				IConfigurationManager& l_rConfigurationManager=l_pKernelContext->getConfigurationManager();

				// @FIXME CERT silent fail if missing file is provided
				l_rConfigurationManager.addConfigurationFromFile(l_rConfigurationManager.expand("${Path_Data}/applications/acquisition-server/acquisition-server-defaults.conf"));

				// User configuration mods
				l_rConfigurationManager.addConfigurationFromFile(l_rConfigurationManager.expand("${Path_UserData}/openvibe-acquisition-server.conf"));

				l_pKernelContext->getPluginManager().addPluginsFromFiles(l_rConfigurationManager.expand("${AcquisitionServer_Plugins}"));

				for(auto itr=l_oConfiguration.m_oTokenMap.begin();
					itr!=l_oConfiguration.m_oTokenMap.end();
					itr++)
				{
					l_pKernelContext->getLogManager() << LogLevel_Trace << "Adding command line configuration token [" << (*itr).first.c_str() << " = " << (*itr).second.c_str() << "]\n";
					l_rConfigurationManager.addOrReplaceConfigurationToken((*itr).first.c_str(), (*itr).second.c_str());
				}

				// Check the clock
				if(!System::Time::isClockSteady())
				{
					l_pKernelContext->getLogManager() << LogLevel_Warning << "The system does not seem to have a steady clock. This may affect the acquisition time precision.\n";
				}

				if(!gtk_init_check(&argc, &argv))
				{
					l_pKernelContext->getLogManager() << LogLevel_Error << "Unable to initialize GTK. Possibly the display could not be opened. Exiting.\n";
					
					OpenViBEToolkit::uninitialize(*l_pKernelContext);
					l_pKernelDesc->releaseKernel(l_pKernelContext);

					l_oKernelLoader.uninitialize();
					l_oKernelLoader.unload();

#if defined(TARGET_OS_Windows)
					timeEndPeriod(1);
#endif

					return -2;
				}

				// gtk_rc_parse(OpenViBE::Directories::getDataDir() + "/applications/designer/interface.gtkrc");

#ifdef TARGET_OS_Linux
				// Replace the gtk signal handlers with the default ones. As a result, 
				// the following exits on terminating signals won't be graceful, 
				// but its better than not exiting at all (gtk default on Linux apparently)
				signal(SIGHUP, SIG_DFL);
				signal(SIGINT, SIG_DFL);
				signal(SIGQUIT, SIG_DFL);
#endif

				{
					// If this is encapsulated by gdk_threads_enter() and gdk_threads_exit(), m_pThread->join() can hang when gtk_main() returns before destructor of app has been called.
					OpenViBEAcquisitionServer::CAcquisitionServerGUI app(*l_pKernelContext);

					try
					{
						gtk_main();	
					}
					catch(...)
					{
						l_pKernelContext->getLogManager() << LogLevel_Fatal << "Catched top level exception\n";
					}
				}

				cout<<"[  INF  ] Application terminated, releasing allocated objects"<<endl;

				OpenViBEToolkit::uninitialize(*l_pKernelContext);

				l_pKernelDesc->releaseKernel(l_pKernelContext);
			}
		}
		l_oKernelLoader.uninitialize();
		l_oKernelLoader.unload();
	}
			
#if defined(TARGET_OS_Windows)
	timeEndPeriod(1);
#endif

	return 0;
}
