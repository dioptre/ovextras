PROJECT(openvibe-plugins-network-io)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.inl)
ADD_LIBRARY(${PROJECT_NAME} SHARED ${source_files})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")
	
INCLUDE("FindOpenViBE")
INCLUDE("FindOpenViBECommon")
INCLUDE("FindOpenViBEToolkit")
INCLUDE("FindThirdPartyBoost")
INCLUDE("FindThirdPartyBoost_System")
INCLUDE("FindThirdPartyLSL")

#so that boost won't need to link against DateTime when using the interprocess communication library
#shared memory writer box
ADD_DEFINITIONS(-DBOOST_DATE_TIME_NO_LIB)

# ---------------------------------
# Finds standard library winmm
# Adds library to target
# Adds include path
# ---------------------------------
IF(WIN32)
	FIND_LIBRARY(LIB_STANDARD_MODULE_WINMM winmm)
	IF(LIB_STANDARD_MODULE_WINMM)
		MESSAGE(STATUS "  Found winmm...")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_STANDARD_MODULE_WINMM})
	ELSE(LIB_STANDARD_MODULE_WINMM)
		MESSAGE(STATUS "  FAILED to find winmm...")
	ENDIF(LIB_STANDARD_MODULE_WINMM)
ENDIF(WIN32)

# ---------------------------------
# Target macros
# Defines target operating system
# Defines target architecture
# Defines target compiler
# ---------------------------------
IF(WIN32)
	ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501) # for boost::asio
ENDIF(WIN32)
SET_BUILD_PLATFORM()

# -----------------------------
# Install files
# -----------------------------
INSTALL(TARGETS ${PROJECT_NAME}
	
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})
