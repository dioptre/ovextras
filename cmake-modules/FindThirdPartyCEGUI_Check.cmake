# ---------------------------------
# Finds CEGUI toolkit
#
# Sets CEGUI_FOUND
# Sets CEGUI_LIBRARIES
# Sets CEGUI_LIBRARY_DIRS
# Sets CEGUI_LDFLAGS
# Sets CEGUI_LDFLAGS_OTHERS
# Sets CEGUI_INCLUDE_DIRS
# Sets CEGUI_CFLAGS
# Sets CEGUI_CFLAGS_OTHERS
# ---------------------------------

IF(OV_DISABLE_OGRE)
	MESSAGE(STATUS "  SKIPPED Ogre3D/OIS (CEGUI), disabled, no 3D ...")
	RETURN()
ENDIF(OV_DISABLE_OGRE)

IF(WIN32)
	FIND_PATH(PATH_CEGUI cegui/include/CEGUI.h PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/cegui)
	IF(PATH_CEGUI)
		SET(CEGUI_FOUND TRUE)
		SET(OgreCEGUIRenderer_FOUND TRUE)
		SET(CEGUI_INCLUDE_DIRS ${PATH_CEGUI}/cegui/include)
		SET(CEGUI_LIBRARIES_DEBUG CEGUIBase_d CEGUIOgreRenderer_d)
		SET(CEGUI_LIBRARIES_RELEASE CEGUIBase CEGUIOgreRenderer)
		SET(CEGUI_LIBRARY_DIRS ${PATH_CEGUI}/lib)
	ENDIF(PATH_CEGUI)
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
