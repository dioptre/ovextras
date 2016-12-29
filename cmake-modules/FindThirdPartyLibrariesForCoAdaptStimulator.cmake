# ---------------------------------
# Finds dependencies required by the external stimulator
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyLibrariesForCoAdaptStimulator)

SET(foundIncludeDir 1)	   # true, if all required headers have been found
SET(required_libs_found 1) # true, if all libs which have headers have been found
	
IF(MSVC90)
	OV_PRINT(OV_PRINTED "CoAdapt P300 requires at least VS 2010\n")
	RETURN()
ENDIF(MSVC90)

#GL is natively installed on our Linux pc's and comes with the installation of visual studio. In that case however, one has to make sure the SDK is in the INCLUDE environment variable
IF(WIN32)
	FIND_PATH(PATH_GL GL\\gl.h)
ELSE(WIN32)
	FIND_PATH(PATH_GL GL/gl.h)
ENDIF(WIN32)
IF(PATH_GL)
	OV_PRINT(OV_PRINTED "  Found OpenGL in ${PATH_GL}" )
	INCLUDE_DIRECTORIES(${PATH_GL})
ELSE(PATH_GL)
	OV_PRINT(OV_PRINTED "  FAILED to find OpenGL" )
	SET(foundIncludeDir 0)
ENDIF(PATH_GL)

#GLFW (for initializing OpenGL context, lighter than SDL)
IF(WIN32)
	FIND_PATH(PATH_GLFW include/GLFW/glfw3.h PATHS ${OV_CUSTOM_DEPENDENCIES_PATH} ${OV_CUSTOM_DEPENDENCIES_PATH}/GLFW )
	IF(PATH_GLFW)
		OV_PRINT(OV_PRINTED "  Found GLFW3 in ${PATH_GLFW}" )
		INCLUDE_DIRECTORIES(${PATH_GLFW}/include)
	ELSE(PATH_GLFW)
		OV_PRINT(OV_PRINTED "  FAILED to find GLFW3" )
		SET(foundIncludeDir 0)
	ENDIF(PATH_GLFW)
ELSE(WIN32)
	INCLUDE("FindThirdPartyPkgConfig")
	pkg_check_modules(GLFW glfw3)
	IF(GLFW_FOUND)
		OV_PRINT(OV_PRINTED "  Found GLFW3 in ${GLFW_INCLUDE_DIRS}" )
		INCLUDE_DIRECTORIES(${GLFW_INCLUDE_DIRS})
	ELSE(GLFW_FOUND)
		# Try ov custom dependencies...
		FIND_PATH(PATH_GLFW include/GLFW/glfw3.h PATHS ${OV_CUSTOM_DEPENDENCIES_PATH} ${OV_CUSTOM_DEPENDENCIES_PATH}/GLFW )	
		IF(PATH_GLFW)
			OV_PRINT(OV_PRINTED "  Found GLFW3 in ${PATH_GLFW}" )	
			INCLUDE_DIRECTORIES(${PATH_GLFW}/include)			
			SET(GLFW_LIBRARY_DIR ${PATH_GLFW}/lib)			
			SET(GLFW_LIBRARIES glfw)
		ELSE(PATH_GLFW)
			OV_PRINT(OV_PRINTED "  FAILED to find GLFW3")
			SET(foundIncludeDir 0)
		ENDIF(PATH_GLFW)
	ENDIF(GLFW_FOUND)
ENDIF(WIN32)

#include windows library inpout32 for writing to parallel port
IF(WIN32)
	FIND_PATH(PATH_INPOUT32 include/inpout32.h ${OV_CUSTOM_DEPENDENCIES_PATH}/inpout32/)
	IF(PATH_INPOUT32)
		OV_PRINT(OV_PRINTED "  Found inpout32 in ${PATH_INPOUT32}")
		INCLUDE_DIRECTORIES(${PATH_INPOUT32}/include)
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyInpout)
	ELSE(PATH_INPOUT32)
		OV_PRINT(OV_PRINTED "  FAILED to find inpout32 (optional), parallel port tagging support disabled")	
		# optional 
	ENDIF(PATH_INPOUT32)
ENDIF(WIN32)

FIND_PATH(PATH_PRESAGE presage.h PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/include/ ${OV_CUSTOM_DEPENDENCIES_PATH}/include/presage/ ${OV_CUSTOM_DEPENDENCIES_PATH}/presage/include)
IF(PATH_PRESAGE)
	OV_PRINT(OV_PRINTED "  Found Presage in ${PATH_PRESAGE}")
	INCLUDE_DIRECTORIES(${PATH_PRESAGE})
	SET(name_libpresage presage)
	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyPresage)
ELSE(PATH_PRESAGE)
	OV_PRINT(OV_PRINTED "  FAILED to find presage (optional), P300 word prediction disabled")
	# optional
ENDIF(PATH_PRESAGE)

