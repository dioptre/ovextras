# ---------------------------------
# Finds third party boost
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyBoost_Math)

INCLUDE("FindThirdPartyBoost")

IF(UNIX)
	FIND_LIBRARY(LIB_Boost_Math NAMES "boost_math_c99" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib NO_DEFAULT_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
	# OV_PRINT(OV_PRINTED "    looking in ${OV_CUSTOM_DEPENDENCIES_PATH}")
	IF(LIB_Boost_Math)
		OV_PRINT(OV_PRINTED "  Found boost math...")	
		OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_Boost_Math}")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_Boost_Math} )
	ELSE(LIB_Boost_Math)
		OV_PRINT(OV_PRINTED "  FAILED to find boost math...")		
		OV_PRINT(OV_PRINTED "    [FAILED] lib libboost_math_c99")
	ENDIF(LIB_Boost_Math)
ENDIF(UNIX)

IF(WIN32)
	IF(PATH_BOOST)
		OV_LINK_BOOST_LIB("math_c99" ${OV_WIN32_BOOST_VERSION} )
	ENDIF(PATH_BOOST)
ENDIF(WIN32)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyBoost_Math "Yes")

