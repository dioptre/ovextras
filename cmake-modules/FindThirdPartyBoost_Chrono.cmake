# ---------------------------------
# Finds third party boost chrono
# Adds a def that its present
# ---------------------------------

FIND_PATH(PATH_BOOST_CHRONO "include/boost/chrono.hpp" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/boost ${OV_CUSTOM_DEPENDENCIES_PATH} NO_DEFAULT_PATH)
FIND_PATH(PATH_BOOST_CHRONO "include/boost/chrono.hpp" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/boost)

IF(PATH_BOOST_CHRONO)
	MESSAGE(STATUS "  Found boost chrono includes...")

	ADD_DEFINITIONS(-DTARGET_HAS_Boost_Chrono)

        FIND_LIBRARY(LIB_Boost_Chrono NAMES "boost_chrono" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib NO_DEFAULT_PATH)
        FIND_LIBRARY(LIB_Boost_Chrono NAMES "boost_chrono" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib)
        IF(LIB_Boost_Chrono)
                MESSAGE(STATUS "    [  OK  ] lib ${LIB_Boost_Chrono}")
                TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_Boost_Chrono})
        ELSE(LIB_Boost_Chrono)
                MESSAGE(STATUS "    [FAILED] lib boost_chrono")
        ENDIF(LIB_Boost_Chrono)

	TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_Boost_Chrono})

	# Fedora / Ubuntu
        FIND_LIBRARY(LIB_STANDARD_MODULE_RT rt)
        IF(LIB_STANDARD_MODULE_RT)
                MESSAGE(STATUS "  Found rt...")
                TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_STANDARD_MODULE_RT})
        ELSE(LIB_STANDARD_MODULE_RT)
                MESSAGE(STATUS "  FAILED to find rt...")
        ENDIF(LIB_STANDARD_MODULE_RT)

ELSE(PATH_BOOST_CHRONO)
	MESSAGE(STATUS "  FAILED to find boost chrono includes...")
ENDIF(PATH_BOOST_CHRONO)
