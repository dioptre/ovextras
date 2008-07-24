// #if defined __MY_COMPILE_ALL

#include "ovkCSimulatedBox.h"
#include "ovkCPlayer.h"
#include "ovkCBoxAlgorithmContext.h"
#include "ovkCMessageClock.h"
#include "ovkCMessageEvent.h"
#include "ovkCMessageSignal.h"

#include "../visualisation/ovkCVisualisationManager.h"
#include "ovkCOgreVisualisation.h"
#include "ovkCOgreObject.h"
#include "ovkCOgreWindow.h"
#include "ovkCOgreScene.h"
#include "../ovkGtkOVCustom.h"

#if defined OVK_OS_Windows
 #include <gdk/gdkwin32.h>
#elif defined OVK_OS_Linux
 #include <gdk/gdkx.h>
#endif

using namespace std;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

#define boolean OpenViBE::boolean
#define _ScopeTester_
#define _MaxCrash_ 5

#define __OV_FUNC__ CString("unknown_function_name")
#define __OV_LINE__ uint32(__LINE__)
#define __OV_FILE__ CString(__FILE__)

// ________________________________________________________________________________________________________________
//

namespace
{
	template <typename T>
	T& _my_get_(list<T>& rList, uint32 ui32Index)
	{
		uint32 i;
		typename list<T>::iterator it=rList.begin();
		for(i=0; i<ui32Index; i++)
		{
			++it;
		}
		return *it;
	}

	template <typename T>
	const T& _my_get_(const list<T>& rList, uint32 ui32Index)
	{
		uint32 i;
		typename list<T>::const_iterator it=rList.begin();
		for(i=0; i<ui32Index; i++)
		{
			++it;
		}
		return *it;
	}
}

// ________________________________________________________________________________________________________________
//

#define _Bad_Time_ 0xffffffffffffffffll

CSimulatedBox::CSimulatedBox(const IKernelContext& rKernelContext, CScheduler& rScheduler)
	:TKernelObject<IBoxIO>(rKernelContext)
	,m_ui32CrashCount(0)
	,m_bReadyToProcess(false)
	,m_bActive(true)
	,m_pBoxAlgorithm(NULL)
	,m_pScenario(NULL)
	,m_pBox(NULL)
	,m_rScheduler(rScheduler)
	,m_ui64LastClockActivationDate(_Bad_Time_)
	,m_ui64ClockFrequency(0)
	,m_ui64ClockActivationStep(0)
	,m_pOgreVis(NULL)
	,m_oSceneIdentifier(OV_UndefinedIdentifier)
{
	log() << LogLevel_Debug << __OV_FUNC__ << " - " << __OV_FILE__ << ":" << __OV_LINE__ << "\n";

	m_pOgreVis = ((CVisualisationManager*)(&rKernelContext.getVisualisationManager()))->getOgreVisualisation();
}

CSimulatedBox::~CSimulatedBox(void)
{
	log() << LogLevel_Debug << __OV_FUNC__ << " - " << __OV_FILE__ << ":" << __OV_LINE__ << "\n";

	//delete OgreWidgets
	std::map<GtkWidget*, OpenViBE::CIdentifier>::iterator it;
	for(it = m_mOgreWindows.begin(); it != m_mOgreWindows.end(); it = m_mOgreWindows.begin())
	{
		//this will destroy widget then call handleDestroyEvent to destroy COgreWindow
		//WARNING : this invalidates iterator!
		gtk_widget_destroy(it->first);
	}

	//clear simulated objects map
	m_mSimulatedObjects.clear();

	//delete OgreScene
	m_pOgreVis->deleteScene(m_oSceneIdentifier);
}

boolean CSimulatedBox::handleDestroyEvent(GtkWidget* pOVCustomWidget)
{
	m_pOgreVis->deleteWindow(m_mOgreWindows[pOVCustomWidget]);
	m_mOgreWindows.erase(pOVCustomWidget);
	return true;
}

