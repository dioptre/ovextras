# ---------------------------------
# Finds FFTW3
#
# Sets define if the lib is found, adds include paths
#
# ---------

# On windows, we take the itpp one.

IF(WIN32)
	FIND_PATH(PATH_FFTW3 include/fftw3.h PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/itpp)
	IF(PATH_FFTW3)
		SET(FFTW3_FOUND TRUE)
		SET(FFTW3_INCLUDE_DIRS ${PATH_FFTW3}/include)	
	ENDIF(PATH_FFTW3)
ENDIF(WIN32)

IF(UNIX)
	INCLUDE("FindThirdPartyPkgConfig")
	pkg_check_modules(FFTW3 fftw3)
ENDIF(UNIX)

IF(FFTW3_FOUND)
	MESSAGE(STATUS "  Found fftw3...")
	INCLUDE_DIRECTORIES(${FFTW3_INCLUDE_DIRS})
	ADD_DEFINITIONS(${FFTW3_CFLAGS})	
	
	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyFFTW3)	
ELSE(FFTW3_FOUND)
	MESSAGE(STATUS "  FAILED to find fftw3...")
ENDIF(FFTW3_FOUND)
