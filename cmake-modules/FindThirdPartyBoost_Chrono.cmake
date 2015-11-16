# ---------------------------------
# Finds third party boost chrono
# Adds a def that its present
# ---------------------------------

FIND_PATH(PATH_BOOST_CHRONO "include/boost/chrono.hpp" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/boost ${OV_CUSTOM_DEPENDENCIES_PATH} NO_DEFAULT_PATH)
FIND_PATH(PATH_BOOST_CHRONO "include/boost/chrono.hpp" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/boost)

IF(PATH_BOOST_CHRONO)
	MESSAGE(STATUS "  Found boost chrono includes...")

	ADD_DEFINITIONS(-DTARGET_HAS_Boost_Chrono)
ELSE(PATH_BOOST_CHRONO)
	MESSAGE(STATUS "  FAILED to find boost chrono includes...")
ENDIF(PATH_BOOST_CHRONO)

