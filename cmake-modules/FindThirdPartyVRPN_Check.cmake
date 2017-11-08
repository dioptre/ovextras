# ---------------------------------
# Finds VRPN
# Sets PATH_VRPN if found
# ---------------------------------
# The first ${..}/vrpn path is for Windows, the second ${..}/ for Linux

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyVRPN_Check)

FIND_PATH(PATH_VRPN include/vrpn_BaseClass.h PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES vrpn . NO_DEFAULT_PATH)
FIND_PATH(PATH_VRPN include/vrpn_BaseClass.h PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES vrpn .)
IF(PATH_VRPN)
	OV_PRINT(OV_PRINTED "  Found VRPN...")
ELSE(PATH_VRPN)
	OV_PRINT(OV_PRINTED "  FAILED to find VRPN")
ENDIF(PATH_VRPN)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyVRPN_Check "Yes")

