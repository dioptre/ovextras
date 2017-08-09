PROJECT(openvibe-examples-openvibe-to-vrpn)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

INCLUDE("FindThirdPartyVRPN_Check")
IF(NOT PATH_VRPN)
	MESSAGE(STATUS "    --> Not building ${PROJECT_NAME}")
	RETURN()
ENDIF(NOT PATH_VRPN)

FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.hpp src/*.inl include/*.h)
INCLUDE_DIRECTORIES(include)

ADD_EXECUTABLE(${PROJECT_NAME} ${source_files})

INCLUDE("FindThirdPartyVRPN")
INCLUDE("FindThirdPartyPThread") # needed on Linux for VRPN

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
OV_INSTALL_LAUNCH_SCRIPT(SCRIPT_PREFIX "${PROJECT_NAME}" EXECUTABLE_NAME  "${PROJECT_NAME}")

# -----------------------------
# Install files
# -----------------------------
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

INSTALL(DIRECTORY box-tutorials/ DESTINATION ${DIST_DATADIR}/openvibe/scenarios/box-tutorials)
