
IF(OV_DISABLE_GTK)
  MESSAGE(STATUS  "Skipping Convert, no GTK (no Designer)")
  RETURN()
ENDIF()

PROJECT(openvibe-convert)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

ADD_CUSTOM_TARGET(openvibe-convert SOURCES
	CMakeLists.txt src/openvibe-convert-real.cmd-base src/openvibe-convert-real.sh-base)

ADD_DEPENDENCIES(openvibe-convert openvibe-plugins-file-io)

# ----------------------
# Generate launch script
# ----------------------
IF(WIN32) 
	SET(OV_EXTENSION "cmd")
ELSEIF(UNIX)
	SET(OV_EXTENSION "sh")
ENDIF()

# Create the actual script thats run
CONFIGURE_FILE(src/openvibe-convert-real.${OV_EXTENSION}-base ${CMAKE_CURRENT_BINARY_DIR}/openvibe-convert-real.${OV_EXTENSION} @ONLY)

# Required for OV_INSTALL_LAUNCH_SCRIPT
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LOCATION openvibe-convert-real.${OV_EXTENSION}) 

# -----------------------------
# Install files
# -----------------------------

# This will call the actual script
OV_INSTALL_LAUNCH_SCRIPT(SCRIPT_PREFIX "${PROJECT_NAME}" EXECUTABLE_NAME  "${PROJECT_NAME}")

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/openvibe-convert-real.${OV_EXTENSION} DESTINATION ${DIST_BINDIR})

INSTALL(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/scenarios/convert)