IF(foundIncludeDir)
	OV_PRINT(OV_PRINTED  "  Found required headers for External P300 Stimulator...")
	IF(WIN32)
		FIND_LIBRARY(LIB_GLFW glfw3 PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/GLFW/lib)
		IF(LIB_GLFW)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_GLFW}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GLFW})
		ELSE(LIB_GLFW)
			OV_PRINT(OV_PRINTED "    [FAILED] lib GLFW")
			SET(required_libs_found 0)#false
		ENDIF(LIB_GLFW)
		
		IF(PATH_PRESAGE)
			FIND_LIBRARY(LIB_presage libpresage-1 PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/presage/lib)		
			IF(LIB_presage)
				OV_PRINT(OV_PRINTED "    [  OK  ] lib  ${LIB_presage}")
				TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_presage})
			ELSE( LIB_presage)
				OV_PRINT(OV_PRINTED "    [FAILED] lib presage")
				SET(required_libs_found 0) # since we found the header, we require the lib
			ENDIF(LIB_presage)
		ENDIF(PATH_PRESAGE)

		FIND_LIBRARY(LIB_GL OpenGL32)		
		IF(LIB_GL)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_GL}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GL})
		ELSE(LIB_GL)
			OV_PRINT(OV_PRINTED "    [FAILED] lib GL")
			SET(required_libs_found 0)#false
		ENDIF(LIB_GL)

		#inpout32.lib
		IF(PATH_INPOUT32)
			FIND_LIBRARY(LIB_INPOUT32 NAMES "inpout32.lib" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/inpout32/lib/)
			IF(LIB_INPOUT32)
				OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_INPOUT32}")
				TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_INPOUT32} )
			ELSE(LIB_INPOUT32)
				OV_PRINT(OV_PRINTED "    [FAILED] lib inpout32")
				SET(required_libs_found 0)  # since we found the header, we require the lib			
			ENDIF(LIB_INPOUT32)	
		ENDIF(PATH_INPOUT32)
	ELSE(WIN32)
		FIND_LIBRARY(LIB_GL GL)
		IF(LIB_GL)
			OV_PRINT(OV_PRINTED "    [  OK  ] GL ${LIB_GL}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GL})
		ELSE(LIB_GL)
			OV_PRINT(OV_PRINTED "    [FAILED] lib GL")
			SET(required_libs_found 0)#false
		ENDIF(LIB_GL)

		FIND_LIBRARY(LIB_GLU GLU)
		IF(LIB_GLU)
			OV_PRINT(OV_PRINTED "    [  OK  ] GLU ${LIB_GLU}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GLU})
		ELSE(LIB_GUL)
			OV_PRINT(OV_PRINTED "    [FAILED] lib GLU")
			SET(required_libs_found 0)#false
		ENDIF(LIB_GLU)

		FOREACH(GLFW_LIB ${GLFW_LIBRARIES})
			SET(GLFW_LIB1 "GLFW_LIB1-NOTFOUND")
			FIND_LIBRARY(GLFW_LIB1 NAMES ${GLFW_LIB} PATHS ${GLFW_LIBRARY_DIRS} ${GLFW_LIBDIR} NO_DEFAULT_PATH)
			FIND_LIBRARY(GLFW_LIB1 NAMES ${GLFW_LIB} PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib/)
			FIND_LIBRARY(GLFW_LIB1 NAMES ${GLFW_LIB})
			IF(GLFW_LIB1)
				OV_PRINT(OV_PRINTED "    [  OK  ] Third party lib ${GLFW_LIB1}")
				TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${GLFW_LIB1})
			ELSE(GLFW_LIB1)
				OV_PRINT(OV_PRINTED "    [FAILED] Third party lib ${GLFW_LIB}")
				SET(required_libs_found 0)#false
			ENDIF(GLFW_LIB1)
		ENDFOREACH(GLFW_LIB)

		IF(PATH_PRESAGE)
			FIND_LIBRARY(LIB_PRESAGE ${name_libpresage} PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib/)
			IF(LIB_PRESAGE)
				OV_PRINT(OV_PRINTED "    [  OK  ] Presage ${LIB_PRESAGE}")
				TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_PRESAGE})
			ELSE(LIB_PRESAGE)
				OV_PRINT(OV_PRINTED "    [FAILED] Presage")
				SET(required_libs_found 0) # since we found the header, we require the lib
			ENDIF(LIB_PRESAGE) 
		ENDIF(PATH_PRESAGE)

		#pthread and rt seemed to be necessary on linux when using boost interprocess communication
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} pthread rt)	
	ENDIF(WIN32)

	IF(required_libs_found)
		OV_PRINT(OV_PRINTED  "  Ok, found all the required headers and libs for the External P300 Stimulator...")	
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyModulesForCoAdaptStimulator -D_GNU_SOURCE=1 -D_REENTRANT)
	ENDIF(required_libs_found)
ENDIF(foundIncludeDir)

IF(NOT (foundIncludeDir AND required_libs_found) )
	OV_PRINT(OV_PRINTED  "  Did not find all required headers and/or libs for the External P300 Stimulator...")
ENDIF(NOT (foundIncludeDir AND required_libs_found) )	



SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyLibrariesForCoAdaptStimulator "Yes")

