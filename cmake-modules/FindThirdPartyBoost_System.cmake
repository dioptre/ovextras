# ---------------------------------
# Finds third party boost
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyBoost_System)

IF(UNIX)
	FIND_LIBRARY(LIB_Boost_System NAMES "boost_system-mt" PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES lib NO_DEFAULT_PATH)
	FIND_LIBRARY(LIB_Boost_System NAMES "boost_system-mt" PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES lib)

	IF(LIB_Boost_System)
		OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_Boost_System}")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_Boost_System} )
	ELSE(LIB_Boost_System)
		# Fedora 20 and Ubuntu 13.10,14.04 have no more multi-thread boost libs ( *-mt ) so try if there are non -mt libs to link
		FIND_LIBRARY(LIB_Boost_System NAMES "boost_system" PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES lib NO_DEFAULT_PATH)
		FIND_LIBRARY(LIB_Boost_System NAMES "boost_system" PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES lib)
		IF(LIB_Boost_System)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_Boost_System}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_Boost_System})
		ELSE(LIB_Boost_System)
			OV_PRINT(OV_PRINTED "    [FAILED] lib boost_system-mt")	
			OV_PRINT(OV_PRINTED "    [FAILED] lib boost_system")
		ENDIF(LIB_Boost_System)
	ENDIF(LIB_Boost_System)
ENDIF(UNIX)

IF(WIN32)
	OV_LINK_BOOST_LIB("system" ${OV_WIN32_BOOST_VERSION})
ENDIF(WIN32)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyBoost_System "Yes")

