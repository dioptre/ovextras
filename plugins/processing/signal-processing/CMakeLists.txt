PROJECT(openvibe-plugins-signal-processing)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.inl)
ADD_LIBRARY(${PROJECT_NAME} SHARED ${source_files}
	"../../../contrib/packages/wavelet2d/wavelet2s.cpp"
	"../../../contrib/packages/wavelet2d/wavelet2s.h")
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")

INCLUDE_DIRECTORIES("../../../contrib/packages/wavelet2d")

INCLUDE("FindOpenViBE")
INCLUDE("FindOpenViBECommon")
INCLUDE("FindOpenViBEToolkit")
INCLUDE("FindOpenViBEModuleEBML")
INCLUDE("FindOpenViBEModuleSystem")
INCLUDE("FindThirdPartyBoost")
INCLUDE("FindThirdPartyEigen")
INCLUDE("FindThirdPartyFFTW3")		# used by the wavelet library
INCLUDE("FindThirdPartyITPP")		# note that itpp gives the fftw3 on Win

# ---------------------------------
# Target macros
# Defines target operating system
# Defines target architecture
# Defines target compiler
# ---------------------------------
SET_BUILD_PLATFORM()

IF(OV_COMPILE_TESTS)
	ADD_SUBDIRECTORY("test")
ENDIF(OV_COMPILE_TESTS)

# -----------------------------
# Install files
# -----------------------------
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

INSTALL(DIRECTORY signals/       DESTINATION ${DIST_DATADIR}/openvibe/scenarios/signals)
INSTALL(DIRECTORY box-tutorials/ DESTINATION ${DIST_DATADIR}/openvibe/scenarios/box-tutorials)
	