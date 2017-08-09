
IF(OV_DISABLE_GTK)
  MESSAGE(STATUS  "Skipping Skeleton Generator, no GTK")
  RETURN()
ENDIF(OV_DISABLE_GTK)

PROJECT(openvibe-skeleton-generator)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.inl)
ADD_EXECUTABLE(${PROJECT_NAME} ${source_files})

INCLUDE("FindOpenViBE")
INCLUDE("FindOpenViBECommon")
INCLUDE("FindOpenViBEToolkit")
INCLUDE("FindOpenViBEModuleSystem")
INCLUDE("FindOpenViBEModuleEBML")
INCLUDE("FindOpenViBEModuleFS")
INCLUDE("FindThirdPartyGTK")
INCLUDE("FindThirdPartyBoost")
INCLUDE("FindThirdPartyBoost_Thread")
INCLUDE("FindThirdPartyBoost_Regex")

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

INSTALL(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/applications/skeleton-generator)
