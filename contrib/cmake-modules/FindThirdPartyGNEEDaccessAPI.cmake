# ---------------------------------
# Finds GNEEDaccessAPI
# Adds library to target
# Adds include path
# ---------------------------------

#IF(OV_ThirdPartyGMobilab)
#	MESSAGE(STATUS "  NOTE gtec Mobilab has already been found, cannot have gUSBAmp driver in the same executable")
#	MESSAGE(STATUS "    [ SKIP ] gUSBampCAPI...")
#	RETURN()
#ENDIF(OV_ThirdPartyGMobilab)

IF(WIN32)
	FIND_PATH(PATH_GNEEDaccessAPI GDSClientAPI.h PATHS
    "C:/Program Files/gtec/gNEEDaccess Client API/C" 
		"C:/Program Files (x86)/gtec/gNEEDaccess Client API/C"
    "C:/Program Files/gtec/gNEEDaccess Client API/C/win32"
		"C:/Program Files (x86)/gtec/gNEEDaccess Client API/C/win32"
		${OV_CUSTOM_DEPENDENCIES_PATH})
	IF(PATH_GNEEDaccessAPI)
		MESSAGE(STATUS "  Found gNEEDaccessAPI...")
		INCLUDE_DIRECTORIES(${PATH_GNEEDaccessAPI})
    FIND_PATH(PATH_ClientAPI GDSClientAPI.lib PATHS
      "C:/Program Files (x86)/gtec/gNEEDaccess Client API/C/win32"
      "C:/Program Files/gtec/gNEEDaccess Client API/C/win32"
    )
    IF(PATH_ClientAPI)
		FIND_LIBRARY(LIB_GDSClientAPI GDSClientAPI PATHS ${PATH_ClientAPI})
		MESSAGE(STATUS "${LIB_GDSClientAPI}")
		IF(LIB_GDSClientAPI)
			MESSAGE(STATUS "    [  OK  ] lib ${LIB_GDSClientAPI}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GDSClientAPI} )
		ELSE(LIB_GDSClientAPI)
			MESSAGE(STATUS "    [FAILED] lib GDSClientAPI")
		ENDIF(LIB_GDSClientAPI)
    ENDIF(PATH_ClientAPI)
  
    FIND_PATH(PATH_ServerLIB GDSServer.lib PATHS
      "C:/Program Files (x86)/gtec/gNEEDaccess Client API/C/win32"
      "C:/Program Files/gtec/gNEEDaccess Client API/C/win32"
    )
    IF(PATH_ServerLIB)
		# Find GDSServer lib and dll
		FIND_LIBRARY(LIB_GDSServer GDSServer PATHS ${PATH_ServerLIB})
		IF(LIB_GDSServer)
			MESSAGE(STATUS "    [  OK  ] lib ${LIB_GDSServer}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GDSServer} )
		ELSE(LIB_GDSServer)
			MESSAGE(STATUS "    [FAILED] lib GDSServer")
		ENDIF(LIB_GDSServer)
    ENDIF(PATH_ServerLIB)
    
	# Copy the DLL file at install
	INSTALL(PROGRAMS "${PATH_ClientAPI}/GDSClientAPI.dll" DESTINATION "bin")
    INSTALL(PROGRAMS "${PATH_ClientAPI}/gAPI.dll" DESTINATION "bin")
    INSTALL(PROGRAMS "${PATH_ClientAPI}/Networking.dll" DESTINATION "bin")
    INSTALL(PROGRAMS "${PATH_ServerLIB}/GDSServer.dll" DESTINATION "bin")
    
	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyGNEEDaccessAPI)
	SET(OV_ThirdPartyGNEEDaccess "YES")
		 
	ELSE(PATH_GNEEDaccessAPI)
		MESSAGE(STATUS "  FAILED to find gNEEDaccessAPI (optional)")
	ENDIF(PATH_GNEEDaccessAPI)
ENDIF(WIN32)

