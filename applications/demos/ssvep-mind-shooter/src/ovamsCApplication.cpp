
#if defined(TARGET_HAS_ThirdPartyOgre3DTerrain)

#include "ovamsCApplication.h"
#include <cmath>
#include <algorithm>

#include "fs/Files.h"

using namespace OpenViBE;
using namespace OpenViBESSVEPMindShooter;
using namespace OpenViBE::Kernel;

CApplication::CApplication(CString scenarioDir)
	: m_sScenarioDir(scenarioDir),
	  m_pStimulusSender(NULL),
	  m_bContinueRendering( true ),
	  m_ui32CurrentFrame( 0 ),
	  m_ui64CurrentTime( 0 ),
	  m_roGUIRenderer( NULL )
{
	m_pStimulusSender = TCPTagging::createStimulusSender();
}

CApplication::~CApplication()
{
	delete m_pStimulusSender;

	if (m_poPainter != NULL)
	{
		(*m_poLogManager) << LogLevel_Debug << "- m_poPainter\n";
		delete m_poPainter;
		m_poPainter = NULL;
	}

	for (std::vector<ICommand*>::iterator it = m_oCommands.begin();
		 it != m_oCommands.end(); ++it)
	{
		(*m_poLogManager) << LogLevel_Debug << "- ICommand\n";
		if (*it != NULL)
		{
			delete *it;
			*it = NULL;
		}
	}


	(*m_poLogManager) << LogLevel_Debug << "- m_poRoot\n";
	if (m_poRoot != NULL)
	{
		delete m_poRoot;
		m_poRoot = NULL;
	}

}

