# ---------------------------------
# Finds OpenGL
# Adds library to target
# Adds include path
# ---------------------------------

SET(foundIncludeDir 1)

#GL is natively installed on our Linux pc's and comes with the installation of visual studio. In that case however, one has to make sure the SDK is in the INCLUDE environment variable
IF(WIN32)
	FIND_PATH(PATH_GL GL\\gl.h)
ELSE(WIN32)
	FIND_PATH(PATH_GL GL/gl.h)
ENDIF(WIN32)
IF(PATH_GL)
	MESSAGE(STATUS "  Found OpenGL in ${PATH_GL}" )
	INCLUDE_DIRECTORIES(${PATH_GL})
ELSE(PATH_GL)
	SET(foundIncludeDir 0)
ENDIF(PATH_GL)

#GLFW (for initializing OpenGL context, lighter than SDL)
IF(WIN32)
	FIND_PATH(PATH_GLFW include/GLFW/glfw3.h PATHS ${OV_CUSTOM_DEPENDENCIES_PATH} ${OV_CUSTOM_DEPENDENCIES_PATH}/GLFW )
	IF(PATH_GLFW)
		MESSAGE(STATUS "  Found GLFW in ${PATH_GLFW}" )
		INCLUDE_DIRECTORIES(${PATH_GLFW}/include)
	ELSE(PATH_GLFW)
		MESSAGE(STATUS " GLFW not found" )
		SET(foundIncludeDir 0)
	ENDIF(PATH_GLFW)
ELSE(WIN32)
	INCLUDE("FindThirdPartyPkgConfig")
	pkg_check_modules(GLFW REQUIRED glfw3)
	IF(GLFW_FOUND)
		MESSAGE(STATUS "  Found glfw in ${PATH_GLFW}" )
		include_directories(${GLFW_INCLUDE_DIRS})
	ELSE(GLFW_FOUND)
		MESSAGE("GLFW not found")
		SET(foundIncludeDir 0)
	ENDIF(GLFW_FOUND)
ENDIF(WIN32)

#include windows library inpout32 for writing to parrequiredel port
IF(WIN32)
	FIND_PATH(INPOUT32 include/inpout32.h ${OV_CUSTOM_DEPENDENCIES_PATH}/inpout32/)
	IF(INPOUT32)
		MESSAGE(STATUS " Found inpout32 in ${INPOUT32}")
		INCLUDE_DIRECTORIES(${INPOUT32}/include)
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyInpout)
	ENDIF(INPOUT32)
ENDIF(WIN32)

FIND_PATH(PATH_PRESAGE presage.h PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/include/ ${OV_CUSTOM_DEPENDENCIES_PATH}/include/presage/ ${OV_CUSTOM_DEPENDENCIES_PATH}/presage/include)
IF(PATH_PRESAGE)
	MESSAGE(STATUS "  Found Presage in ${PATH_PRESAGE}")
	INCLUDE_DIRECTORIES(${PATH_PRESAGE})
	SET(name_libpresage presage)
	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyPresage)
ELSE(PATH_PRESAGE)
	MESSAGE(STATUS "  FAILED to find Presage")
ENDIF(PATH_PRESAGE)

IF(foundIncludeDir)
	SET(required_libs_found 1)#only the required libs, true
	IF(WIN32)
		FIND_LIBRARY(LIB_GL OpenGL32)
		FIND_LIBRARY(LIB_GLFW glfw3 PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/GLFW/lib)
		FIND_LIBRARY(LIB_presage libpresage-1 PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/presage/lib)
		IF(LIB_GLFW)
			MESSAGE(STATUS "    [  OK  ] lib ${LIB_GLFW}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GLFW})
		ELSE(LIB_GLFW)
			MESSAGE(STATUS "    [FAILED] lib GLFW")
			SET(required_libs_found 0)#false
		ENDIF(LIB_GLFW)
		IF(LIB_presage)
			MESSAGE(STATUS "    [  OK  ] lib  ${LIB_presage}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_presage})
		ELSE( LIB_presage)
			MESSAGE(STATUS "    [FAILED] lib presage")
		ENDIF(LIB_presage)

		IF(LIB_GL)
			MESSAGE(STATUS "    [  OK  ] lib ${LIB_GL}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GL})
		ELSE(LIB_GL)
			MESSAGE(STATUS "    [FAILED] lib GL")
			SET(required_libs_found 0)#false
		ENDIF(LIB_GL)

		#inpout32.lib
		FIND_LIBRARY(LIB_INPOUT32 NAMES "inpout32.lib" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/inpout32/lib/)
		IF(LIB_INPOUT32)
			MESSAGE(STATUS "    [  OK  ] lib ${LIB_INPOUT32}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_INPOUT32} )
		ELSE(LIB_INPOUT32)
			MESSAGE(STATUS "    [FAILED] lib inpout32")
		ENDIF(LIB_INPOUT32)	
	ELSE(WIN32)
		FIND_LIBRARY(LIB_GL GL)
		IF(LIB_GL)
			MESSAGE("GL OK")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GL})
		ELSE(LIB_GL)
			MESSAGE("GL NOT OK")
			SET(required_libs_found 0)#false
		ENDIF(LIB_GL)

		FIND_LIBRARY(LIB_GLU GLU)
		IF(LIB_GLU)
			MESSAGE("GLU OK")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GLU})
		ELSE(LIB_GUL)
			MESSAGE("GLU NOT OK")
			SET(required_libs_found 0)#false
		ENDIF(LIB_GLU)

		FOREACH(GLFW_LIB ${GLFW_LIBRARIES})
			SET(GLFW_LIB1 "GLFW_LIB1-NOTFOUND")
			FIND_LIBRARY(GLFW_LIB1 NAMES ${GLFW_LIB} PATHS ${GLFW_LIBRARY_DIRS} ${GLFW_LIBDIR} NO_DEFAULT_PATH)
			FIND_LIBRARY(GLFW_LIB1 NAMES ${GLFW_LIB} PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib/)
			FIND_LIBRARY(GLFW_LIB1 NAMES ${GLFW_LIB})
			IF(GLFW_LIB1)
				MESSAGE(STATUS "    [  OK  ] Third party lib ${GLFW_LIB1}")
				TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${GLFW_LIB1})
			ELSE(GLFW_LIB1)
				MESSAGE(STATUS "    [FAILED] Third party lib ${GLFW_LIB}")
				SET(required_libs_found 0)#false
			ENDIF(GLFW_LIB1)
		ENDFOREACH(GLFW_LIB)

		FIND_LIBRARY(LIB_PRESAGE ${name_libpresage} PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib/)
		IF(LIB_PRESAGE)
			MESSAGE("PRESAGE OK")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_PRESAGE})
		ELSE(LIB_PRESAGE)
			MESSAGE("PRESAGE NOT OK")
		ENDIF(LIB_PRESAGE) 

		#pthread and rt seemed to be necessary on linux when using boost interprocess communication
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} pthread rt)	
	ENDIF(WIN32)

	IF(required_libs_found)
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyModulesForCoAdaptStimulator -D_GNU_SOURCE=1 -D_REENTRANT)
	ENDIF(required_libs_found)
ENDIF(foundIncludeDir)


