# ---------------------------------
# Finds third party boost
# Adds library to target
# Adds include path
# ---------------------------------

INCLUDE("FindThirdPartyBoost")

IF(UNIX)
	FIND_LIBRARY(LIB_Boost_Math NAMES "boost_math_c99" PATHS $ENV{OpenViBE_dependencies}/lib NO_DEFAULT_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
	MESSAGE(STATUS "    looking in $ENV{OV_DEP_BOOST_MATH}")
	IF(LIB_Boost_Math)
		MESSAGE(STATUS "    [  OK  ] lib ${LIB_Boost_Math}")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME}-dynamic ${LIB_Boost_Math} )
	ELSE(LIB_Boost_Math)
		MESSAGE(STATUS "    [FAILED] lib libboost_math_c99")
	ENDIF(LIB_Boost_Math)
ENDIF(UNIX)

IF(WIN32)
	IF(PATH_BOOST)
		LINK_DIRECTORIES(${PATH_BOOST}/lib)
	ENDIF(PATH_BOOST)
ENDIF(WIN32)