bool CApplication::setup(OpenViBE::Kernel::IKernelContext* poKernelContext)
{
	m_poKernelContext = poKernelContext;
	m_poLogManager = &(m_poKernelContext->getLogManager());

	IConfigurationManager* l_poConfigurationManager = &(m_poKernelContext->getConfigurationManager());

	(*m_poLogManager) << LogLevel_Debug << "  * CApplication::setup()\n";

	// Plugin config path setup
	Ogre::String l_oPluginsPath;

#if defined TARGET_OS_Windows
#if defined TARGET_BUILDTYPE_Debug
	l_oPluginsPath = std::string(getenv("OGRE_HOME")) + std::string("/bin/debug/plugins_d.cfg");
#else
	l_oPluginsPath = std::string(getenv("OGRE_HOME")) + std::string("/bin/release/plugins.cfg");
#endif
#elif defined TARGET_OS_Linux
	l_oPluginsPath = std::string(l_poConfigurationManager->expand("${Path_Data}/openvibe-ogre-plugins.cfg").toASCIIString());
#else
#error "No OS defined."
#endif

	// Create LogManager to stop Ogre flooding the console and creating random files


	(*m_poLogManager) << LogLevel_Debug << "+ Creating Ogre logmanager\n";
	Ogre::LogManager* l_poLogManager = new Ogre::LogManager();
	(*m_poLogManager) << LogLevel_Info << "Log level: " << l_poConfigurationManager->expand("${Kernel_ConsoleLogLevel}") << "\n";
	(*m_poLogManager) << LogLevel_Info << "Application will output Ogre console log : " << l_poConfigurationManager->expandAsBoolean("${SSVEP_Ogre_LogToConsole}", false) << "\n";
	CString l_sOgreLog = l_poConfigurationManager->expand("${SSVEP_UserDataFolder}/mind-shooter-[$core{date}-$core{time}]-ogre.log");
	(*m_poLogManager) << LogLevel_Info << "Ogre log file : " << l_sOgreLog << "\n";
	FS::Files::createParentPath(l_sOgreLog);
	l_poLogManager->createLog(l_sOgreLog.toASCIIString(), true, l_poConfigurationManager->expandAsBoolean("${SSVEP_Ogre_LogToConsole}", false), false );

	// Root creation
	CString l_sOgreCfg = l_poConfigurationManager->expand("${SSVEP_MindShooterScenarioPath}") + "/appconf/mind-shooter-ogre.conf";
	(*m_poLogManager) << LogLevel_Debug << "+ m_poRoot = new Ogre::Root(...)\n";
	(*m_poLogManager) << LogLevel_Info << "Ogre cfg file : " << l_sOgreCfg << "\n";
	m_poRoot = new Ogre::Root(l_oPluginsPath, l_sOgreCfg.toASCIIString(), l_sOgreLog.toASCIIString());

	// Resource handling
	this->setupResources();

	// Configuration from file or dialog window if needed
	if (!this->configure())
	{
		(*m_poLogManager) << LogLevel_Fatal << "The configuration process ended unexpectedly.\n";
		return false;
	}

	// m_poWindow = m_poRoot->initialise(true);


	Ogre::NameValuePairList l_oOptionList;
	m_poRoot->initialise(false);


	l_oOptionList["vsync"] = "1";

	int l_iWidth = (int) l_poConfigurationManager->expandAsInteger("${SSVEP_Ogre_ScreenWidth}", 800);
	int l_iHeight = (int) l_poConfigurationManager->expandAsInteger("${SSVEP_Ogre_ScreenHeight}", 600);
	const OpenViBE::boolean l_bFullScreen = l_poConfigurationManager->expandAsBoolean("${SSVEP_Ogre_FullScreen}", false);

	(*m_poLogManager) << LogLevel_Info << "Width : " << l_iWidth << " Height : " << l_iHeight << " Fullscreen : " << l_bFullScreen << "\n";

	//m_poWindow = m_poRoot->createRenderWindow("SSVEP Stimulator", 960, 600, false, &l_oOptionList);
	m_poWindow = m_poRoot->createRenderWindow("SSVEP Stimulator", l_iWidth, l_iHeight, l_bFullScreen, &l_oOptionList);

	m_ui32WindowWidth = m_poWindow->getWidth();
	m_ui32WindowHeight = m_poWindow->getHeight();

	m_poSceneManager = m_poRoot->createSceneManager(Ogre::ST_GENERIC);
	m_poCamera = m_poSceneManager->createCamera("SSVEPApplicationCamera");
	m_poCameraNode = m_poSceneManager->getRootSceneNode()->createChildSceneNode();
	m_poCameraNode->attachObject(m_poCamera);

	Ogre::SceneManager* l_poFillSceneManager = m_poRoot->createSceneManager(Ogre::ST_GENERIC);
	Ogre::Camera* l_poFillCamera = l_poFillSceneManager->createCamera("SSVEPFillCamera");
	m_poWindow->addViewport(l_poFillCamera, 0);

	m_poViewport = m_poWindow->addViewport(m_poCamera, 1);
	//this->resizeViewport();
	//	m_poViewport->setBackgroundColour(Ogre::ColourValue(0.0, 0.5, 0.5));

	m_poCamera->setAspectRatio(Ogre::Real(m_poViewport->getActualWidth()) / Ogre::Real(m_poViewport->getActualHeight()));

	m_poSceneNode = m_poSceneManager->getRootSceneNode()->createChildSceneNode("SSVEPApplicationNode");

	// initialize the painter object
	(*m_poLogManager) << LogLevel_Debug << "+ m_poPainter = new CBasicPainter(...)\n";
	m_poPainter = new CBasicPainter( this );

	(*m_poLogManager) << LogLevel_Debug << "  * initializing CEGUI\n";
	this->initCEGUI(l_poConfigurationManager->expand("${SSVEP_UserDataFolder}/mind-shooter-[$core{date}-$core{time}]-cegui.log"));

	(*m_poLogManager) << LogLevel_Debug << "  * CEGUI initialized\n";

	// create the vector of stimulation frequencies

	m_f64ScreenRefreshRate = (OpenViBE::uint32)(l_poConfigurationManager->expandAsUInteger("${SSVEP_ScreenRefreshRate}", 60));
	(*m_poLogManager) << LogLevel_Info << "Specified screen refresh rate :" << m_f64ScreenRefreshRate << "Hz\n";

	if (m_f64ScreenRefreshRate > 64)
	{
		(*m_poLogManager) << LogLevel_Error << "Screen refresh rate exceeds the max bit pattern length of 64\n";
		return false;
	}

	OpenViBE::uint32 i = 1;

	char l_sFrequencyString[32] = "1";

	CIdentifier l_oFrequencyId = l_poConfigurationManager->createConfigurationToken("SSVEP_FrequencyId", CString(l_sFrequencyString));

	m_oFrequencies.push_back(30);

	OpenViBE::uint32 l_ui32PatternsLoaded = 0;

	// TODO: Load patterns

	// Load pre-defined stimulation patterns (binary encoded dark/light frames inside a 64bit integer)
	while (l_poConfigurationManager->lookUpConfigurationTokenIdentifier(l_poConfigurationManager->expand("SSVEP_Pattern_${SSVEP_FrequencyId}")) != OV_UndefinedIdentifier)
	{
		OpenViBE::uint64 l_ui64StimulationPattern = (OpenViBE::uint64)(l_poConfigurationManager->expandAsInteger("${SSVEP_Pattern_${SSVEP_FrequencyId}}"));

		(*m_poLogManager) << LogLevel_Info << "Pattern number " << i << " pattern : " << l_ui64StimulationPattern << "\n";
		m_oFrequencies[i] = l_ui64StimulationPattern;

		l_poConfigurationManager->releaseConfigurationToken(l_oFrequencyId);

		sprintf(l_sFrequencyString, "%d", ++i);

		l_oFrequencyId = l_poConfigurationManager->createConfigurationToken("SSVEP_FrequencyId", CString(l_sFrequencyString));
		l_ui32PatternsLoaded++;
	}


	// Generate patterns from frequencies
	if (l_ui32PatternsLoaded == 0)
	{
		// Load frequencies
		while (l_poConfigurationManager->lookUpConfigurationTokenIdentifier(l_poConfigurationManager->expand("SSVEP_Frequency_${SSVEP_FrequencyId}")) != OV_UndefinedIdentifier)
		{
			const float64 l_f64CurrentFrequency = (OpenViBE::float64)(l_poConfigurationManager->expandAsFloat("${SSVEP_Frequency_${SSVEP_FrequencyId}}"));

			const float64 l_f64ApproximatedFrameCount= m_f64ScreenRefreshRate / l_f64CurrentFrequency;

			const uint32 l_ui32RoundedFrameCount = static_cast<uint32>(floor(l_f64ApproximatedFrameCount + 0.5));

			// test if the desired frequency can be reasonably created on the screen
			if (fabs(l_f64ApproximatedFrameCount - l_ui32RoundedFrameCount) < 0.003)
			{
				const uint32 l_ui32FramesL = l_ui32RoundedFrameCount / 2 + l_ui32RoundedFrameCount % 2;
				const uint32 l_ui32FramesD = l_ui32RoundedFrameCount / 2;

				// the pattern is procedurally generated and always starts by a 1, 
				// following by as many 0s as there are Dark frames and finally as many 1s as there are Light frames.
				// The frame loop will consume one bit per frame from the right and then reset when the reset marker is hit.

				// Start with the reset marker
				OpenViBE::uint64 l_ui64StimulationPattern = 1;

				// The dark zeroes
				l_ui64StimulationPattern <<= l_ui32FramesD;

				// The light ones
				for (OpenViBE::uint32 j = 0; j < l_ui32FramesL; j++)
				{
					l_ui64StimulationPattern <<= 1;
					l_ui64StimulationPattern += 1;
				}

				(*m_poLogManager) << LogLevel_Info << "Frequency number " << i << ": " << l_f64CurrentFrequency << "Hz / " << floor(l_f64ApproximatedFrameCount + 0.5) << " ( " << l_ui32FramesL << " light, " << l_ui32FramesD << " dark) frames @ " << m_f64ScreenRefreshRate << "fps\n";
				(*m_poLogManager) << LogLevel_Info << "Frequency number " << i << " pattern : " << l_ui64StimulationPattern << "\n";

				std::stringstream l_ssBinary; l_ssBinary << std::bitset<64>(l_ui64StimulationPattern);
				(*m_poLogManager) << LogLevel_Info << "Frequency number " << i << " binary  : " << l_ssBinary.str().c_str() << "\n";

				m_oFrequencies.push_back(l_ui64StimulationPattern);
				l_ui32PatternsLoaded++;
			}
			else
			{
				(*m_poLogManager) << LogLevel_Error << "The selected frequency (" << l_f64CurrentFrequency << "Hz) is not supported by your screen.\n";
			}

			l_poConfigurationManager->releaseConfigurationToken(l_oFrequencyId);

			sprintf(l_sFrequencyString, "%d", ++i);

			l_oFrequencyId = l_poConfigurationManager->createConfigurationToken("SSVEP_FrequencyId", CString(l_sFrequencyString));
		}
	}

	if(!l_ui32PatternsLoaded)
	{
		(*m_poLogManager) << LogLevel_Error << "No flashing frequencies loaded. Have you run the SSVEP Impact Shooter configuring scenario?\n";
		(*m_poLogManager) << LogLevel_Error << "Are you running this app from the correct scenario with the previous stages run properly?\n";
		return false;
	}

	m_pStimulusSender->connect("localhost", "15361");

	return true;
}

