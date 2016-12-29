# ---------------------------------
# Finds GUSBampCAPI
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyGUSBampCAPI)

IF(WIN32)
	FIND_PATH(PATH_GUSBampCAPI gUSBamp.h PATHS 
		"C:/Program Files/gtec/gUSBampCAPI/API" 
		"C:/Program Files (x86)/gtec/gUSBampCAPI/API" 
		"C:/Program Files/gtec/gUSBampCAPI/API/Win32" 
		"C:/Program Files (x86)/gtec/gUSBampCAPI/API/Win32"
		${OV_CUSTOM_DEPENDENCIES_PATH})
	IF(PATH_GUSBampCAPI)
		OV_PRINT(OV_PRINTED "  Found gUSBampCAPI...")
		INCLUDE_DIRECTORIES(${PATH_GUSBampCAPI})
		FIND_LIBRARY(LIB_GUSBampCAPI gUSBamp PATHS ${PATH_GUSBampCAPI})
		IF(LIB_GUSBampCAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_GUSBampCAPI}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GUSBampCAPI} )
		ELSE(LIB_GUSBampCAPI)
			OV_PRINT(OV_PRINTED "    [FAILED] lib gUSBamp")
		ENDIF(LIB_GUSBampCAPI)

		# Copy the DLL file at install
		INSTALL(PROGRAMS "${PATH_GUSBampCAPI}/gUSBamp.dll" DESTINATION "bin")

		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyGUSBampCAPI)
		SET(OV_ThirdPartyGUSBAmp "YES")
		 
	ELSE(PATH_GUSBampCAPI)
		OV_PRINT(OV_PRINTED "  FAILED to find gUSBampCAPI (optional)")
	ENDIF(PATH_GUSBampCAPI)
ENDIF(WIN32)


IF(UNIX)
	# To try other versions of the gtec's library, change the number below
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so.1.14")
	FIND_LIBRARY(gUSBAmpLinux_LIBRARY NAMES "gusbampapi" PATHS "/usr/lib" "/usr/local/lib")
	IF(gUSBAmpLinux_LIBRARY)
		OV_PRINT(OV_PRINTED "  Found GUSBAmpAPILinux...")
		OV_PRINT(OV_PRINTED "    [  OK  ] Third party lib ${gUSBAmpLinux_LIBRARY}")
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyGUSBampCAPI_Linux)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${gUSBAmpLinux_LIBRARY} )
		SET(OV_ThirdPartyGUSBAmp "YES")
	ELSE(gUSBAmpLinux_LIBRARY)
		OV_PRINT(OV_PRINTED "  FAILED to find GUSBAmpAPI Linux... (optional)")
		OV_PRINT(OV_PRINTED "   : If it should be found, see that 'libgusbampapi.so.1.14' link exists on the fs, with no further nemeric suffixes in the filename.")
		OV_PRINT(OV_PRINTED "   : e.g. do 'cd /usr/lib/ ; ln -s libgusbampapi.so.1.14'. See gtec-bcilab/README for details.")
	ENDIF(gUSBAmpLinux_LIBRARY)
ENDIF(UNIX)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyGUSBampCAPI "Yes")

