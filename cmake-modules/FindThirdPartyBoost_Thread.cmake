# ---------------------------------
# Finds third party boost
# Adds library to target
# Adds include path
# ---------------------------------

IF(UNIX)
	FIND_LIBRARY(LIB_Boost_Thread NAMES "boost_thread-mt" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib NO_DEFAULT_PATH)
	FIND_LIBRARY(LIB_Boost_Thread NAMES "boost_thread-mt" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib)
	# test for Ubuntu 13.10 where is not -mt suffixe in libs names at version 1.53.00 of boost lib (multi-threaded libs are identical to standard ?)
	FIND_PROGRAM(LSB_RELEASE NAMES lsb_release)
	IF(LSB_RELEASE)
	    EXEC_PROGRAM("${LSB_RELEASE}" ARGS "-si" OUTPUT_VARIABLE "distrib")
	    EXEC_PROGRAM("${LSB_RELEASE}" ARGS "-sr" OUTPUT_VARIABLE "distrib-release")
	    IF(${distrib} MATCHES "Ubuntu") 
            IF(${distrib-release} MATCHES "13.10")
	            FIND_LIBRARY(LIB_Boost_Thread NAMES "boost_thread" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib NO_DEFAULT_PATH)
	            FIND_LIBRARY(LIB_Boost_Thread NAMES "boost_thread" PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/lib)
            ENDIF(${distrib-release} MATCHES "13.10")
        ENDIF(${distrib} MATCHES "Ubuntu")
    ENDIF(LSB_RELEASE)
	IF(LIB_Boost_Thread)
		MESSAGE(STATUS "    [  OK  ] lib ${LIB_Boost_Thread}")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_Boost_Thread} )
	ELSE(LIB_Boost_Thread)
		MESSAGE(STATUS "    [FAILED] lib boost_thread-mt")
	ENDIF(LIB_Boost_Thread)

	# For Fedora
	FIND_LIBRARY(LIB_STANDARD_MODULE_PTHREAD pthread)
	IF(LIB_STANDARD_MODULE_PTHREAD)
		MESSAGE(STATUS "  Found pthread...")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_STANDARD_MODULE_PTHREAD})
	ELSE(LIB_STANDARD_MODULE_PTHREAD)
		MESSAGE(STATUS "  FAILED to find pthread...")
	ENDIF(LIB_STANDARD_MODULE_PTHREAD)

	# For Ubuntu 13.04 (interprocess/ipc/message_queue.hpp in ovasCPluginExternalStimulations.cpp caused dep)
	FIND_LIBRARY(LIB_STANDARD_MODULE_RT rt)
	IF(LIB_STANDARD_MODULE_RT)
		MESSAGE(STATUS "  Found rt...")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_STANDARD_MODULE_RT})
	ELSE(LIB_STANDARD_MODULE_RT)
		MESSAGE(STATUS "  FAILED to find rt...")
	ENDIF(LIB_STANDARD_MODULE_RT)
	
ENDIF(UNIX)

IF(WIN32)
	OV_LINK_BOOST_LIB("thread" ${OV_WIN32_BOOST_VERSION})
ENDIF(WIN32)
	
