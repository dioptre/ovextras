
GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyMensiaAcquisition)


IF(WIN32)
	FIND_PATH(PATH_MENSIA openvibe-driver-mensia-acquisition.dll PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES sdk-mensia-acquisition-driver NO_DEFAULT_PATH)
ENDIF(WIN32)

IF(PATH_MENSIA)
	OV_PRINT(OV_PRINTED "  Found Mensia Acquisition driver...")

	INSTALL(DIRECTORY "${PATH_MENSIA}/" DESTINATION "${DIST_BINDIR}/" FILES_MATCHING PATTERN "*.dll")
	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyMensiaAcquisition)
ELSE(PATH_MENSIA)
	OV_PRINT(OV_PRINTED "  FAILED to find Mensia Acquisition driver (optional driver)")
ENDIF(PATH_MENSIA)


SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyMensiaAcquisition "Yes")

