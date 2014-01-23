# ---------------------------------
# Finds third party boost
# Adds library to target
# Adds include path
# ---------------------------------

IF(UNIX)
	FIND_LIBRARY(LIB_Boost_Filesystem NAMES "boost_filesystem-mt" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib NO_DEFAULT_PATH)
	FIND_LIBRARY(LIB_Boost_Filesystem NAMES "boost_filesystem-mt" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib)
	# test for Ubuntu 13.10 where is not -mt suffixe in libs names at version 1.53.00 of boost lib (multi-threaded libs are identical to standard ?)
	FIND_PROGRAM(LSB_RELEASE NAMES lsb_release)
	IF(LSB_RELEASE)
	    EXEC_PROGRAM("${LSB_RELEASE}" ARGS "-si" OUTPUT_VARIABLE "distrib")
	    EXEC_PROGRAM("${LSB_RELEASE}" ARGS "-sr" OUTPUT_VARIABLE "distrib-release")
	    IF(${distrib} MATCHES "Ubuntu") 
            IF(${distrib-release} MATCHES "13.10")
            	FIND_LIBRARY(LIB_Boost_Filesystem NAMES "boost_filesystem" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib NO_DEFAULT_PATH)
	            FIND_LIBRARY(LIB_Boost_Filesystem NAMES "boost_filesystem" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib)
            ENDIF(${distrib-release} MATCHES "13.10")
        ENDIF(${distrib} MATCHES "Ubuntu")
    ENDIF(LSB_RELEASE)

	IF(LIB_Boost_Filesystem)
		MESSAGE(STATUS "    [  OK  ] lib ${LIB_Boost_Filesystem}")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_Boost_Filesystem} )
	ELSE(LIB_Boost_Filesystem)
		MESSAGE(STATUS "    [FAILED] lib boost_Filesystem-mt")
	ENDIF(LIB_Boost_Filesystem)

	# For Fedora
	FIND_LIBRARY(LIB_STANDARD_MODULE_PTHREAD pthread)
	IF(LIB_STANDARD_MODULE_PTHREAD)
		MESSAGE(STATUS "  Found pthread...")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_STANDARD_MODULE_PTHREAD})
	ELSE(LIB_STANDARD_MODULE_PTHREAD)
		MESSAGE(STATUS "  FAILED to find pthread...")
	ENDIF(LIB_STANDARD_MODULE_PTHREAD)
ENDIF(UNIX)

IF(WIN32)
	OV_LINK_BOOST_LIB("filesystem" ${OV_WIN32_BOOST_VERSION} )
	OV_LINK_BOOST_LIB("system" ${OV_WIN32_BOOST_VERSION} )			# filesystem depends on system
ENDIF(WIN32)