boolean CSimulatedBox::handleRealizeEvent(GtkWidget* pOVCustomWidget)
{
	//create render window embedded in this widget
	COgreWindow* l_pOgreWindow = m_pOgreVis->getOgreWindow(m_mOgreWindows[pOVCustomWidget]);
	if(l_pOgreWindow == NULL)
	{
		return false;
	}

	std::string l_sExternalHandle;
#if defined OVK_OS_Windows
	l_sExternalHandle=Ogre::StringConverter::toString((unsigned long)GDK_WINDOW_HWND(pOVCustomWidget->window));
#elif defined OVK_OS_Linux
	::GdkDisplay* l_pGdkDisplay=gdk_drawable_get_display(GDK_DRAWABLE(pOVCustomWidget->window));
	::GdkScreen* l_pGdkScreen=gdk_drawable_get_screen(GDK_DRAWABLE(pOVCustomWidget->window));
	::GdkVisual* l_pGdkVisual=gdk_drawable_get_visual(GDK_DRAWABLE(pOVCustomWidget->window));

	::Display* l_pXDisplay=GDK_DISPLAY_XDISPLAY(l_pGdkDisplay);
	::Screen* l_pXScreen=GDK_SCREEN_XSCREEN(l_pGdkScreen);
	::XID l_pXWindow=GDK_WINDOW_XWINDOW(pOVCustomWidget->window);
	::Visual* l_pXVisual=GDK_VISUAL_XVISUAL(l_pGdkVisual);
	int l_iScreenIndex=::XScreenNumberOfScreen(l_pXScreen);

	::XVisualInfo l_oXVisualInfo;
	::memset(&l_oXVisualInfo, 0, sizeof(::XVisualInfo));
	l_oXVisualInfo.visual=l_pXVisual;
	l_oXVisualInfo.visualid=::XVisualIDFromVisual(l_pXVisual);
	l_oXVisualInfo.screen=l_iScreenIndex;
	l_oXVisualInfo.depth=24;

	l_sExternalHandle=
		Ogre::StringConverter::toString(reinterpret_cast<unsigned long>(l_pXDisplay))+":"+
		Ogre::StringConverter::toString(static_cast<unsigned int>(l_iScreenIndex))+":"+
		Ogre::StringConverter::toString(static_cast<unsigned long>(l_pXWindow))+":"+
		Ogre::StringConverter::toString(reinterpret_cast<unsigned long>(&l_oXVisualInfo));
#else
		#error failed compilation
#endif

	try
	{
		l_pOgreWindow->createRenderWindow(l_sExternalHandle, pOVCustomWidget->allocation.width, pOVCustomWidget->allocation.height);
		getLogManager() << LogLevel_Trace << "Created 3D widget\n";
	}
	catch(Ogre::Exception& e)
	{
		getLogManager() << LogLevel_Error << "Could not create render window : " << e.what() << "\n";
		return false;
	}

	//handle realization
	l_pOgreWindow->handleRealizeEvent();
	return true;
}

boolean CSimulatedBox::handleUnrealizeEvent(GtkWidget* pOVCustomWidget)
{
	COgreWindow* l_pOgreWindow = m_pOgreVis->getOgreWindow(m_mOgreWindows[pOVCustomWidget]);
	if(l_pOgreWindow == NULL)
	{
		return false;
	}
	l_pOgreWindow->handleUnrealizeEvent();
	return true;
}

boolean CSimulatedBox::handleSizeAllocateEvent(GtkWidget* pOVCustomWidget, unsigned int uiWidth, unsigned int uiHeight)
{
	COgreWindow* l_pOgreWindow = m_pOgreVis->getOgreWindow(m_mOgreWindows[pOVCustomWidget]);
	if(l_pOgreWindow == NULL)
	{
		return false;
	}
	l_pOgreWindow->handleSizeAllocateEvent(uiWidth, uiHeight);
	return true;
}

boolean CSimulatedBox::handleExposeEvent(GtkWidget* pOVCustomWidget)
{
	COgreWindow* l_pOgreWindow = m_pOgreVis->getOgreWindow(m_mOgreWindows[pOVCustomWidget]);
	if(l_pOgreWindow == NULL)
	{
		return false;
	}
	l_pOgreWindow->handleExposeEvent();
	return true;
}

boolean CSimulatedBox::handleMotionEvent(GtkWidget* pOVCustomWidget, int i32X, int i32Y)
{
	COgreWindow* l_pOgreWindow = m_pOgreVis->getOgreWindow(m_mOgreWindows[pOVCustomWidget]);
	if(l_pOgreWindow == NULL)
	{
		return false;
	}
	l_pOgreWindow->handleMotionEvent(i32X, i32Y);
	return true;
}

boolean CSimulatedBox::handleButtonPressEvent(GtkWidget* pOVCustomWidget, unsigned int uiButton, int i32X, int i32Y)
{
	COgreWindow* l_pOgreWindow = m_pOgreVis->getOgreWindow(m_mOgreWindows[pOVCustomWidget]);
	if(l_pOgreWindow == NULL)
	{
		return false;
	}
	l_pOgreWindow->handleButtonPressEvent(uiButton, i32X, i32Y);
	return true;
}

boolean CSimulatedBox::handleButtonReleaseEvent(GtkWidget* pOVCustomWidget, unsigned int uiButton, int i32X, int i32Y)
{
	COgreWindow* l_pOgreWindow = m_pOgreVis->getOgreWindow(m_mOgreWindows[pOVCustomWidget]);
	if(l_pOgreWindow == NULL)
	{
		return false;
	}
	l_pOgreWindow->handleButtonReleaseEvent(uiButton, i32X, i32Y);
	return true;
}

CIdentifier CSimulatedBox::create3DWidget(::GtkWidget*& p3DWidget)
{
	//don't attempt to create 3D widget if Ogre wasn't initialized properly.
	if(m_pOgreVis->ogreInitialized() == false || m_pOgreVis->resourcesInitialized() == false)
	{
		log () << LogLevel_Error << "Plugin " << m_pBox->getName() << " was disabled because the required 3D context couldn't be created!\n";
		m_bActive=false;
		return OV_UndefinedIdentifier;
	}

	//create Ogre widget
	GtkWidget* l_pOVCustomWidget = gtk_ov_custom_new(this);
	p3DWidget = GTK_WIDGET(l_pOVCustomWidget);

	//create a window and generate an identifier for this widget
	CIdentifier l_oWindowIdentifier = createOgreWindow();

	//associate identifier to widget in a map
	m_mOgreWindows[l_pOVCustomWidget] = l_oWindowIdentifier;

	return l_oWindowIdentifier;
}

