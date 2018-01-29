GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyEemagineEEGO)

# ---------------------------------
# Finds the Eemagine EEGO API & library
# Adds library to target
# Adds include path
# ---------------------------------

if (WIN32)
    FIND_PATH(PATH_EEGOAPI amplifier.h PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES sdk-eemagine-eego/eemagine/sdk/)
else()
    FIND_PATH(PATH_EEGOAPI amplifier.h PATHS /usr/include PATH_SUFFIXES eemagine/sdk/)
endif(WIN32)

IF(NOT PATH_EEGOAPI)
    OV_PRINT(OV_PRINTED "  FAILED to find EEGO API (optional driver) - cmake looked in '${LIST_DEPENDENCIES_PATH}', skipping EEGO.")
    RETURN()
ENDIF(NOT PATH_EEGOAPI)

OV_PRINT(OV_PRINTED "  Found EEGO API in ${PATH_EEGOAPI}...")

if (WIN32)
    FIND_FILE(LIB_EEGOAPI NAMES eego-SDK.dll PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES sdk-eemagine-eego/eemagine/bin/)
else()
    find_library(LIB_EEGOAPI NAMES eego-SDK)
    target_link_libraries(${PROJECT_NAME} -ldl)
    OV_PRINT(OV_PRINTED "  LIB_EEGOAPI : ${LIB_EEGOAPI}")
endif(WIN32)

IF(NOT LIB_EEGOAPI)
    OV_PRINT(OV_PRINTED "    [FAILED] EEGO lib not found under '${LIST_DEPENDENCIES_PATH}', skipping EEGO.")
    RETURN()
ENDIF(NOT LIB_EEGOAPI)

INCLUDE_DIRECTORIES("${PATH_EEGOAPI}/../../")

INSTALL(PROGRAMS "${LIB_EEGOAPI}" DESTINATION ${DIST_BINDIR})
ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEEGOAPI)
ADD_DEFINITIONS(-DEEGO_SDK_BIND_DYNAMIC)
	
OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_EEGOAPI}")

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyEemagineEEGO "Yes")
