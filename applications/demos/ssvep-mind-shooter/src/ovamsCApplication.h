#ifndef __OpenViBEApplication_CApplication_H__
#define __OpenViBEApplication_CApplication_H__

#include "ovams_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <Ogre.h>
#include <vector>
#include <CEGUI.h>
#if (CEGUI_VERSION_MAJOR > 0) || (CEGUI_VERSION_MINOR >= 8)
#include <CEGUI/RendererModules/Ogre/Renderer.h>
#else
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>
#endif

#include "ovamsICommand.h"
#include "ovamsCBasicPainter.h"

#include <tcptagging/IStimulusSender.h>

namespace OpenViBESSVEPMindShooter
{
	class CApplication : public Ogre::FrameListener, public Ogre::WindowEventListener
	{
		public:
			CApplication(OpenViBE::CString scenarioDir);
			virtual ~CApplication();

			void addCommand(ICommand* pCommand);
			virtual bool setup(OpenViBE::Kernel::IKernelContext* poKernelContext);
			void go();

			virtual void startExperiment();
			virtual void stopExperiment();
			virtual void startFlickering() {}
			virtual void stopFlickering() {}

			virtual void debugAction1() {}
			virtual void debugAction2() {}
			virtual void debugAction3() {}
			virtual void debugAction4() {}

			virtual void setTarget(int) {}

			Ogre::RenderWindow* getWindow() { return m_poWindow; }
			Ogre::SceneManager* getSceneManager() { return m_poSceneManager; }
			Ogre::SceneNode*    getSceneNode() { return m_poSceneNode; }
			Ogre::Camera*       getCamera() { return m_poCamera; }
			Ogre::SceneNode*    getCameraNode() { return m_poCameraNode; }
			CBasicPainter*      getPainter() { return m_poPainter; }

			void exit()
			{
				m_bContinueRendering = false;
			}

			OpenViBE::Kernel::ILogManager& getLogManager()
			{
				return (*m_poLogManager);
			}

			OpenViBE::Kernel::IConfigurationManager* getConfigurationManager()
			{
				return &(m_poKernelContext->getConfigurationManager());
			}

			std::vector<OpenViBE::uint64>* getFrequencies()
			{
				return &m_oFrequencies;
			}

			void resizeViewport();

			OpenViBE::Kernel::ILogManager& logPrefix()
			{
				(*m_poLogManager) << OpenViBE::Kernel::LogLevel_Trace << "% [" << m_ui64CurrentTime << "] ";
				return (*m_poLogManager);
			}

			OpenViBE::CString m_sScenarioDir;

			TCPTagging::IStimulusSender* m_pStimulusSender;

		protected:
			OpenViBE::Kernel::IKernelContext* m_poKernelContext;
			OpenViBE::Kernel::ILogManager* m_poLogManager;

			OpenViBE::float64 m_f64ScreenRefreshRate;
			CBasicPainter* m_poPainter;

			bool m_bContinueRendering;
			OpenViBE::uint32 m_ui32CurrentFrame;
			OpenViBE::uint64 m_ui64CurrentTime;

			Ogre::Root* m_poRoot;
			Ogre::SceneManager* m_poSceneManager;
			Ogre::Camera* m_poCamera;
			Ogre::SceneNode* m_poCameraNode;
			Ogre::RenderWindow* m_poWindow;
			Ogre::Viewport* m_poViewport;
			Ogre::SceneNode* m_poSceneNode;

			CEGUI::OgreRenderer* m_roGUIRenderer;
			CEGUI::WindowManager* m_poGUIWindowManager;
			CEGUI::Window* m_poSheet;

			std::vector<OpenViBE::uint64> m_oFrequencies;

			std::vector<ICommand*> m_oCommands;

			virtual void processFrame(OpenViBE::uint32 ui32CurrentFrame);

			bool frameRenderingQueued(const Ogre::FrameEvent &evt);
			bool frameStarted(const Ogre::FrameEvent &evt);

			bool configure();
			void setupResources();
			
		private:
			void setOgreParameters(void);	// Set some render parameters

			void initCEGUI(const char *logFilename);

			Ogre::uint32 m_ui32WindowWidth;
			Ogre::uint32 m_ui32WindowHeight;


	};



}

#endif // __OpenViBEApplication_CApplication_H__
