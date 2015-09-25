
INCLUDE(CheckLanguage)

check_language(Fortran)
if(NOT CMAKE_Fortran_COMPILER)
	MESSAGE(STATUS "Fortran not available, skipping Fortran example")
	RETURN()
ENDIF(NOT CMAKE_Fortran_COMPILER)

enable_language(Fortran)
#   INCLUDE(FortranCInterface) 

PROJECT(openvibe-plugins-fortran-bridge CXX C Fortran)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

IF(WIN32)
	INCLUDE("CMakeAddFortranSubdirectory")
	cmake_add_fortran_subdirectory("src/box-algorithms/example1" 
		PROJECT openvibe-plugins-fortran-example1
		LINK_LIBRARIES
		NO_EXTERNAL_INSTALL)
	FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.inl)
ELSE(WIN32)
	FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.inl *.f90)
ENDIF(WIN32)

ADD_LIBRARY(${PROJECT_NAME} SHARED ${source_files})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	COMPILE_FLAGS "-DOV_Exports -DOV_Shared")

IF(WIN32)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} openvibe-plugins-fortran-example1)
	#MESSAGE(STATUS "lib 1 ${CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES}")
	#MESSAGE(STATUS "lib 2 ${CMAKE_Fortran_IMPLICIT_LINK_DIRECTORIES}")
ENDIF(WIN32)

INCLUDE("FindOpenViBE")
INCLUDE("FindOpenViBECommon")
INCLUDE("FindOpenViBEToolkit")
INCLUDE("FindOpenViBEModuleEBML")
INCLUDE("FindOpenViBEModuleSystem")

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

#ENDIF(NOT CMAKE_Fortran_COMPILER)


#ENDIF(UNIX)
