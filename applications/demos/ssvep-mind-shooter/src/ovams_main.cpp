
/*
 * Notes:
 *
 * - This demo used to be called 'Impact Shooter' before being renamed to 'Mind Shooter'. The various 
 * mentions of 'Impact' are propagating from that time and should be cleaned up one day.
 *
 *
 */ 
#if defined(TARGET_HAS_ThirdPartyOgre3DTerrain)

#include "ovams_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <openvibe/ov_directories.h>

#include "ovamsCApplication.h"
#include "GenericStimulator/ovamsCGenericStimulatorApplication.h"
#include "Impact/ovamsCImpactApplication.h"

using namespace OpenViBE;

/**
 */
int main(int argc, char** argv)
{
	if (argc != 3)
	{
		printf("Usage : %s <application-type> <scenario path>\n", argv[0]);
		exit(1);
	}
	
	// initialize the OpenViBE kernel
	
	OpenViBE::CKernelLoader l_oKernelLoader;
	OpenViBE::CString l_sError;
	OpenViBE::Kernel::IKernelDesc* l_poKernelDesc = NULL;
	OpenViBE::Kernel::IKernelContext* l_poKernelContext = NULL;
	OpenViBE::Kernel::ILogManager* l_poLogManager = NULL;
	OpenViBE::Kernel::IConfigurationManager* l_poConfigurationManager = NULL;

	CString l_sApplicationString = CString(argv[1]);
	CString l_sScenarioFolder = CString(argv[2]);

	CString l_sApplicationType, l_sApplicationSubtype;

	if (l_sApplicationString == CString("generic"))
	{
		l_sApplicationType = "generic";
		l_sApplicationSubtype = "";
	}
	else if (l_sApplicationString == CString("impact-trainer"))
	{
		l_sApplicationType = "impact";
		l_sApplicationSubtype = "trainer";
	}
	else if (l_sApplicationString == CString("impact-shooter"))
	{
		l_sApplicationType = "impact";
		l_sApplicationSubtype = "shooter";
	}
	else
	{
		std::cout << "[ERROR] unknown command line parameter '" << l_sApplicationString << "'\n";
		exit(3);
	}

#ifdef TARGET_OS_Windows
	std::cout << "[  INF  ] Loading Windows kernel\n";
	if(!l_oKernelLoader.load(OpenViBE::Directories::getBinDir() + "/openvibe-kernel.dll", &l_sError))
#else
	std::cout << "[  INF  ] Loading Linux kernel\n";
	if(!l_oKernelLoader.load(OpenViBE::Directories::getLibDir() + "/libopenvibe-kernel.so", &l_sError))
#endif
	{
		std::cout << "[ FAILED ] Error loading kernel (" << l_sError << ")" << "\n";
		exit(1);
	}
	else
	{
		std::cout<< "[  INF  ] Kernel module loaded, trying to get kernel descriptor\n";

		l_oKernelLoader.initialize();
		l_oKernelLoader.getKernelDesc(l_poKernelDesc);

		if(!l_poKernelDesc)
		{
			std::cout << "[ FAILED ] No kernel descriptor\n";
		}
		else
		{
			std::cout << "[  INF  ] Got kernel descriptor, trying to create kernel\n";

			l_poKernelContext = l_poKernelDesc->createKernel("ssvep-stimulator", OpenViBE::Directories::getDataDir() + "/kernel/openvibe.conf");

			if(!l_poKernelContext)
			{
				std::cout << "[ FAILED ] No kernel created by kernel descriptor\n";
			}
			else
			{
				OpenViBEToolkit::initialize(*l_poKernelContext);

				l_poConfigurationManager = &(l_poKernelContext->getConfigurationManager());
				l_poConfigurationManager->addConfigurationFromFile(l_poConfigurationManager->expand("${Path_Data}/kernel/openvibe.conf"));
				/*
				l_poConfigurationManager->createConfigurationToken("SSVEP_ApplicationDescriptor", CString(argv[1]));
				(*l_poLogManager) << OpenViBE::Kernel::LogLevel_Info << "Application Descriptor : " << argv[1] << "\n";
				*/
				/*
				l_poConfigurationManager->createConfigurationToken("SSVEP_ApplicationDescriptor", l_sApplicationType);
				l_poConfigurationManager->createConfigurationToken("SSVEP_ApplicationSubtype", l_sApplicationSubtype);
				*/
				l_poLogManager = &(l_poKernelContext->getLogManager());

				// (*l_poLogManager) << OpenViBE::Kernel::LogLevel_Info << l_poConfigurationManager->expand("${UserHome}") << "\n";
				const CString l_sConfigFile = l_sScenarioFolder + "/appconf/application-configuration.conf";
				if(!l_poConfigurationManager->addConfigurationFromFile(l_sConfigFile)) 
				{
					(*l_poLogManager) << OpenViBE::Kernel::LogLevel_Error << "Unable to read [" << l_sConfigFile << "]\n";
					exit(3);
				}

				// This folder contains resources such as textures etc
				l_poConfigurationManager->createConfigurationToken("SSVEP_MindShooterFolderName", "ssvep-mind-shooter");

				// This folder is the runtime scenario path
				l_poConfigurationManager->createConfigurationToken("SSVEP_MindShooterScenarioPath", l_sScenarioFolder);
			}
		}
	}



	OpenViBESSVEPMindShooter::CApplication* l_pApp = NULL;



	if (l_sApplicationType == CString("generic"))
	{
		(*l_poLogManager) << OpenViBE::Kernel::LogLevel_Debug << "+ app = new OpenViBESSVEPMindShooter::CGenericStimulatorApplication(...)\n";
		l_pApp = new OpenViBESSVEPMindShooter::CGenericStimulatorApplication(l_sScenarioFolder);
	}
	else if (l_sApplicationType == CString("impact"))
	{
		(*l_poLogManager) << OpenViBE::Kernel::LogLevel_Debug << "+ app = new OpenViBESSVEPMindShooter::CImpactApplication(...)\n";
		(*l_poLogManager) << OpenViBE::Kernel::LogLevel_Info << "application subtype " << l_sApplicationSubtype << "\n";
		l_pApp = new OpenViBESSVEPMindShooter::CImpactApplication(l_sScenarioFolder, l_sApplicationSubtype);
	}
	else
	{
		(*l_poLogManager) << OpenViBE::Kernel::LogLevel_Error << "Wrong application identifier specified\n";

		exit(1);
	}

	if(!l_pApp->setup(l_poKernelContext))
	{
		(*l_poLogManager) << OpenViBE::Kernel::LogLevel_Error << "Cannot proceed, exiting\n";

		exit(2);
	}
	  
	l_pApp->go();


	(*l_poLogManager) << OpenViBE::Kernel::LogLevel_Debug << "- app\n";
	delete l_pApp;

	return 0;
}



#else
#include <stdio.h>

int main(int argc, char** argv)
{
	printf("SSVEP Mind Shooter has not been compiled as it depends on Ogre (missing/disabled)\n");

	return -1;
}
#endif