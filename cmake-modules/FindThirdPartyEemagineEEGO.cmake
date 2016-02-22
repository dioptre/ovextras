# ---------------------------------
# Finds the Eemagine EEGO API & library
# Adds library to target
# Adds include path
# ---------------------------------
IF(WIN32)
	SET(EEGOAPI_DIR ${OV_CUSTOM_DEPENDENCIES_PATH}/sdk-eemagine-eego/)

	# SET(PATH_EEGOAPI "-NotFound")
	
  	FIND_PATH(PATH_EEGOAPI amplifier.h PATHS ${EEGOAPI_DIR}/eemagine/sdk/)
	IF(NOT PATH_EEGOAPI)
		MESSAGE(STATUS "  FAILED to find EEGO API - cmake looked in '${EEGOAPI_DIR}/eemagine/sdk/', skipping EEGO.")
		RETURN()
	ENDIF(NOT PATH_EEGOAPI)
	
	MESSAGE(STATUS "  Found EEGO API in ${PATH_EEGOAPI}...")

	FIND_FILE(LIB_EEGOAPI NAMES eego-SDK.dll PATHS ${EEGOAPI_DIR}/eemagine/bin/)
	IF(NOT LIB_EEGOAPI)
		MESSAGE(STATUS "    [FAILED] EEGO lib not found in ${EEGOAPI_DIR}/eemagine/bin/, skipping EEGO.")	
		RETURN()
	ENDIF(NOT LIB_EEGOAPI)
	
	INCLUDE_DIRECTORIES(${EEGOAPI_DIR})
	INSTALL(PROGRAMS "${LIB_EEGOAPI}" DESTINATION "bin")	
	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEEGOAPI)	
				
	MESSAGE(STATUS "    [  OK  ] lib ${LIB_EEGOAPI}")

ENDIF(WIN32)