boolean CSimulatedBox::setBackgroundColor(const CIdentifier& rWindowIdentifier, float32 f32ColorRed, float32 f32ColorGreen, float32 f32ColorBlue)
{
	COgreWindow* l_pOgreWindow = m_pOgreVis->getOgreWindow(rWindowIdentifier);
	if(l_pOgreWindow != NULL)
	{
		l_pOgreWindow->setBackgroundColor(f32ColorRed, f32ColorGreen, f32ColorBlue);
	}
	return true;
}

boolean CSimulatedBox::setCameraToEncompassObjects(const CIdentifier& rWindowIdentifier)
{
	COgreWindow* l_pOgreWindow = m_pOgreVis->getOgreWindow(rWindowIdentifier);
	if(l_pOgreWindow != NULL)
	{
		l_pOgreWindow->setCameraToEncompassObjects();
	}
	return true;
}

CIdentifier CSimulatedBox::createObject(const CString& rObjectFileName)
{
	if(rObjectFileName == CString(""))
	{
		return OV_UndefinedIdentifier;
	}

	//generate a name from an identifier for this object
	CIdentifier l_oIdentifier = getUnusedIdentifier();

	string l_oSceneFileName(rObjectFileName);
	l_oSceneFileName += ".scene";

	if(m_pOgreVis->getOgreScene(m_oSceneIdentifier)->createObject(l_oIdentifier, l_oIdentifier.toString().toASCIIString(), l_oSceneFileName) == NULL)
	{
		return OV_UndefinedIdentifier;
	}

	m_mSimulatedObjects[l_oIdentifier] = l_oIdentifier.toString();

	return l_oIdentifier;
}

CIdentifier CSimulatedBox::createObject(const EStandard3DObject eStandard3DObject)
{
	//TODO : read mapping of standard 3D objects to .scene file names from a config file
	if(eStandard3DObject == Standard3DObject_Sphere)
	{
		return createObject("sphere_ov");
	}
	else if(eStandard3DObject == Standard3DObject_Cone)
	{
		return createObject("cone_ov");
	}
	else
	{
		return OV_UndefinedIdentifier;
	}
}

boolean CSimulatedBox::removeObject(const CIdentifier& rObjectIdentifier)
{
	boolean res = m_pOgreVis->getOgreScene(m_oSceneIdentifier)->removeObject(rObjectIdentifier);
	m_mSimulatedObjects.erase(rObjectIdentifier);
	return res;
}

boolean CSimulatedBox::setObjectScale(const CIdentifier& rIdentifier, float32 f32ScaleX, float32 f32ScaleY, float32 f32ScaleZ)
{
	COgreObject* l_pOgreObject = m_pOgreVis->getOgreScene(m_oSceneIdentifier)->getOgreObject(rIdentifier);
	if(l_pOgreObject != NULL)
	{
		return l_pOgreObject->setScale(f32ScaleX, f32ScaleY, f32ScaleZ);
	}
	else
	{
		return false;
	}
}

boolean CSimulatedBox::setObjectScale(const CIdentifier& rIdentifier, float32 f32Scale)
{
	COgreObject* l_pOgreObject = m_pOgreVis->getOgreScene(m_oSceneIdentifier)->getOgreObject(rIdentifier);
	if(l_pOgreObject != NULL)
	{
		return l_pOgreObject->setScale(f32Scale, f32Scale, f32Scale);
	}
	else
	{
		return false;
	}
}

boolean CSimulatedBox::setObjectPosition(const CIdentifier& rIdentifier, float32 f32PositionX, float32 f32PositionY, float32 f32PositionZ)
{
	COgreObject* l_pOgreObject = m_pOgreVis->getOgreScene(m_oSceneIdentifier)->getOgreObject(rIdentifier);
	if(l_pOgreObject != NULL)
	{
		return l_pOgreObject->setPosition(f32PositionX, f32PositionY, f32PositionZ);
	}
	else
	{
		return false;
	}
}

boolean CSimulatedBox::setObjectOrientation(const CIdentifier& rIdentifier, float32 f32OrientationX, float32 f32OrientationY,
	float32 f32OrientationZ, float32 f32OrientationW)
{
	COgreObject* l_pOgreObject = m_pOgreVis->getOgreScene(m_oSceneIdentifier)->getOgreObject(rIdentifier);
	if(l_pOgreObject != NULL)
	{
		return l_pOgreObject->setRotation(f32OrientationX, f32OrientationY, f32OrientationZ, f32OrientationW);
	}
	else
	{
		return false;
	}
}

boolean CSimulatedBox::setObjectColor(const CIdentifier& rIdentifier, float32 f32ColorRed, float32 f32ColorGreen, float32 f32ColorBlue)
{
	COgreObject* l_pOgreObject = m_pOgreVis->getOgreScene(m_oSceneIdentifier)->getOgreObject(rIdentifier);
	if(l_pOgreObject != NULL)
	{
		return l_pOgreObject->setDiffuseColor(Ogre::ColourValue(f32ColorRed, f32ColorGreen, f32ColorBlue, 1));
	}
	else
	{
		return false;
	}
}

