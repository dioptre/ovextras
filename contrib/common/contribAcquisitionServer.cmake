INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/contrib/common")

SET(ADDITIONAL_PATH "${CMAKE_SOURCE_DIR}/contrib/plugins/server-extensions/external-stimulations/")
INCLUDE_DIRECTORIES(${ADDITIONAL_PATH})
FILE(GLOB_RECURSE additional_source_files ${ADDITIONAL_PATH}/*.cpp ${ADDITIONAL_PATH}/*.h)
SET(source_files "${source_files};${additional_source_files}")

SET(ADDITIONAL_PATH "${CMAKE_SOURCE_DIR}/contrib/plugins/server-extensions/tcp-tagging/")
INCLUDE_DIRECTORIES(${ADDITIONAL_PATH})
FILE(GLOB_RECURSE additional_source_files ${ADDITIONAL_PATH}/*.cpp ${ADDITIONAL_PATH}/*.h)
SET(source_files "${source_files};${additional_source_files}")

FUNCTION(OV_ADD_CONTRIB_DRIVER DRIVER_PATH)

	SET(ADDITIONAL_PATH ${DRIVER_PATH})
	INCLUDE_DIRECTORIES(${ADDITIONAL_PATH}/src)
	FILE(GLOB_RECURSE additional_source_files ${ADDITIONAL_PATH}/src/*.cpp ${ADDITIONAL_PATH}/src/*.h)
	SET(source_files "${source_files};${additional_source_files}" PARENT_SCOPE)

	#MESSAGE(STATUS "DO I EXIST: ${ADDITIONAL_PATH}/share/")
	IF(EXISTS "${ADDITIONAL_PATH}/share/")
		#MESSAGE(STATUS "I EXIST: ${ADDITIONAL_PATH}/share/")
		INSTALL(DIRECTORY "${ADDITIONAL_PATH}/share/" DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/openvibe/applications/acquisition-server/" PATTERN ".svn" EXCLUDE)
	ENDIF(EXISTS "${ADDITIONAL_PATH}/share/")

	#MESSAGE(STATUS "DO I EXIST: ${ADDITIONAL_PATH}/bin/")
	IF(EXISTS "${ADDITIONAL_PATH}/bin/")
		#MESSAGE(STATUS "I EXIST: ${ADDITIONAL_PATH}/bin/")
		INSTALL(DIRECTORY "${ADDITIONAL_PATH}/bin/" DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}" PATTERN ".svn" EXCLUDE)
	ENDIF(EXISTS "${ADDITIONAL_PATH}/bin/")

	# Add the dir to be parsed for documentation later.
	GET_PROPERTY(OV_TMP GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS)
	SET(OV_TMP "${OV_TMP};${ADDITIONAL_PATH}")
	SET_PROPERTY(GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS ${OV_TMP})
				
ENDFUNCTION(OV_ADD_CONTRIB_DRIVER)

OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/brainmaster-discovery")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/brainproducts-brainvisionrecorder")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/cognionics")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/ctfvsm-meg")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/field-trip-protocol")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/gtec-gipsa")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/gtec-bcilab")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/gtec-gmobilabplus")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/gtec-gusbamp")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/gtec-gnautilus")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/mbt-smarting")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/mitsarEEG202A")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/openal-mono16bit-audiocapture")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/openeeg-modulareeg")
OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/openbci")

OV_ADD_CONTRIB_DRIVER("${CMAKE_SOURCE_DIR}/contrib/plugins/server-drivers/eemagine-eego")
# Bad hack to get one eego source file we need but that we don't want in git
IF(WIN32)
	SET(EEGOAPI_DIR ${OV_CUSTOM_DEPENDENCIES_PATH}/sdk-eemagine-eego/)
	FIND_PATH(PATH_EEGOAPI amplifier.h PATHS ${EEGOAPI_DIR}/eemagine/sdk/)
	IF(PATH_EEGOAPI)
	  SET(source_files "${source_files};${PATH_EEGOAPI}/wrapper.cc")
	ENDIF(PATH_EEGOAPI)
ENDIF(WIN32)

