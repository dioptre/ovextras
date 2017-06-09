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

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyCEGUI)

IF(OV_DISABLE_OGRE)
	OV_PRINT(OV_PRINTED "  SKIPPED Ogre3D/OIS (CEGUI), disabled, no 3D ...")
	RETURN()
ENDIF(OV_DISABLE_OGRE)

IF(WIN32)
	FIND_PATH(PATH_CEGUI cegui/include/CEGUI.h PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES cegui)
	IF(PATH_CEGUI)
		SET(CEGUI_FOUND TRUE)
		SET(OgreCEGUIRenderer_FOUND TRUE)
		SET(CEGUI_INCLUDE_DIRS ${PATH_CEGUI}/cegui/include)
		STRING(REGEX MATCH "vc120.*" MSVC_VER120 ${MSVC_SERVICE_PACK})
		IF(MSVC_VER120) 
			SET(CEGUI_LIBRARIES_DEBUG CEGUIBase-0_d CEGUIOgreRenderer-0_d)
			SET(CEGUI_LIBRARIES_RELEASE CEGUIBase-0 CEGUIOgreRenderer-0)
		ELSE(MSVC_VER120)
			SET(CEGUI_LIBRARIES_DEBUG CEGUIBase_d CEGUIOgreRenderer_d)
			SET(CEGUI_LIBRARIES_RELEASE CEGUIBase CEGUIOgreRenderer)		
		ENDIF(MSVC_VER120)
		SET(CEGUI_LIBRARY_DIRS ${PATH_CEGUI}/lib)
	ENDIF(PATH_CEGUI)
ENDIF(WIN32)

IF(UNIX)
	# Assumes FindThirdPartyCEGUI_Check.cmake has been run
ENDIF(UNIX)

IF(CEGUI_FOUND AND OgreCEGUIRenderer_FOUND)
	OV_PRINT(OV_PRINTED "  Found CEGUI/OgreCEGUIRenderer...")
	
	INCLUDE_DIRECTORIES(${CEGUI_INCLUDE_DIRS} ${OgreCEGUIRenderer_INCLUDE_DIRS})
	ADD_DEFINITIONS(${CEGUI_CFLAGS} ${OgreCEGUIRenderer_CFLAGS})
	ADD_DEFINITIONS(${CEGUI_CFLAGS_OTHERS} ${OgreCEGUIRenderer_CFLAGS_OTHERS})
	# LINK_DIRECTORIES(${CEGUI_LIBRARY_DIRS} ${OgreCEGUIRenderer_LIBRARY_DIRS})
IF(UNIX)

	FOREACH(CEGUI_LIB ${CEGUI_LIBRARIES} ${OgreCEGUIRenderer_LIBRARIES})
		SET(CEGUI_LIB1 "CEGUI_LIB1-NOTFOUND")
		FIND_LIBRARY(CEGUI_LIB1 NAMES ${CEGUI_LIB} PATHS ${CEGUI_LIBRARY_DIRS} ${CEGUI_LIBDIR} NO_DEFAULT_PATH)
		FIND_LIBRARY(CEGUI_LIB1 NAMES ${CEGUI_LIB})
		IF(CEGUI_LIB1)
			OV_PRINT(OV_PRINTED "    [  OK  ] Third party lib ${CEGUI_LIB1}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CEGUI_LIB1})
		ELSE(CEGUI_LIB1)
			OV_PRINT(OV_PRINTED "    [FAILED] Third party lib ${CEGUI_LIB}")
		ENDIF(CEGUI_LIB1)
	ENDFOREACH(CEGUI_LIB)
ENDIF(UNIX)
IF(WIN32)
	FOREACH(CEGUI_LIB ${CEGUI_LIBRARIES_DEBUG})
		SET(CEGUI_LIB1 "CEGUI_LIB1-NOTFOUND")
		FIND_LIBRARY(CEGUI_LIB1 NAMES ${CEGUI_LIB} PATHS ${CEGUI_LIBRARY_DIRS} ${CEGUI_LIBDIR} NO_DEFAULT_PATH)
		FIND_LIBRARY(CEGUI_LIB1 NAMES ${CEGUI_LIB})
		IF(CEGUI_LIB1)
			OV_PRINT(OV_PRINTED "    [  OK  ] Third party lib ${CEGUI_LIB1}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} debug ${CEGUI_LIB1})
		ELSE(CEGUI_LIB1)
			OV_PRINT(OV_PRINTED "    [FAILED] Third party lib ${CEGUI_LIB}")
		ENDIF(CEGUI_LIB1)
	ENDFOREACH(CEGUI_LIB)
	FOREACH(CEGUI_LIB ${CEGUI_LIBRARIES_RELEASE})
		SET(CEGUI_LIB1 "CEGUI_LIB1-NOTFOUND")
		FIND_LIBRARY(CEGUI_LIB1 NAMES ${CEGUI_LIB} PATHS ${CEGUI_LIBRARY_DIRS} ${CEGUI_LIBDIR} NO_DEFAULT_PATH)
		FIND_LIBRARY(CEGUI_LIB1 NAMES ${CEGUI_LIB})
		IF(CEGUI_LIB1)
			OV_PRINT(OV_PRINTED "    [  OK  ] Third party lib ${CEGUI_LIB1}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} optimized ${CEGUI_LIB1})
		ELSE(CEGUI_LIB1)
			OV_PRINT(OV_PRINTED "    [FAILED] Third party lib ${CEGUI_LIB}")
		ENDIF(CEGUI_LIB1)
	ENDFOREACH(CEGUI_LIB)	
ENDIF(WIN32)

ELSE(CEGUI_FOUND AND OgreCEGUIRenderer_FOUND)
	OV_PRINT(OV_PRINTED "  FAILED to find CEGUI/OgreCEGUIRenderer...")
ENDIF(CEGUI_FOUND AND OgreCEGUIRenderer_FOUND)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyCEGUI "Yes")

