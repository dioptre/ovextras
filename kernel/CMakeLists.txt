PROJECT(openvibe-kernel)
OV_ADD_THIS_TO_PROJECT_LIST()

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.inl)

ADD_LIBRARY(${PROJECT_NAME} SHARED ${source_files})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	COMPILE_FLAGS "-DOV_Exports -DOV_Shared")

INCLUDE("FindOpenViBE")
INCLUDE("FindOpenViBECommon")
INCLUDE("FindOpenViBEModuleFS")
INCLUDE("FindOpenViBEModuleSystem")
INCLUDE("FindOpenViBEModuleXML")
INCLUDE("FindThirdPartyGTK")
INCLUDE("FindThirdPartyOgre3D")			# OGRE + OIS included, pulls boost thread
INCLUDE("FindThirdPartyBoost")			# Using Ogre headers on Win32 causes dependency to Boost thread library
INCLUDE("FindThirdPartyBoost_Thread")	# Note that this is a potential issue on Windows, as the dependencies/ogre/boost and dependencies/boost are not the same at the time of writing this.
INCLUDE("FindThirdPartyX11")			# Linux packaging ("lintian") complains if not linked to, NOP on Win32

# ---------------------------------
# Finds standard library dl
# Adds library to target
# Adds include path
# ---------------------------------
IF(UNIX)
	FIND_LIBRARY(LIB_STANDARD_MODULE_DL dl)
	IF(LIB_STANDARD_MODULE_DL)
		MESSAGE(STATUS "  Found dl...")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_STANDARD_MODULE_DL})
	ELSE(LIB_STANDARD_MODULE_PTHREAD)
		MESSAGE(STATUS "  FAILED to find dl...")
	ENDIF(LIB_STANDARD_MODULE_DL)
ENDIF(UNIX)

# ---------------------------------
# Finds standard library pthread
# Adds library to target
# Adds include path
# ---------------------------------
IF(UNIX)
	FIND_LIBRARY(LIB_STANDARD_MODULE_PTHREAD pthread)
	IF(LIB_STANDARD_MODULE_PTHREAD)
		MESSAGE(STATUS "  Found pthread...")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_STANDARD_MODULE_PTHREAD})
	ELSE(LIB_STANDARD_MODULE_PTHREAD)
		MESSAGE(STATUS "  FAILED to find pthread...")
	ENDIF(LIB_STANDARD_MODULE_PTHREAD)
ENDIF(UNIX)

# ---------------------------------
# Target macros
# Defines target operating system
# Defines target architecture
# Defines target compiler
# ---------------------------------
IF(WIN32)
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
	ADD_DEFINITIONS(-DTARGET_OS_Windows)
	ADD_DEFINITIONS(-DTARGET_ARCHITECTURE_i386)
	ADD_DEFINITIONS(-DTARGET_COMPILER_VisualStudio)
ENDIF(WIN32)
IF(UNIX)
	# ADD_DEFINITIONS(-fvisibility=hidden) # This flag should be present... man gcc
	ADD_DEFINITIONS(-g)
	ADD_DEFINITIONS(-DTARGET_OS_Linux)
	ADD_DEFINITIONS(-DTARGET_ARCHITECTURE_i386)
	ADD_DEFINITIONS(-DTARGET_COMPILER_GCC)
ENDIF(UNIX)

# -----------------------------
# Install files
# -----------------------------
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})

INSTALL(DIRECTORY share/ DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/openvibe/kernel   PATTERN ".svn" EXCLUDE PATTERN "*-base" EXCLUDE)

#INSTALL(DIRECTORY src/   DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h" PATTERN ".svn" EXCLUDE PATTERN "doc" EXCLUDE)

# -----------------------------
# Install and configure .conf
# -----------------------------

# This construct is to fetch the documentation specific to this version. 
# In addition, it tweaks the patch version to allow the "1.0.0+git" style version to 
# fetch the "1.0.0" (base) version documentation as we don't have git version docs on the web.
STRING(REGEX REPLACE "[^0-9]" "" PROJECT_VERSION_PATCH_PRUNED ${PROJECT_VERSION_PATCH})
SET(OV_PROJECT_BASE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH_PRUNED})

CONFIGURE_FILE(share/openvibe.conf-base "${CMAKE_CURRENT_BINARY_DIR}/openvibe.conf" @ONLY)
CONFIGURE_FILE(share/openvibe_test.conf-base "${CMAKE_CURRENT_BINARY_DIR}/openvibe_test.conf" @ONLY)

INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/openvibe.conf" DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/openvibe/kernel)
INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/openvibe_test.conf" DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/openvibe/kernel)