boolean CSimulatedBox::setObjectTransparency(const CIdentifier& rIdentifier, float32 f32Transparency)
{
	COgreObject* l_pOgreObject = m_pOgreVis->getOgreScene(m_oSceneIdentifier)->getOgreObject(rIdentifier);
	if(l_pOgreObject != NULL)
	{
		return l_pOgreObject->setTransparency(f32Transparency);
	}
	else
	{
		return false;
	}
	return true;
}

boolean CSimulatedBox::setObjectVertexColorArray(const CIdentifier& rIdentifier, const OpenViBE::uint32 ui32VertexColorCount, const float32* pVertexColorArray)
{
	COgreObject* l_pOgreObject = m_pOgreVis->getOgreScene(m_oSceneIdentifier)->getOgreObject(rIdentifier);
	if(l_pOgreObject == NULL)
	{
		return false;
	}

	if(sizeof(Ogre::Real) == sizeof(OpenViBE::float32))
	{
		return l_pOgreObject->setVertexColorArray((Ogre::uint32)ui32VertexColorCount, (Ogre::Real*)pVertexColorArray);
	}
	else
	{
		Ogre::Real* l_pOgreVertexColorArray(new Ogre::Real[ui32VertexColorCount]);
		for(uint32 i=0; i<ui32VertexColorCount; i++)
		{
			l_pOgreVertexColorArray[4*i] = pVertexColorArray[4*i];
			l_pOgreVertexColorArray[4*i+1] = pVertexColorArray[4*i+1];
			l_pOgreVertexColorArray[4*i+2] = pVertexColorArray[4*i+2];
			l_pOgreVertexColorArray[4*i+3] = pVertexColorArray[4*i+3];
		}
		boolean l_bRes = l_pOgreObject->setVertexColorArray((Ogre::uint32)ui32VertexColorCount, l_pOgreVertexColorArray);
		delete l_pOgreVertexColorArray;
		return l_bRes;
	}
}

boolean CSimulatedBox::getObjectVertexCount(const CIdentifier& rIdentifier, OpenViBE::uint32& ui32VertexCount) const
{
	COgreObject* l_pOgreObject = m_pOgreVis->getOgreScene(m_oSceneIdentifier)->getOgreObject(rIdentifier);
	if(l_pOgreObject == NULL)
	{
		return false;
	}
	return l_pOgreObject->getVertexCount((Ogre::uint32&)ui32VertexCount);
}

boolean CSimulatedBox::getObjectVertexPositionArray( const CIdentifier& rIdentifier, const OpenViBE::uint32 ui32VertexColorCount, float32* pVertexPositionArray) const
{
	COgreObject* l_pOgreObject = m_pOgreVis->getOgreScene(m_oSceneIdentifier)->getOgreObject(rIdentifier);
	if(l_pOgreObject == NULL)
	{
		return false;
	}
	if(sizeof(Ogre::Real) == sizeof(OpenViBE::float32))
	{
		return l_pOgreObject->getVertexPositionArray((Ogre::uint32)ui32VertexColorCount, (Ogre::Real*)pVertexPositionArray);
	}
	else
	{
		Ogre::Real* l_pOgreVertexPositionArray(new Ogre::Real[ui32VertexColorCount]);
		boolean l_bRes = l_pOgreObject->getVertexPositionArray((Ogre::uint32)ui32VertexColorCount, pVertexPositionArray);
		if(l_bRes == true)
		{
			for(uint32 i=0; i<ui32VertexColorCount; i++)
			{
				pVertexPositionArray[4*i] = l_pOgreVertexPositionArray[4*i];
				pVertexPositionArray[4*i+1] = l_pOgreVertexPositionArray[4*i+1];
				pVertexPositionArray[4*i+2] = l_pOgreVertexPositionArray[4*i+2];
				pVertexPositionArray[4*i+3] = l_pOgreVertexPositionArray[4*i+3];
			}
		}
		delete l_pOgreVertexPositionArray;
		return l_bRes;
	}
}

CIdentifier CSimulatedBox::createOgreWindow()
{
	//create unique window name
	char l_sBuffer[1024];
	sprintf(l_sBuffer, "%s_Window%d", getName().toASCIIString(), m_mOgreWindows.size());

	//FIXME
	//create unique window identifier
	CIdentifier l_oWindowIdentifier = (((uint64)rand())<<32)+((uint64)rand());//m_mOgreWindows.size();

	//ensure a scene was created
	if(m_oSceneIdentifier == OV_UndefinedIdentifier)
	{
		m_oSceneIdentifier = getUnusedIdentifier();
		m_pOgreVis->createScene(m_oSceneIdentifier);
	}

	//create window
	m_pOgreVis->createWindow(l_oWindowIdentifier, string(l_sBuffer), m_oSceneIdentifier);

	return l_oWindowIdentifier;
}

CIdentifier CSimulatedBox::getUnusedIdentifier(void) const
{
	uint64 l_ui64Identifier=(((uint64)rand())<<32)+((uint64)rand());
	CIdentifier l_oResult;
	std::map<CIdentifier, CString>::const_iterator i;
	do
	{
		l_ui64Identifier++;
		l_oResult=CIdentifier(l_ui64Identifier);
		i=m_mSimulatedObjects.find(l_oResult);
	}
	while(i!=m_mSimulatedObjects.end() || l_oResult==OV_UndefinedIdentifier);
	//TODO : browse window and scene maps as well to guarantee ID unicity!
	return l_oResult;
}

boolean CSimulatedBox::setScenarioIdentifier(const CIdentifier& rScenarioIdentifier)
{
	// FIXME test if rScenario is a scenario identifier
	m_pScenario=&getScenarioManager().getScenario(rScenarioIdentifier);
	return m_pScenario!=NULL;
}

boolean CSimulatedBox::setBoxIdentifier(const CIdentifier& rBoxIdentifier)
{
	if(!m_pScenario)
	{
		return false;
	}

	m_pBox=m_pScenario->getBoxDetails(rBoxIdentifier);
	return m_pBox!=NULL;
}

boolean CSimulatedBox::initialize(void)
{
	log() << LogLevel_Debug << __OV_FUNC__ << " - " << __OV_FILE__ << ":" << __OV_LINE__ << "\n";

	if(!m_bActive) return false;

	// FIXME test for already initialized boxes etc
	if(!m_pBox) return false;
	if(!m_pScenario) return false;

	m_vInput.resize(m_pBox->getInputCount());
	m_vOutput.resize(m_pBox->getOutputCount());
	m_vCurrentOutput.resize(m_pBox->getOutputCount());

	m_oBenchmarkChronoProcessClock.reset(1024);
	m_oBenchmarkChronoProcessInput.reset(1024);
	m_oBenchmarkChronoProcess.reset(1024);

	m_ui64LastClockActivationDate=_Bad_Time_;
	m_ui64ClockFrequency=0;
	m_ui64ClockActivationStep=0;

	m_pBoxAlgorithm=getPluginManager().createBoxAlgorithm(m_pBox->getAlgorithmClassIdentifier(), NULL);
	if(!m_pBoxAlgorithm)
	{
		getLogManager() << LogLevel_Error << "Could not create box algorithm with class id " << m_pBox->getAlgorithmClassIdentifier() << "... This box will be deactivated but the whole scenario behavior will probably suffer !\n";
		m_bActive=false;
		return false;
	}

	{
		CBoxAlgorithmContext l_oBoxAlgorithmContext(getKernelContext(), this, m_pBox);
		{
#if defined _ScopeTester_
			Tools::CScopeTester l_oScopeTester(getKernelContext(), m_pBox->getName() + CString(" (IBoxAlgorithm::initialize)"));
#endif
			try
			{
				if(!m_pBoxAlgorithm->initialize(l_oBoxAlgorithmContext))
				{
					getLogManager() << LogLevel_ImportantWarning << "Box algorithm <" << m_pBox->getName() << "> has been deactivated because initialization phase returned bad status\n";
					m_bActive=false;
				}
			}
			catch (...)
			{
				this->handleCrash("initialization callback");
			}
		}
	}

	return true ;
}

boolean CSimulatedBox::uninitialize(void)
{
	log() << LogLevel_Debug << __OV_FUNC__ << " - " << __OV_FILE__ << ":" << __OV_LINE__ << "\n";

	if(!m_bActive) return false;

	{
		CBoxAlgorithmContext l_oBoxAlgorithmContext(getKernelContext(), this, m_pBox);
		{
#if defined _ScopeTester_
			Tools::CScopeTester l_oScopeTester(getKernelContext(), m_pBox->getName() + CString(" (IBoxAlgorithm::uninitialize)"));
#endif
			{
				try
				{
					if(!m_pBoxAlgorithm->uninitialize(l_oBoxAlgorithmContext))
					{
						getLogManager() << LogLevel_ImportantWarning << "Box algorithm <" << m_pBox->getName() << "> has been deactivated because uninitialization phase returned bad status\n";
						m_bActive=false;
					}
				}
				catch (...)
				{
					this->handleCrash("uninitialization callback");
				}
			}
		}
	}

	getPluginManager().releasePluginObject(m_pBoxAlgorithm);
	m_pBoxAlgorithm=NULL;

	return true ;
}

boolean CSimulatedBox::processClock(void)
{
	log() << LogLevel_Debug << __OV_FUNC__ << " - " << __OV_FILE__ << ":" << __OV_LINE__ << "\n";

	if(!m_bActive) return false;

	{
		CBoxAlgorithmContext l_oBoxAlgorithmContext(getKernelContext(), this, m_pBox);
		{
#if defined _ScopeTester_
			Tools::CScopeTester l_oScopeTester(getKernelContext(), m_pBox->getName() + CString(" (IBoxAlgorithm::getClockFrequency)"));
#endif
			try
			{
				uint64 l_ui64NewClockFrequency=m_pBoxAlgorithm->getClockFrequency(l_oBoxAlgorithmContext);
				if(l_ui64NewClockFrequency==0)
				{
					m_ui64ClockActivationStep=_Bad_Time_;
					m_ui64LastClockActivationDate=_Bad_Time_;
				}
				else
				{
					// note: 1LL should be left shifted 64 bits but this
					//       would result in an integer over shift (the one
					//       would exit). Thus the left shift of 63 bits
					//       and the left shift of 1 bit after the division
					m_ui64ClockActivationStep=((1LL<<63)/l_ui64NewClockFrequency)<<1;
				}
				m_ui64ClockFrequency=l_ui64NewClockFrequency;
			}
			catch (...)
			{
				this->handleCrash("clock frequency request callback");
			}
		}
	}

	if((m_ui64ClockFrequency!=0) && (m_ui64LastClockActivationDate==_Bad_Time_ || m_rScheduler.getCurrentTime()-m_ui64LastClockActivationDate>=m_ui64ClockActivationStep))
	{
		CBoxAlgorithmContext l_oBoxAlgorithmContext(getKernelContext(), this, m_pBox);
		{
#if defined _ScopeTester_
			Tools::CScopeTester l_oScopeTester(getKernelContext(), m_pBox->getName() + CString(" (IBoxAlgorithm::processClock)"));
#endif
			try
			{
				m_oBenchmarkChronoProcessClock.stepIn();

				if(m_ui64LastClockActivationDate==_Bad_Time_)
				{
					m_ui64LastClockActivationDate=m_rScheduler.getCurrentTime();
				}
				else
				{
					m_ui64LastClockActivationDate=m_ui64LastClockActivationDate+m_ui64ClockActivationStep;
				}

				Kernel::CMessageClock l_oClockMessage(this->getKernelContext());
				l_oClockMessage.setTime(m_ui64LastClockActivationDate);
				m_pBoxAlgorithm->processClock(l_oBoxAlgorithmContext, l_oClockMessage);
				m_oBenchmarkChronoProcessClock.stepOut();
			}
			catch (...)
			{
				this->handleCrash("clock processing callback");
			}
			m_bReadyToProcess|=l_oBoxAlgorithmContext.isAlgorithmReadyToProcess();
		}
	}

	return true;
}

boolean CSimulatedBox::processInput(const uint32 ui32InputIndex, const CChunk& rChunk)
{
	log() << LogLevel_Debug << __OV_FUNC__ << " - " << __OV_FILE__ << ":" << __OV_LINE__ << "\n";

	if(!m_bActive) return false;

	m_vInput[ui32InputIndex].push_back(rChunk);

	{
		CBoxAlgorithmContext l_oBoxAlgorithmContext(getKernelContext(), this, m_pBox);
		{
#if defined _ScopeTester_
			Tools::CScopeTester l_oScopeTester(getKernelContext(), m_pBox->getName() + CString(" (IBoxAlgorithm::processInput)"));
#endif
			try
			{
				m_oBenchmarkChronoProcessInput.stepIn();
				m_pBoxAlgorithm->processInput(l_oBoxAlgorithmContext, ui32InputIndex);
				m_oBenchmarkChronoProcessInput.stepOut();
			}
			catch (...)
			{
				this->handleCrash("input processing callback");
			}
		}
		m_bReadyToProcess|=l_oBoxAlgorithmContext.isAlgorithmReadyToProcess();
	}

	return true;
}

boolean CSimulatedBox::process(void)
{
	log() << LogLevel_Debug << __OV_FUNC__ << " - " << __OV_FILE__ << ":" << __OV_LINE__ << "\n";

	if(!m_bActive) return false;
	if(!m_bReadyToProcess) return true;

	{
		CBoxAlgorithmContext l_oBoxAlgorithmContext(getKernelContext(), this, m_pBox);
		{
#if defined _ScopeTester_
			Tools::CScopeTester l_oScopeTester(getKernelContext(), m_pBox->getName() + CString(" (IBoxAlgorithm::process)"));
#endif
			try
			{
				m_oBenchmarkChronoProcess.stepIn();
				if(!m_pBoxAlgorithm->process(l_oBoxAlgorithmContext))
				{
					getLogManager() << LogLevel_ImportantWarning << "Box algorithm <" << m_pBox->getName() << "> has been deactivated because process phase returned bad status\n";
					m_bActive=false;
				}
				m_oBenchmarkChronoProcess.stepOut();
			}
			catch (...)
			{
				this->handleCrash("processing callback");
			}
		}
	}

	// perform output sending
	CIdentifier l_oLinkIdentifier=m_pScenario->getNextLinkIdentifierFromBox(OV_UndefinedIdentifier, m_pBox->getIdentifier());
	while(l_oLinkIdentifier!=OV_UndefinedIdentifier)
	{
		const ILink* l_pLink=m_pScenario->getLinkDetails(l_oLinkIdentifier);
		if(l_pLink)
		{
			CIdentifier l_oTargetBoxIdentifier=l_pLink->getTargetBoxIdentifier();
			uint32 l_ui32TargetBoxInputIndex=l_pLink->getTargetBoxInputIndex();

			uint32 l_ui32SourceOutputIndex=l_pLink->getSourceBoxOutputIndex();
			list < CChunk >::iterator i=m_vOutput[l_ui32SourceOutputIndex].begin();
			while(i!=m_vOutput[l_ui32SourceOutputIndex].end())
			{
				m_rScheduler.sendInput(*i, l_oTargetBoxIdentifier, l_ui32TargetBoxInputIndex);
				++i;
			}
		}
		l_oLinkIdentifier=m_pScenario->getNextLinkIdentifierFromBox(l_oLinkIdentifier, m_pBox->getIdentifier());
	}

	// iterators for input and output chunks
	vector < list< CChunk > >::iterator i;
	list < CChunk >::iterator j;
	vector < CChunk >::iterator k;

	// perform input cleaning
	i=m_vInput.begin();
	while(i!=m_vInput.end())
	{
		j=i->begin();
		while(j!=i->end())
		{
			if(j->isDeprecated())
			{
				j=i->erase(j);
			}
			else
			{
				++j;
			}
		}
		++i;
	}

	// flushes sent output chunks
	i=m_vOutput.begin();
	while(i!=m_vOutput.end())
	{
		i->resize(0);
		++i;
	}

	// discards waiting output chunks
	k=m_vCurrentOutput.begin();
	while(k!=m_vCurrentOutput.end())
	{
		// checks buffer size
		if(k->getBuffer().getSize())
		{
			// the buffer has been (partially ?) filled but not sent
			CBoxAlgorithmContext l_oBoxAlgorithmContext(getKernelContext(), this, m_pBox);
			l_oBoxAlgorithmContext.getPlayerContext()->getLogManager() << LogLevel_Warning << "Output buffer filled but not marked as ready to send\n"; // $$$ should use log
			k->getBuffer().setSize(0, true);
		}

		++k;
	}

	m_bReadyToProcess=false;

	// FIXME : might not be the most relevant place where to refresh 3D windows!
	map<GtkWidget*, CIdentifier>::iterator it;

	for(it = m_mOgreWindows.begin(); it != m_mOgreWindows.end(); it++)
	{
		m_pOgreVis->getOgreWindow(it->second)->update();
	}

#if 1
/*-----------------------------------------------*/
/* TODO send this messages with better frequency */
	if(m_oBenchmarkChronoProcessClock.hasNewEstimation())
	{
		log() << LogLevel_Benchmark
			<< "<" << LogColor_PushStateBit << LogColor_ForegroundBlue << "Player" << LogColor_PopStateBit
			<< "::" << LogColor_PushStateBit << LogColor_ForegroundBlue << "process clock" << LogColor_PopStateBit
			<< "::" << m_pBox->getName() << "> "
			<< "Average computing time is " << ((m_oBenchmarkChronoProcessClock.getAverageStepInDuration()*1000000)>>32) << "us\n";
	}
	if(m_oBenchmarkChronoProcessInput.hasNewEstimation())
	{
		log() << LogLevel_Benchmark
			<< "<" << LogColor_PushStateBit << LogColor_ForegroundBlue << "Player" << LogColor_PopStateBit
			<< "::" << LogColor_PushStateBit << LogColor_ForegroundBlue << "process input" << LogColor_PopStateBit
			<< "::" << m_pBox->getName() << "> "
			<< "Average computing time is " << ((m_oBenchmarkChronoProcessInput.getAverageStepInDuration()*1000000)>>32) << "us\n";
	}
	if(m_oBenchmarkChronoProcess.hasNewEstimation())
	{
		log() << LogLevel_Benchmark
			<< "<" << LogColor_PushStateBit << LogColor_ForegroundBlue << "Player" << LogColor_PopStateBit
			<< "::" << LogColor_PushStateBit << LogColor_ForegroundBlue << "process      " << LogColor_PopStateBit
			<< "::" << m_pBox->getName() << "> "
			<< "Average computing time is " << ((m_oBenchmarkChronoProcess.getAverageStepInDuration()*1000000)>>32) << "us\n";
	}
/* TODO Thank you for reading :)                 */
/*-----------------------------------------------*/
#endif

	return true;
}

boolean CSimulatedBox::isReadyToProcess(void) const
{
	return m_bReadyToProcess;
}

// --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
// - --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- -
// --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---

uint64 CSimulatedBox::getCurrentTime(void) const
{
	return m_rScheduler.getCurrentTime();
}

CString CSimulatedBox::getName(void) const
{
	return m_pBox->getName();
}

const IScenario& CSimulatedBox::getScenario(void) const
{
	return *m_pScenario;
}

// ________________________________________________________________________________________________________________
//

uint32 CSimulatedBox::getInputChunkCount(
	const uint32 ui32InputIndex) const
{
	if(ui32InputIndex>=m_vInput.size())
	{
		return false;
	}
	return m_vInput[ui32InputIndex].size();
}

boolean CSimulatedBox::getInputChunk(
	const uint32 ui32InputIndex,
	const uint32 ui32ChunkIndex,
	uint64& rStartTime,
	uint64& rEndTime,
	uint64& rChunkSize,
	const uint8*& rpChunkBuffer) const
{
	if(ui32InputIndex>=m_vInput.size())
	{
		return false;
	}
	if(ui32ChunkIndex>=m_vInput[ui32InputIndex].size())
	{
		return false;
	}

	const CChunk& l_rChunk=_my_get_(m_vInput[ui32InputIndex], ui32ChunkIndex);
	rStartTime=l_rChunk.getStartTime();
	rEndTime=l_rChunk.getEndTime();
	rChunkSize=l_rChunk.getBuffer().getSize();
	rpChunkBuffer=l_rChunk.getBuffer().getDirectPointer();
	return true;
}

