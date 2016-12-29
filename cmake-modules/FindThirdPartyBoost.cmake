# ---------------------------------
# Finds third party boost
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyBoost)

FIND_PATH(PATH_BOOST "include/boost/config/auto_link.hpp" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/boost ${OV_CUSTOM_DEPENDENCIES_PATH} NO_DEFAULT_PATH)
FIND_PATH(PATH_BOOST "include/boost/config/auto_link.hpp" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/boost)

IF(PATH_BOOST)
	OV_PRINT(OV_PRINTED "  Found boost includes...")
	INCLUDE_DIRECTORIES(${PATH_BOOST}/include)

	ADD_DEFINITIONS(-DTARGET_HAS_Boost)
ELSE(PATH_BOOST)
	OV_PRINT(OV_PRINTED "  FAILED to find boost includes...")
ENDIF(PATH_BOOST)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyBoost "Yes")

