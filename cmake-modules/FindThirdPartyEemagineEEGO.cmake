
GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyEemagineEEGO)

# ---------------------------------
# Finds the Eemagine EEGO API & library
# Adds library to target
# Adds include path
# ---------------------------------
IF(WIN32)
	# SET(PATH_EEGOAPI "-NOTFOUND")
	
  	FIND_PATH(PATH_EEGOAPI amplifier.h PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES sdk-eemagine-eego/eemagine/sdk/)
	IF(NOT PATH_EEGOAPI)
		OV_PRINT(OV_PRINTED "  FAILED to find EEGO API (optional driver) - cmake looked in '${LIST_DEPENDENCIES_PATH}', skipping EEGO.")
		RETURN()
	ENDIF(NOT PATH_EEGOAPI)
	
	OV_PRINT(OV_PRINTED "  Found EEGO API in ${PATH_EEGOAPI}...")

	FIND_FILE(LIB_EEGOAPI NAMES eego-SDK.dll PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES sdk-eemagine-eego/eemagine/bin/)
	IF(NOT LIB_EEGOAPI)
		OV_PRINT(OV_PRINTED "    [FAILED] EEGO lib not found under '${LIST_DEPENDENCIES_PATH}', skipping EEGO.")	
		RETURN()
	ENDIF(NOT LIB_EEGOAPI)
	
	INCLUDE_DIRECTORIES("${PATH_EEGOAPI}/../../")
	INSTALL(PROGRAMS "${LIB_EEGOAPI}" DESTINATION "bin")	
	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEEGOAPI)	
	ADD_DEFINITIONS(-DEEGO_SDK_BIND_DYNAMIC)
	
	OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_EEGOAPI}")

ENDIF(WIN32)



SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyEemagineEEGO "Yes")

