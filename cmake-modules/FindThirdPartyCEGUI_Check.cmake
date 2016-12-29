# ---------------------------------
# Finds CEGUI & Ogre toolkit
#
# Should only be used to check the presence, include later
#
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyCEGUI_Check)

IF(OV_DISABLE_OGRE)
	OV_PRINT(OV_PRINTED "  SKIPPED Ogre3D/OIS (CEGUI), disabled, no 3D ...")
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
	INCLUDE("FindPkgConfig")
	SET(CEGUI_FOUND "-NOTFOUND")
	pkg_check_modules(CEGUI QUIET CEGUI)
	IF(NOT CEGUI_FOUND)
		# we have this mess as the cegui filenames & paths are different on Fedora 21 at least,
		# and the include_dirs doesn't contain the CEGUI/ part ... I'd put that to the .h/.cpp IF the same path
		# convention was the case on all platforms... but it is not  
		pkg_check_modules(CEGUI QUIET CEGUI-0)
		SET(CEGUI_INCLUDE_DIRS "${CEGUI_INCLUDE_DIRS}/CEGUI")
	ENDIF(NOT CEGUI_FOUND)
	SET(OgreCEGUIRenderer_FOUND "-NOTFOUND")
	pkg_check_modules(OgreCEGUIRenderer QUIET CEGUI-OGRE)
	IF(NOT OgreCEGUIRenderer_FOUND)
		pkg_check_modules(OgreCEGUIRenderer QUIET CEGUI-0-OGRE)
	ENDIF(NOT OgreCEGUIRenderer_FOUND)
ENDIF(UNIX)

# IF(CEGUI_FOUND AND OgreCEGUIRenderer_FOUND)
# 	OV_PRINT(OV_PRINTED "  Found CEGUI/OgreCEGUIRenderer...")
# ELSE(CEGUI_FOUND AND OgreCEGUIRenderer_FOUND)
# 	OV_PRINT(OV_PRINTED "  FAILED to find CEGUI/OgreCEGUIRenderer...")
# ENDIF(CEGUI_FOUND AND OgreCEGUIRenderer_FOUND)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyCEGUI_Check "Yes")