// Set hard-coded parameters, VSync in particular, for all known render systems
void CApplication::setOgreParameters(void)
{
	const OpenViBE::boolean l_bFullScreen = m_poKernelContext->getConfigurationManager().expandAsBoolean("${SSVEP_Ogre_FullScreen}", false);
	const Ogre::RenderSystemList& l_oList = m_poRoot->getAvailableRenderers();
	for (size_t i = 0; i < l_oList.size(); i++)
	{
		l_oList[i]->setConfigOption("VSync", "Yes");
		l_oList[i]->setConfigOption("Full Screen", (l_bFullScreen ? "Yes" : "No"));
	}
}

bool CApplication::configure()
{
	if(! m_poRoot->restoreConfig())
	{
		setOgreParameters();

		if( ! m_poRoot->showConfigDialog() )
		{
			(*m_poLogManager) << LogLevel_Error << "No configuration created from the dialog window.\n";
			return false;
		}
	}

	// Override 'unsuitable' user choices
	(*m_poLogManager) << LogLevel_Info << "Forcing vsync and using fullscreen settings from configuration scenario.\n";
	setOgreParameters();
	
	// Save the config again after we have forced the params, otherwise the conf file looks misleading
	m_poRoot->saveConfig();

	return true;
}


void CApplication::initCEGUI(const char *logFilename)
{
	// Instantiate logger before bootstrapping the system, this way we will be able to get the log redirected
	if (!CEGUI::Logger::getSingletonPtr()) 
	{
		new CEGUI::DefaultLogger();		// singleton; instantiate only, no delete
	}
	(*m_poLogManager) << LogLevel_Info << "+ CEGUI log will be in '" << logFilename << "'\n";
	FS::Files::createParentPath(logFilename);
	CEGUI::Logger::getSingleton().setLogFilename(logFilename, false);

	(*m_poLogManager) << LogLevel_Debug << "+ Creating CEGUI Ogre bootstrap\n";
	m_roGUIRenderer = &(CEGUI::OgreRenderer::bootstrapSystem(*m_poWindow));
	(*m_poLogManager) << LogLevel_Debug << "+ Creating CEGUI Scheme Manager\n";

#if (CEGUI_VERSION_MAJOR > 0) || (CEGUI_VERSION_MINOR >= 8)
	CEGUI::SchemeManager::getSingleton().createFromFile((CEGUI::utf8*)"TaharezLook-ov-0.8.scheme");
#else
	CEGUI::SchemeManager::getSingleton().create((CEGUI::utf8*)"TaharezLook-ov.scheme");
#endif

	(*m_poLogManager) << LogLevel_Debug << "+ Creating CEGUI WindowManager\n";
	m_poGUIWindowManager = CEGUI::WindowManager::getSingletonPtr();
	m_poSheet = m_poGUIWindowManager->createWindow("DefaultWindow", "RootSheet");

	(*m_poLogManager) << LogLevel_Debug << "+ Setting CEGUI StyleSheet\n";
#if (CEGUI_VERSION_MAJOR > 0) || (CEGUI_VERSION_MINOR >= 8)
	CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(m_poSheet);
#else
	CEGUI::System::getSingleton().setGUISheet(m_poSheet);
#endif

}

