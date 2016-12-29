#
# Win32 only
#
# This concoction mainly exists to locate the boost libraries on WIN32 in the case where our dependency/ folder has only a 
# truncated version of boost and we cannot use find_package(). The background story is that we wish to link explicitly 
# instead of using automatic linking and specifying a generic boost path to linker before each project definition.
#
# The downside of this is that it assumes the boost library names to have a particular format and only works for VC90/VC100
#
# Function: Links specified boost library defined by COMPONENT and BOOST_VERSION to PROJECT_NAME.
#


FUNCTION(OV_LINK_BOOST_LIB BOOST_COMPONENT BOOST_VERSION)

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_BOOST_${BOOST_COMPONENT})

IF(WIN32)
	
	SET(LIB_BOOST "unknown-platform-NOTFOUND")
	SET(LIB_BOOST_DEBUG "unknown-platform-NOTFOUND")	
	
	STRING(REGEX MATCH "vc100.*" MSVC_VER100 ${MSVC_SERVICE_PACK})
	IF(MSVC_VER100) 
		SET(LIB_BOOST "libboost_${BOOST_COMPONENT}-vc100-mt-${BOOST_VERSION}.lib")
		SET(LIB_BOOST_DEBUG "libboost_${BOOST_COMPONENT}-vc100-mt-gd-${BOOST_VERSION}.lib")			
	ENDIF(MSVC_VER100)
	
	STRING(REGEX MATCH "vc120.*" MSVC_VER120 ${MSVC_SERVICE_PACK})
	IF(MSVC_VER120) 
		SET(LIB_BOOST "libboost_${BOOST_COMPONENT}-vc120-mt-${BOOST_VERSION}.lib")
		SET(LIB_BOOST_DEBUG "libboost_${BOOST_COMPONENT}-vc120-mt-gd-${BOOST_VERSION}.lib")			
	ENDIF(MSVC_VER120)
	
	SET(LIB_BOOST_PATH "-NOTFOUND")	
	FIND_LIBRARY(LIB_BOOST_PATH NAMES ${LIB_BOOST} PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/boost/lib NO_DEFAULT_PATH)	
	SET(LIB_BOOST_DEBUG_PATH "-NOTFOUND")	
	FIND_LIBRARY(LIB_BOOST_DEBUG_PATH NAMES ${LIB_BOOST_DEBUG} PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/boost/lib NO_DEFAULT_PATH)
	
	IF(LIB_BOOST_PATH AND LIB_BOOST_DEBUG_PATH)
		OV_PRINT(OV_PRINTED "  Found Boost ${BOOST_COMPONENT} libraries ...")
	ELSE(LIB_BOOST_PATH AND LIB_BOOST_DEBUG_PATH)
		OV_PRINT(OV_PRINTED "  FAILED to find all Boost ${BOOST_COMPONENT} libraries ...")	
	ENDIF(LIB_BOOST_PATH AND LIB_BOOST_DEBUG_PATH)
	
	IF(LIB_BOOST_PATH)
		OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_BOOST_PATH}")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} optimized ${LIB_BOOST_PATH})	
	ELSE(LIB_BOOST_PATH)
		MESSAGE(STATUS "    FAILED to find boost lib ${LIB_BOOST}")
	ENDIF(LIB_BOOST_PATH)
		
	IF(LIB_BOOST_DEBUG_PATH)
		OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_BOOST_DEBUG_PATH}")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} debug ${LIB_BOOST_DEBUG_PATH})	
	ELSE(LIB_BOOST_DEBUG_PATH)
		MESSAGE(STATUS "    FAILED to find boost debug lib ${LIB_BOOST_DEBUG}")
	ENDIF(LIB_BOOST_DEBUG_PATH)	
ENDIF(WIN32)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_BOOST_${BOOST_COMPONENT} "Yes")

ENDFUNCTION(OV_LINK_BOOST_LIB)
