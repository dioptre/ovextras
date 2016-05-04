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

	SET(PATHS_GNEEDaccessAPI    "C:/Program Files/gtec/gNEEDaccess Client API/C" "C:/Program Files (x86)/gtec/gNEEDaccess Client API/C")
	SET(PATHS_GNEEDaccessLIB    "C:/Program Files/gtec/gNEEDaccess Client API/C/win32" "C:/Program Files (x86)/gtec/gNEEDaccess Client API/C/win32")
	SET(PATHS_GNEEDaccessServer "C:/Program Files/gtec/gNEEDaccess/" "C:/Program Files (x86)/gtec/gNEEDaccess/")
		  
	FIND_PATH(PATH_GNEEDaccessAPI GDSClientAPI.h PATHS 
		${PATHS_GNEEDaccessAPI}
		NO_DEFAULT_PATH)
	IF(PATH_GNEEDaccessAPI)
		MESSAGE(STATUS "  Found gNEEDaccessAPI...")

		# Find GDSClientAPI lib and dll		
		FIND_PATH(PATH_ClientLIB GDSClientAPI.dll PATHS 
			${PATHS_GNEEDaccessLIB}
			NO_DEFAULT_PATH)
		
		FIND_LIBRARY(LIB_GDSClientAPI GDSClientAPI PATHS ${PATHS_GNEEDaccessLIB} NO_DEFAULT_PATH)
		IF(LIB_GDSClientAPI)
			MESSAGE(STATUS "    [  OK  ] lib ${LIB_GDSClientAPI}")
		ELSE(LIB_GDSClientAPI)
			MESSAGE(STATUS "    [FAILED] lib GDSClientAPI")
		ENDIF(LIB_GDSClientAPI)
	  
		# Find GDSServer dll
		FIND_PATH(PATH_ServerDLL GDSServer.dll PATHS 
			${PATHS_GNEEDaccessServer}
			NO_DEFAULT_PATH)
		IF(PATH_ServerDLL)
			MESSAGE(STATUS "    [  OK  ] dll ${PATH_ServerDLL}")
		ELSE(PATH_ServerDLL)
			MESSAGE(STATUS "    [FAILED] dll GDSServer")
		ENDIF(PATH_ServerDLL)
		
		# Find GDSServer lib
		FIND_LIBRARY(LIB_GDSServer GDSServer PATHS ${PATHS_GNEEDaccessLIB} NO_DEFAULT_PATH)
		IF(LIB_GDSServer)
			MESSAGE(STATUS "    [  OK  ] lib ${LIB_GDSServer}")
		ELSE(LIB_GDSServer)
			MESSAGE(STATUS "    [FAILED] lib GDSServer")
		ENDIF(LIB_GDSServer)
		
		# MESSAGE(STATUS "1, ${PATH_ClientLIB} 2, ${LIB_GDSClientAPI} 3, ${PATH_ServerDLL} 4, ${LIB_GDSServer}")
				
		# Only add the compile/install directive if all necessary components were found
		IF(PATH_ClientLIB AND LIB_GDSClientAPI AND PATH_ServerDLL AND LIB_GDSServer)
			# Copy the DLL file at install
			INSTALL(PROGRAMS "${PATH_ClientLIB}/GDSClientAPI.dll" DESTINATION "bin")
			INSTALL(PROGRAMS "${PATH_ClientLIB}/gAPI.dll" DESTINATION "bin")
			INSTALL(PROGRAMS "${PATH_ClientLIB}/Networking.dll" DESTINATION "bin")
			INSTALL(PROGRAMS "${PATH_ServerDLL}/GDSServer.dll" DESTINATION "bin")
		
			INCLUDE_DIRECTORIES(${PATH_GNEEDaccessAPI})
			
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GDSClientAPI} )	
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GDSServer} )
						
			ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyGNEEDaccessAPI)
			SET(OV_ThirdPartyGNEEDaccess "YES")
		ENDIF(PATH_ClientLIB AND LIB_GDSClientAPI AND PATH_ServerDLL AND LIB_GDSServer)
	ELSE(PATH_GNEEDaccessAPI)
		MESSAGE(STATUS "  FAILED to find gNEEDaccessAPI (optional)")
	ENDIF(PATH_GNEEDaccessAPI)
ENDIF(WIN32)

