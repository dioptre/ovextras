# ---------------------------------
# Finds third party boost
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyBoost_Thread)

IF(UNIX)
	FIND_LIBRARY(LIB_Boost_Thread NAMES "boost_thread-mt" PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES lib NO_DEFAULT_PATH)
	FIND_LIBRARY(LIB_Boost_Thread NAMES "boost_thread-mt" PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES lib)

	IF(LIB_Boost_Thread)
		OV_PRINT(OV_PRINTED "  Found boost thread...")
		OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_Boost_Thread}")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_Boost_Thread} )
	ELSE(LIB_Boost_Thread)
		# Fedora 20 and Ubuntu 13.10,14.04 have no more multi-thread boost libs ( *-mt ) so try if there are non -mt libs to link
		FIND_LIBRARY(LIB_Boost_Thread NAMES "boost_thread" PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES lib NO_DEFAULT_PATH)
		FIND_LIBRARY(LIB_Boost_Thread NAMES "boost_thread" PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES lib)
		IF(LIB_Boost_Thread)
			OV_PRINT(OV_PRINTED "  Found boost thread...")
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_Boost_Thread}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_Boost_Thread})
		ELSE(LIB_Boost_Thread)
			OV_PRINT(OV_PRINTED "  FAILED to find boost thread...")		
			OV_PRINT(OV_PRINTED "    [FAILED] lib boost_thread-mt")		
			OV_PRINT(OV_PRINTED "    [FAILED] lib boost_thread")
		ENDIF(LIB_Boost_Thread)
	ENDIF(LIB_Boost_Thread)

	# For Fedora
	INCLUDE("FindThirdPartyPThread")

	# For Ubuntu 13.04 (interprocess/ipc/message_queue.hpp in ovasCPluginExternalStimulations.cpp caused dep)
	FIND_LIBRARY(LIB_STANDARD_MODULE_RT rt)
	IF(LIB_STANDARD_MODULE_RT)
		OV_PRINT(OV_PRINTED "  Found rt...")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_STANDARD_MODULE_RT})
	ELSE(LIB_STANDARD_MODULE_RT)
		OV_PRINT(OV_PRINTED "  FAILED to find rt...")
	ENDIF(LIB_STANDARD_MODULE_RT)
	
ENDIF(UNIX)

IF(WIN32)
	OV_LINK_BOOST_LIB("thread" ${OV_WIN32_BOOST_VERSION})
ENDIF(WIN32)
	

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyBoost_Thread "Yes")

