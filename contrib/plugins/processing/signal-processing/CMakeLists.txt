PROJECT(openvibe-plugins-contrib-signal-processing)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

#FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.inl)
set(source_files src/ovp_main.cpp)
set(BOX_ALGORITHMS
    ovpCInputChannel.cpp ovpCSteadyStateFrequencyComparison.cpp ovpCBoxAlgorithmUnivariateStatistics.cpp
    ovpCSpatialFilterBoxAlgorithm.cpp ovpCOutputChannel.cpp ovpCDownsamplingBoxAlgorithm.cpp
    ovpCTemporalFilterBoxAlgorithm.cpp ovpCEpoching.cpp ovpCDetectingMinMaxBoxAlgorithm.cpp
    ovpCBoxAlgorithmSynchro.cpp ovpCBandFrequencyAverage.cpp)
if (TARGET_HAS_ThirdPartyITPP)
    set(BOX_ALGORITHMS ${BOX_ALGORITHMS} ovpCBoxAlgorithmCSPSpatialFilterTrainer.cpp ovpCFastICA.cpp ovpCSpectralAnalysisCospectra.cpp
        ovpCSpectralAnalysis.cpp ovpCTemporalFilter.cpp ovpCWindowingFunctions.cpp)
endif()
foreach (i ${BOX_ALGORITHMS})
    set(source_files ${source_files} src/box-algorithms/${i})
endforeach()

set(ALGORITHMS
    ovpCApplySpatialFilter.cpp ovpCAlgorithmUnivariateStatistics.cpp ovpCDetectingMinMax.cpp ovpCDownsampling.cpp)
if (TARGET_HAS_ThirdPartyITPP)
    set(ALGORITHMS ${ALGORITHMS} ovpCApplyTemporalFilter.cpp ovpCComputeTemporalFilterCoefficients.cpp)
endif()
foreach (i ${ALGORITHMS})
    set(source_files ${source_files} src/algorithms/${i})
endforeach()

ADD_LIBRARY(${PROJECT_NAME} SHARED ${source_files})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	COMPILE_FLAGS "-DOV_Exports -DOV_Shared")

INCLUDE("FindOpenViBE")
INCLUDE("FindOpenViBECommon")
INCLUDE("FindOpenViBEToolkit")
INCLUDE("FindOpenViBEModuleEBML")
INCLUDE("FindOpenViBEModuleSystem")
# INCLUDE("FindThirdPartyBoost")
INCLUDE("FindThirdPartyITPP")

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

#INSTALL(DIRECTORY src/   DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h" PATTERN ".svn" EXCLUDE PATTERN "doc" EXCLUDE)
