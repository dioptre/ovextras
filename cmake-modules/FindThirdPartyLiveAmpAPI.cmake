# /*
 # * FindThirdPartyLiveAmpAPI.cmake
 # *
 # * Copyright (c) 2016, Brain Products GmbH. All rights reserved.
 # * -- Rights transferred to Inria, contract signed ...
 # *
 # */

# ---------------------------------
# Finds LiveAmp library
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_FindThirdPartyLiveAmpAPI)

IF(WIN32)
	FIND_PATH(PATH_LiveAmpAPI Amplifier_LIB.h PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/sdk-brainproducts-liveamp)
	IF(PATH_LiveAmpAPI)
		OV_PRINT(OV_PRINTED "  Found LiveAmp API...")
		INCLUDE_DIRECTORIES(${PATH_LiveAmpAPI})

		FIND_LIBRARY(LIB_LiveAmpAPI LiveAmpLib2 PATHS ${PATH_LiveAmpAPI} )
		IF(LIB_LiveAmpAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_LiveAmpAPI}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_LiveAmpAPI} )
		ELSE(LIB_LiveAmpAPI)
			OV_PRINT(OV_PRINTED "    [FAILED] lib LiveAmp")
		ENDIF(LIB_LiveAmpAPI)

		# Copy the DLL file at install
		INSTALL(PROGRAMS "${PATH_LiveAmpAPI}/LiveAmpLib2.dll" DESTINATION "bin")
		
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyLiveAmpAPI)
	ELSE(PATH_LiveAmpAPI)
		OV_PRINT(OV_PRINTED "  FAILED to find LiveAmp API (optional)")
	ENDIF(PATH_LiveAmpAPI)
ENDIF(WIN32)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_FindThirdPartyLiveAmpAPI "Yes")
