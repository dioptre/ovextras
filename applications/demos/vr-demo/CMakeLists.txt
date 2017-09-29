PROJECT(openvibe-vr-demo)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

INCLUDE("FindThirdPartyVRPN_Check")
IF(NOT PATH_VRPN)
	MESSAGE(STATUS "    --> Not building ${PROJECT_NAME}")
	RETURN()
ENDIF(NOT PATH_VRPN)

INCLUDE("FindThirdPartyCEGUI_Check")
IF(NOT CEGUI_FOUND OR NOT OgreCEGUIRenderer_FOUND)
        MESSAGE(STATUS "    --> Not building ${PROJECT_NAME}")
        RETURN()
ENDIF(NOT CEGUI_FOUND OR NOT OgreCEGUIRenderer_FOUND)

FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.inl)
ADD_EXECUTABLE(${PROJECT_NAME} ${source_files})

INCLUDE("FindOpenViBE")
INCLUDE("FindOpenViBECommon")
INCLUDE("FindOpenViBEModuleSystem")
INCLUDE("FindOpenViBEModuleFS")
INCLUDE("FindThirdPartyCEGUI") # CEGUI + CEGUIOgreRender included
INCLUDE("FindThirdPartyVRPN")
INCLUDE("FindThirdPartyOgre3D") 		# OGRE + OIS included
INCLUDE("FindThirdPartyBoost")			# Using Ogre headers on Win32 causes dependency to Boost thread library
INCLUDE("FindThirdPartyBoost_Thread")	# Note that this is a potential issue on Windows, as the dependencies/ogre/boost and dependencies/boost are not the same at the time of writing this.
INCLUDE("FindThirdPartyBoost_System")	# Note that this is a potential issue on Windows, as the dependencies/ogre/boost and dependencies/boost are not the same at the time of writing this.

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
SET_BUILD_PLATFORM()

# ----------------------
# Generate launch script
# ----------------------
IF(WIN32)
        OV_INSTALL_LAUNCH_SCRIPT(SCRIPT_PREFIX "openvibe-vr-demo-handball" EXECUTABLE_NAME ${PROJECT_NAME} PARAMETERS "handball")
	OV_INSTALL_LAUNCH_SCRIPT(SCRIPT_PREFIX "openvibe-vr-demo-spaceship" EXECUTABLE_NAME ${PROJECT_NAME} PARAMETERS "spaceship")
ENDIF(WIN32)
IF(UNIX)
        OV_INSTALL_LAUNCH_SCRIPT(SCRIPT_PREFIX "openvibe-vr-demo-handball" EXECUTABLE_NAME ${PROJECT_NAME} PARAMETERS "handball")
	OV_INSTALL_LAUNCH_SCRIPT(SCRIPT_PREFIX "openvibe-vr-demo-spaceship" EXECUTABLE_NAME ${PROJECT_NAME} PARAMETERS "spaceship")
ENDIF(UNIX)

# -----------------------------
# Install files
# -----------------------------
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION  DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION  DESTINATION ${DIST_LIBDIR})

INSTALL(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/applications/vr-demo)

INSTALL(DIRECTORY signals      DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
INSTALL(DIRECTORY bci-examples DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