void CApplication::resizeViewport()
{
	(*m_poLogManager) << LogLevel_Trace << "Creating a new viewport\n";

	Ogre::uint32 l_ui32ViewportSize = std::min(m_ui32WindowWidth, m_ui32WindowHeight);
	(*m_poLogManager) << LogLevel_Info << "New viewport size : " << l_ui32ViewportSize << "\n";

	m_poViewport->setDimensions(
				Ogre::Real(m_ui32WindowWidth - l_ui32ViewportSize) / Ogre::Real(m_ui32WindowWidth) / 2,
				Ogre::Real(m_ui32WindowHeight - l_ui32ViewportSize) / Ogre::Real(m_ui32WindowHeight) / 2,
				Ogre::Real(l_ui32ViewportSize) / Ogre::Real(m_ui32WindowWidth),
				Ogre::Real(l_ui32ViewportSize) / Ogre::Real(m_ui32WindowHeight)
				);
}

void CApplication::processFrame(OpenViBE::uint32 ui32CurrentFrame)
{
	m_ui64CurrentTime++;
}


void CApplication::setupResources()
{
	IConfigurationManager* l_poConfigurationManager = &(m_poKernelContext->getConfigurationManager());

	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(l_poConfigurationManager->expand("${Path_Data}/applications/${SSVEP_MindShooterFolderName}/resources").toASCIIString(), "FileSystem", "SSVEP");
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(l_poConfigurationManager->expand("${Path_Data}/applications/${SSVEP_MindShooterFolderName}/resources/generic").toASCIIString(), "FileSystem", "SSVEPGeneric");
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(l_poConfigurationManager->expand("${Path_Data}/applications/${SSVEP_MindShooterFolderName}/resources/generic/textures").toASCIIString(), "FileSystem", "SSVEPGeneric");
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(l_poConfigurationManager->expand("${Path_Data}/applications/${SSVEP_MindShooterFolderName}/resources/trainer").toASCIIString(), "FileSystem", "SSVEPTrainer");
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(l_poConfigurationManager->expand("${Path_Data}/applications/${SSVEP_MindShooterFolderName}/resources/gui").toASCIIString(), "FileSystem", "CEGUI");
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation(CString(m_sScenarioDir+"/appconf/materials").toASCIIString(), "FileSystem", "CEGUI");

	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("SSVEP");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("SSVEPTrainer");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("SSVEPGeneric");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("CEGUI");
}

