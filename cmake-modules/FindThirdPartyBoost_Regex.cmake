# ---------------------------------
# Finds third party boost
# Adds library to target
# Adds include path
# ---------------------------------


IF(UNIX)
	FIND_LIBRARY(LIB_Boost_Regex NAMES "boost_regex-mt" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib NO_DEFAULT_PATH)
	FIND_LIBRARY(LIB_Boost_Regex NAMES "boost_regex-mt" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib)
	
	# test for Ubuntu 13.10 where is not -mt suffixe in libs names at version 1.53.00 of boost lib (multi-threaded libs are identical to standard ?)
	FIND_PROGRAM(LSB_RELEASE NAMES lsb_release)
	IF(LSB_RELEASE)
	    EXEC_PROGRAM("${LSB_RELEASE}" ARGS "-si" OUTPUT_VARIABLE "distrib")
	    EXEC_PROGRAM("${LSB_RELEASE}" ARGS "-sr" OUTPUT_VARIABLE "distrib-release")
	    IF(${distrib} MATCHES "Ubuntu") 
            IF(${distrib-release} MATCHES "13.10")
            	FIND_LIBRARY(LIB_Boost_Regex NAMES "boost_regex" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib NO_DEFAULT_PATH)
	            FIND_LIBRARY(LIB_Boost_Regex NAMES "boost_regex" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib)
            ENDIF(${distrib-release} MATCHES "13.10")
        ENDIF(${distrib} MATCHES "Ubuntu")
    ENDIF(LSB_RELEASE)
	IF(LIB_Boost_Regex)
		MESSAGE(STATUS "    [  OK  ] lib ${LIB_Boost_Regex}")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_Boost_Regex} )
	ELSE(LIB_Boost_Regex)
		MESSAGE(STATUS "    [FAILED] lib boost_regex-mt")
	ENDIF(LIB_Boost_Regex)
ENDIF(UNIX)

IF(WIN32)
	OV_LINK_BOOST_LIB("regex" ${OV_WIN32_BOOST_VERSION})
ENDIF(WIN32)
