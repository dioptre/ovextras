# /*
 # * FindThirdPartyActiCHampAPI.cmake
 # *
 # * Copyright (c) 2012, Mensia Technologies SA. All rights reserved.
 # * -- Rights transferred to Inria, contract signed 21.11.2014
 # *
 # */

# ---------------------------------
# Finds ActiCHamp library
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyActiCHampAPI)

IF(WIN32)
	FIND_PATH(PATH_ActiCHampAPI ActiChamp.h PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/sdk-brainproducts-actichamp)
	IF(PATH_ActiCHampAPI)
		OV_PRINT(OV_PRINTED "  Found actiCHamp API...")
		INCLUDE_DIRECTORIES(${PATH_ActiCHampAPI})

		FIND_LIBRARY(LIB_ActiCHampAPI ActiChamp_x86 PATHS ${PATH_ActiCHampAPI} )
		IF(LIB_ActiCHampAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_ActiCHampAPI}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_ActiCHampAPI} )
		ELSE(LIB_ActiCHampAPI)
			OV_PRINT(OV_PRINTED "    [FAILED] lib actiCHamp")
		ENDIF(LIB_ActiCHampAPI)

		FIND_FILE(FIRMWARE_ActiCHampAPI ActiChamp.bit PATHS ${PATH_ActiCHampAPI} )
		IF(FIRMWARE_ActiCHampAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] firmware ${FIRMWARE_ActiCHampAPI}")
		ELSE(FIRMWARE_ActiCHampAPI)
			OV_PRINT(OV_PRINTED "    [FAILED] firmware actiCHamp")
		ENDIF(FIRMWARE_ActiCHampAPI)

		# Copy the DLL file at install
		INSTALL(PROGRAMS "${PATH_ActiCHampAPI}/ActiChamp_x86.dll" DESTINATION "bin")

		# Copy the firmware file at install
		INSTALL(PROGRAMS "${FIRMWARE_ActiCHampAPI}" DESTINATION "bin")
		
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyActiCHampAPI)
	ELSE(PATH_ActiCHampAPI)
		OV_PRINT(OV_PRINTED "  FAILED to find actiCHamp API (optional)")
	ENDIF(PATH_ActiCHampAPI)
ENDIF(WIN32)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyActiCHampAPI "Yes")