bool CApplication::frameRenderingQueued(const Ogre::FrameEvent &evt)
{
	return (m_bContinueRendering && !m_poWindow->isClosed());
}

bool CApplication::frameStarted(const Ogre::FrameEvent &evt)
{
	m_ui32CurrentFrame++;
	m_ui32CurrentFrame %= int(m_f64ScreenRefreshRate);


	for (OpenViBE::uint32 i = 0; i < m_oCommands.size(); i++)
	{
		m_oCommands[i]->processFrame();
	}

	this->processFrame(m_ui32CurrentFrame);

	return true;
}

void CApplication::go()
{
	(*m_poLogManager) << LogLevel_Debug << "Associating application as Ogre frame listener\n";

	m_poRoot->addFrameListener(this);

	(*m_poLogManager) << LogLevel_Debug << "Entering Ogre rendering loop\n";
	m_poRoot->startRendering();
	(*m_poLogManager) << LogLevel_Debug << "Ogre rendering loop finished ... exiting\n";
}

void CApplication::addCommand(ICommand* pCommand)
{
	m_oCommands.push_back(pCommand);
}

void CApplication::startExperiment()
{
	(*m_poLogManager) << LogLevel_Info << "[!] Experiment starting\n";
}

void CApplication::stopExperiment()
{
	(*m_poLogManager) << LogLevel_Info << "[!] Experiment halting\n";
	this->exit();
}


#endif
