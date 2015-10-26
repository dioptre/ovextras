
IF(WIN32)
	FIND_PATH(PATH_MENSIA openvibe-driver-mensia-acquisition.dll PATHS ${OV_CUSTOM_DEPENDENCIES_PATH}/sdk-mensia-acquisition-driver/ NO_DEFAULT_PATH)
ENDIF(WIN32)

IF(PATH_MENSIA)
	MESSAGE(STATUS "  Found Mensia Acquisition driver...")

	INSTALL(PROGRAMS "${PATH_MENSIA}/openvibe-driver-mensia-acquisition.dll" DESTINATION "bin")
	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyMensiaAcquisition)
ELSE(PATH_MENSIA)
	MESSAGE(STATUS "  FAILED to find Mensia Acquisition driver (optional)")
ENDIF(PATH_MENSIA)

