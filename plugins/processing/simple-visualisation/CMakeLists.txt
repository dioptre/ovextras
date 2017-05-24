IF(OV_DISABLE_GTK)
  MESSAGE(STATUS  "Skipping Plugins: Simple Visualisation, no GTK")
  RETURN()
ENDIF(OV_DISABLE_GTK)

PROJECT(openvibe-plugins-simple-visualisation)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.inl)

# @FIXME CERT
LIST(REMOVE_ITEM source_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCSimple3DDisplay.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCSimple3DDisplay.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCTopographicMap3DDisplay.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCTopographicMap3DDisplay.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCVoxelDisplay.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCVoxelDisplay.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCSimple3DDisplay/ovpCSimple3DDatabase.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCSimple3DDisplay/ovpCSimple3DDatabase.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCSimple3DDisplay/ovpCSimple3DView.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCSimple3DDisplay/ovpCSimple3DView.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}src/box-algorithms/ovpCTopographicMap3DDisplay/ovpCTopographicMap3DView.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCTopographicMap3DDisplay/ovpCTopographicMap3DView.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}src/box-algorithms/ovpCPowerSpectrumDisplay/ovpCPowerSpectrumChannelDisplay.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCPowerSpectrumDisplay/ovpCPowerSpectrumChannelDisplay.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}src/box-algorithms/ovpCPowerSpectrumDisplay/ovpCPowerSpectrumDatabase.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCPowerSpectrumDisplay/ovpCPowerSpectrumDatabase.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}src/box-algorithms/ovpCPowerSpectrumDisplay/ovpCPowerSpectrumDisplayView.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCPowerSpectrumDisplay/ovpCPowerSpectrumDisplayView.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}src/box-algorithms/ovpCPowerSpectrumDisplay.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCPowerSpectrumDisplay.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}src/box-algorithms/ovpCVoxelDisplay/ovpCVoxelView.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCVoxelDisplay/ovpCVoxelView.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}src/box-algorithms/ovpCTimeFrequencyMapDisplay.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCTimeFrequencyMapDisplay.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}src/box-algorithms/ovpCTimeFrequencyMapDisplay/ovpCTimeFrequencyMapChannelDisplay.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCTimeFrequencyMapDisplay/ovpCTimeFrequencyMapChannelDisplay.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}src/box-algorithms/ovpCTimeFrequencyMapDisplay/ovpCTimeFrequencyMapDisplayView.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/box-algorithms/ovpCTimeFrequencyMapDisplay/ovpCTimeFrequencyMapDisplayView.cpp")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${source_files})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")

INCLUDE("FindOpenViBE")
INCLUDE("FindOpenViBECommon")
INCLUDE("FindOpenViBEToolkit")
INCLUDE("FindOpenViBEModuleEBML")
INCLUDE("FindOpenViBEModuleSystem")
INCLUDE("FindOpenViBEModuleTCPTagging")
INCLUDE("FindThirdPartyGTK")
INCLUDE("FindThirdPartyBoost")          # need boost for erp plot
INCLUDE("FindOpenViBEVisualizationToolkit")

   
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
	ADD_DEFINITIONS(-fnon-call-exceptions)
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

INSTALL(DIRECTORY share/ DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/openvibe/plugins/simple-visualisation   PATTERN ".svn" EXCLUDE)

INSTALL(DIRECTORY signals       DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/openvibe/scenarios/   PATTERN ".svn" EXCLUDE)
INSTALL(DIRECTORY box-tutorials DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/openvibe/scenarios/   PATTERN ".svn" EXCLUDE)

#INSTALL(DIRECTORY src/   DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h" PATTERN ".svn" EXCLUDE PATTERN "doc" EXCLUDE)
