PROJECT(openvibe-py2-check)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.inl)
ADD_EXECUTABLE(${PROJECT_NAME} ${source_files})
INCLUDE("FindThirdPartyPython")

# ---------------------------------
# Target macros
# Defines target operating system
# Defines target architecture
# Defines target compiler
# ---------------------------------
SET_BUILD_PLATFORM()
if(WIN32)
	add_definitions("-DWIN32")
endif()
# ----------------------
# Generate launch script
# ----------------------
OV_INSTALL_LAUNCH_SCRIPT(SCRIPT_PREFIX "openvibe-id-generator" EXECUTABLE_NAME "${PROJECT_NAME}" PAUSE)

# -----------------------------
# Install files
# -----------------------------
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})
