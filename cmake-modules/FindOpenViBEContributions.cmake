# ---------------------------------
# Finds OpenViBE contributions
# Only serves to set up a define for the preprocessor
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBEContributions)

FIND_PATH(PATH_OPENVIBE_CONTRIBUTIONS common/contribAcquisitionServer.cmake PATHS ${OV_BASE_DIR}/contrib  NO_DEFAULT_PATH)
IF(PATH_OPENVIBE_CONTRIBUTIONS)
	OV_PRINT(OV_PRINTED "  Found openvibe-contributions...")

	ADD_DEFINITIONS(-DTARGET_HAS_OpenViBEContributions)
ELSE(PATH_OPENVIBE_CONTRIBUTIONS)
	OV_PRINT(OV_PRINTED "  FAILED to find openvibe-contributions...")
ENDIF(PATH_OPENVIBE_CONTRIBUTIONS)


SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBEContributions "Yes")

