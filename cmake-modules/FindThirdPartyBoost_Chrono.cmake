# ---------------------------------
# Finds third party boost chrono
# Adds a def that its present
# ---------------------------------

IF(WIN32)
	FIND_PATH(PATH_BOOST_CHRONO "include/boost/chrono.hpp" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/boost ${OV_CUSTOM_DEPENDENCIES_PATH} NO_DEFAULT_PATH)
	IF(PATH_BOOST_CHRONO)
		MESSAGE(STATUS "  Found boost chrono includes...")		
		OV_LINK_BOOST_LIB("chrono" ${OV_WIN32_BOOST_VERSION} )
		IF(LIB_BOOST_PATH AND LIB_BOOST_DEBUG_PATH)
			ADD_DEFINITIONS(-DTARGET_HAS_Boost_Chrono)
		ENDIF(LIB_BOOST_PATH AND LIB_BOOST_DEBUG_PATH)
	ENDIF(PATH_BOOST_CHRONO)
ENDIF(WIN32)

IF(UNIX)

	FIND_PATH(PATH_BOOST_CHRONO "include/boost/chrono.hpp" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/boost ${OV_CUSTOM_DEPENDENCIES_PATH} NO_DEFAULT_PATH)
	FIND_PATH(PATH_BOOST_CHRONO "include/boost/chrono.hpp" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/boost)

	IF(PATH_BOOST_CHRONO)
		MESSAGE(STATUS "  Found boost chrono includes...")

		FIND_LIBRARY(LIB_Boost_Chrono NAMES "boost_chrono" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib NO_DEFAULT_PATH)
		FIND_LIBRARY(LIB_Boost_Chrono NAMES "boost_chrono" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib)
		IF(LIB_Boost_Chrono)
			MESSAGE(STATUS "    [  OK  ] lib ${LIB_Boost_Chrono}")
			ADD_DEFINITIONS(-DTARGET_HAS_Boost_Chrono)
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_Boost_Chrono})
		ELSE(LIB_Boost_Chrono)
			MESSAGE(STATUS "    [FAILED] lib boost_chrono")
		ENDIF(LIB_Boost_Chrono)

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

ENDIF(UNIX)

