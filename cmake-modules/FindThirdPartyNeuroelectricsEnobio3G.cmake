# ---------------------------------
# Finds the ENOBIO API & library
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyNeuroelectricsEnobio3G)

IF(WIN32)
  	FIND_PATH(PATH_ENOBIOAPI enobio3g.h PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES sdk-enobio3g/enobio3g)
	IF(NOT PATH_ENOBIOAPI)
		OV_PRINT(OV_PRINTED "  FAILED to find ENOBIO API (optional driver) - cmake looked in '${LIST_DEPENDENCIES_PATH}', skipping Enobio.")
		RETURN()
	ENDIF(NOT PATH_ENOBIOAPI)
	
	OV_PRINT(OV_PRINTED "  Found ENOBIO API...")

	FIND_LIBRARY(LIB_ENOBIOAPI Enobio3GAPI PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES sdk-enobio3g/MSVC)
	IF(NOT LIB_ENOBIOAPI)
		OV_PRINT(OV_PRINTED "    [FAILED] Enobio libs not found, skipping Enobio.")	
		RETURN()
	ENDIF(NOT LIB_ENOBIOAPI)
	
	INCLUDE_DIRECTORIES("${PATH_ENOBIOAPI}")						
	INSTALL(DIRECTORY "${PATH_ENOBIOAPI}/../MSVC/" DESTINATION "${DIST_BINDIR}/" FILES_MATCHING PATTERN "*.dll")
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_ENOBIOAPI} )
	ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEnobioAPI)	
			
	OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_ENOBIOAPI}")

ENDIF(WIN32)

# For now, the Linux building of Enobio3G is disabled as it has not been tested with a recent Neuroelectrics lib.
IF(UNIX)
	OV_PRINT(OV_PRINTED "  Skipped Enobio3G, its work in progress.")
	RETURN()
ENDIF(UNIX)

IF(UNIX)
	SET(QTCORE_INCLUDE_PREFIX /usr/share/qt4/include/)
	SET(QTCORE_LIB_PREFIX /usr/lib/x86_64-linux-gnu/)
	
	FIND_PATH(PATH_ENOBIOAPI enobio3g.h PATHS $ENV{OpenViBE_dependencies} ${OV_BASE_DIR}/contrib/plugins/server-drivers/enobio3G/Enobio3GAPI.linux/)
	IF(PATH_ENOBIOAPI)
		OV_PRINT(OV_PRINTED "  Found ENOBIO API...")
		INCLUDE_DIRECTORIES(${PATH_ENOBIOAPI})
		FIND_PATH(PATH_QTCORE_INCLUDE QtCore/QtCore ${QTCORE_INCLUDE_PREFIX})
		IF(PATH_QTCORE_INCLUDE)
		  OV_PRINT(OV_PRINTED "    [  OK  ] QtCore include ${PATH_QTCORE_INCLUDE}/")
		  INCLUDE_DIRECTORIES(${PATH_QTCORE_INCLUDE}/)
		ELSE(PATH_QTCORE_INCLUDE)
		  OV_PRINT(OV_PRINTED "    FAILED TO FIND QtCore include PATH")
		ENDIF(PATH_QTCORE_INCLUDE)
		FIND_LIBRARY(LIB_ENOBIOAPI Enobio3GAPI PATHS ${PATH_ENOBIOAPI}/libs/ )
		IF(LIB_ENOBIOAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_ENOBIOAPI}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_ENOBIOAPI} )
		ELSE(LIB_ENOBIOAPI)
			OV_PRINT(OV_PRINTED "    [FAILED] lib ENOBIO")
		ENDIF(LIB_ENOBIOAPI)

		FIND_LIBRARY(LIB_QT QtCore ${QTCORE_LIB_PREFIX})
		IF(LIB_QT)
		  OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_QT}")
		  TARGET_LINK_LIBRARIES(${PROJECT_NAME} -L${LIB_QT} -lQtCore)
		ELSE(LIB_QT)
		  OV_PRINT(OV_PRINTED "    [  FAILED  ] lib QT ${QTCORE_LIB_PREFIX}")
		ENDIF(LIB_QT)
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEnobioAPI)
		
		# Copying the DLL file at postbuild
		ADD_CUSTOM_COMMAND(
				TARGET ${PROJECT_NAME}
				POST_BUILD
				COMMAND ${CMAKE_COMMAND}
				ARGS -E copy "${LIB_ENOBIOAPI}" "${PROJECT_SOURCE_DIR}/bin"
				COMMENT "      --->   Copying lib file ${LIB_ENOBIOAPI} for the Neuroelectrics Enobio driver."
			VERBATIM)
	ELSE(PATH_ENOBIOAPI)
		OV_PRINT(OV_PRINTED "  FAILED to find ENOBIO API - cmake looked in $ENV{OpenViBE_dependencies} and in ${OV_BASE_DIR}/contrib/plugins/server-drivers/enobio3G/Enobio3GAPI/")
	ENDIF(PATH_ENOBIOAPI)
ENDIF(UNIX)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyNeuroelectricsEnobio3G "Yes")

