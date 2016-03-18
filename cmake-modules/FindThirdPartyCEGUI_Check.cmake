# ---------------------------------
# Finds CEGUI & Ogre toolkit
#
# Should only be used to check the presence, include later
#
# ---------------------------------

IF(OV_DISABLE_OGRE)
	MESSAGE(STATUS "  SKIPPED Ogre3D/OIS (CEGUI), disabled, no 3D ...")
	RETURN()
ENDIF(OV_DISABLE_OGRE)

IF(WIN32)
	FIND_PATH(PATH_CEGUI cegui/include/CEGUI.h PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/cegui)
	IF(PATH_CEGUI)
		SET(CEGUI_FOUND TRUE)
	ENDIF(PATH_CEGUI)
	FIND_PATH(PATH_Ogre3D include/OGRE/Ogre.h PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/ogre $ENV{OGRE_HOME})
	IF(PATH_Ogre3D)
		SET(OgreCEGUIRenderer_FOUND TRUE)	
	ENDIF(PATH_Ogre3D)
ENDIF(WIN32)

IF(UNIX)
	INCLUDE("FindThirdPartyPkgConfig")
	pkg_check_modules(CEGUI CEGUI)
	pkg_check_modules(OgreCEGUIRenderer CEGUI-OGRE)
ENDIF(UNIX)

IF(CEGUI_FOUND AND OgreCEGUIRenderer_FOUND)
	MESSAGE(STATUS "  Found CEGUI/OgreCEGUIRenderer...")
ELSE(CEGUI_FOUND AND OgreCEGUIRenderer_FOUND)
	MESSAGE(STATUS "  FAILED to find CEGUI/OgreCEGUIRenderer...")
ENDIF(CEGUI_FOUND AND OgreCEGUIRenderer_FOUND)