const IMemoryBuffer* CSimulatedBox::getInputChunk(
	const uint32 ui32InputIndex,
	const uint32 ui32ChunkIndex) const
{
	if(ui32InputIndex>=m_vInput.size())
	{
		return NULL;
	}
	if(ui32ChunkIndex>=m_vInput[ui32InputIndex].size())
	{
		return NULL;
	}
	return &_my_get_(m_vInput[ui32InputIndex], ui32ChunkIndex).getBuffer();
}

uint64 CSimulatedBox::getInputChunkStartTime(
	const uint32 ui32InputIndex,
	const uint32 ui32ChunkIndex) const
{
	if(ui32InputIndex>=m_vInput.size())
	{
		return false;
	}
	if(ui32ChunkIndex>=m_vInput[ui32InputIndex].size())
	{
		return false;
	}

	const CChunk& l_rChunk=_my_get_(m_vInput[ui32InputIndex], ui32ChunkIndex);
	return l_rChunk.getStartTime();
}

uint64 CSimulatedBox::getInputChunkEndTime(
	const uint32 ui32InputIndex,
	const uint32 ui32ChunkIndex) const
{
	if(ui32InputIndex>=m_vInput.size())
	{
		return false;
	}
	if(ui32ChunkIndex>=m_vInput[ui32InputIndex].size())
	{
		return false;
	}

	const CChunk& l_rChunk=_my_get_(m_vInput[ui32InputIndex], ui32ChunkIndex);
	return l_rChunk.getEndTime();
}

boolean CSimulatedBox::markInputAsDeprecated(
	const uint32 ui32InputIndex,
	const uint32 ui32ChunkIndex)
{
	if(ui32InputIndex>=m_vInput.size())
	{
		return false;
	}
	if(ui32ChunkIndex>=m_vInput[ui32InputIndex].size())
	{
		return false;
	}
	_my_get_(m_vInput[ui32InputIndex], ui32ChunkIndex).markAsDeprecated(true);
	return true;
}

// ________________________________________________________________________________________________________________
//

uint64 CSimulatedBox::getOutputChunkSize(
	const uint32 ui32OutputIndex) const
{
	if(ui32OutputIndex>=m_vCurrentOutput.size())
	{
		return 0;
	}
	return m_vCurrentOutput[ui32OutputIndex].getBuffer().getSize();
}

boolean CSimulatedBox::setOutputChunkSize(
	const uint32 ui32OutputIndex,
	const uint64 ui64Size,
	const boolean bDiscard)
{
	if(ui32OutputIndex>=m_vOutput.size())
	{
		return false;
	}
	return m_vCurrentOutput[ui32OutputIndex].getBuffer().setSize(ui64Size, bDiscard);
}

uint8* CSimulatedBox::getOutputChunkBuffer(
	const uint32 ui32OutputIndex)
{
	if(ui32OutputIndex>=m_vOutput.size())
	{
		return NULL;
	}
	return m_vCurrentOutput[ui32OutputIndex].getBuffer().getDirectPointer();
}

boolean CSimulatedBox::appendOutputChunkData(
	const uint32 ui32OutputIndex,
	const uint8* pBuffer,
	const uint64 ui64BufferSize)
{
	if(ui32OutputIndex>=m_vOutput.size())
	{
		return false;
	}
	return m_vCurrentOutput[ui32OutputIndex].getBuffer().appendOutputChunkData(pBuffer, ui64BufferSize);
}

IMemoryBuffer* CSimulatedBox::getOutputChunk(
	const uint32 ui32OutputIndex)
{
	if(ui32OutputIndex>=m_vOutput.size())
	{
		return NULL;
	}
	return &m_vCurrentOutput[ui32OutputIndex].getBuffer();
}

boolean CSimulatedBox::markOutputAsReadyToSend(
	const uint32 ui32OutputIndex,
	const uint64 ui64StartTime,
	const uint64 ui64EndTime)
{
	if(ui32OutputIndex>=m_vOutput.size())
	{
		return false;
	}

	// sets start and end time
	m_vCurrentOutput[ui32OutputIndex].setStartTime(ui64StartTime);
	m_vCurrentOutput[ui32OutputIndex].setEndTime(ui64EndTime);

	// copies chunk
	m_vOutput[ui32OutputIndex].push_back(m_vCurrentOutput[ui32OutputIndex]);

	// resets chunk size
	m_vCurrentOutput[ui32OutputIndex].getBuffer().setSize(0, true);

	return true;
}

// ________________________________________________________________________________________________________________
//

void CSimulatedBox::handleCrash(const char* sHintName)
{
	m_ui32CrashCount++;

	log() << LogLevel_Error << "Plugin code caused crash " << m_ui32CrashCount << " time(s)\n";
	log() << LogLevel_Error << "  [name:" << m_pBox->getName() << "]\n";
	log() << LogLevel_Error << "  [identifier:" << m_pBox->getIdentifier() << "]\n";
	log() << LogLevel_Error << "  [algorithm class identifier:" << m_pBox->getAlgorithmClassIdentifier() << "]\n";
	log() << LogLevel_Error << "  [place:" << sHintName << "]\n";

	if(m_ui32CrashCount>=_MaxCrash_)
	{
		log () << LogLevel_Fatal << "  This plugin has been disabled !\n";
		m_bActive=false;
	}
}

// #endif // __MY_COMPILE_ALL
