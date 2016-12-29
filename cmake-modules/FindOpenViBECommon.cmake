# ---------------------------------
# Finds OpenViBE common include files
# Adds dependency to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBECommon)

FIND_PATH(PATH_OPENVIBE_COMMON ov_common_defines.h PATHS ${OV_BASE_DIR}/common/include NO_DEFAULT_PATH)
IF(PATH_OPENVIBE_COMMON)
	OV_PRINT(OV_PRINTED "  Found openvibe-common...")
	INCLUDE_DIRECTORIES(${PATH_OPENVIBE_COMMON})
	
	ADD_DEPENDENCIES(${PROJECT_NAME} openvibe-common)
	
	ADD_DEFINITIONS(-DTARGET_HAS_OpenViBE_Common)
	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)	
ELSE(PATH_OPENVIBE_COMMON)
	OV_PRINT(OV_PRINTED "  FAILED to find openvibe-common...")
ENDIF(PATH_OPENVIBE_COMMON)



SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBECommon "Yes")

