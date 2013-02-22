# ---------------------------------
# Finds Eigen headers
#
# Adds include path
# ---------------------------------

IF(WIN32)
	FIND_PATH(PATH_EIGEN Eigen/Eigen PATHS ${OV_WIN32_DEP_DIR}/eigen)
	IF(PATH_EIGEN)
		SET(EIGEN_FOUND TRUE)
		SET(EIGEN_INCLUDE_DIRS ${PATH_EIGEN})
		SET(EIGEN_CFLAGS "")
		#SET(EIGEN_LIBRARIES_RELEASE EIGEN)
		#SET(EIGEN_LIBRARIES_DEBUG EIGENd)	
		#SET(EIGEN_LIBRARY_DIRS ${PATH_EIGEN}/lib )
	ENDIF(PATH_EIGEN)
ENDIF(WIN32)

IF(UNIX)
	INCLUDE("FindThirdPartyPkgConfig")
	pkg_check_modules(EIGEN eigen3)
ENDIF(UNIX)

IF(EIGEN_FOUND)
	MESSAGE(STATUS "  Found eigen3...")
	INCLUDE_DIRECTORIES(${EIGEN_INCLUDE_DIRS})
	ADD_DEFINITIONS(${EIGEN_CFLAGS})
	
	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEIGEN)
ELSE(EIGEN_FOUND)
	MESSAGE(STATUS "  FAILED to find eigen3...")
ENDIF(EIGEN_FOUND)