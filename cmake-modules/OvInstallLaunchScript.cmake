INCLUDE(CMakeParseArguments)
#
# Creates launch script from a common OpenViBE template (in "cmake-modules/launchers/"), but dedicated to scenarios to be executed with the Designer
#
# The mandatory 1st argument SCRIPT_PREFIX specifies what the resulting script is called. A platform specific postfix will be added.
# The mandatory 2nd argument EXECUTABLE_NAME specifies what the resulting script will called eventually.
# The optional 3nd argument ARGV1 specifies some extra argument or switch that is given to the launched executable by the script
#
FUNCTION(OV_INSTALL_LAUNCH_SCRIPT)
	SET(options PAUSE NOPROJECT)
	SET(oneValueArgs SCRIPT_PREFIX EXECUTABLE_NAME ICON_PATH)
	SET(multiValueArgs PARAMETERS)

	CMAKE_PARSE_ARGUMENTS(OV_INSTALL_LAUNCH_SCRIPT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	#OV_CONFIGURE_RC(NAME ${OV_INSTALL_LAUNCH_SCRIPT_SCRIPT_PREFIX} ICON_PATH ${OV_INSTALL_LAUNCH_SCRIPT_ICON_PATH})

	# Install executable launcher if install_exe option is set to on, the os is WIN32, and no argument has been specified
	IF(WIN32 AND INSTALL_EXE AND NOT(OV_INSTALL_LAUNCH_SCRIPT_PAUSE) AND NOT(OV_INSTALL_LAUNCH_SCRIPT_PARAMETERS))
		# Add the dir to be parsed for documentation later. We need to do this before adding subdir, in case the subdir is the actual docs dir
		GET_PROPERTY(OV_TMP GLOBAL PROPERTY OV_EXE_PROJECTS_TO_INSTALL)
		SET(OV_TMP "${OV_TMP};${OV_INSTALL_LAUNCH_SCRIPT_SCRIPT_PREFIX}")
		SET_PROPERTY(GLOBAL PROPERTY OV_EXE_PROJECTS_TO_INSTALL ${OV_TMP})
	ELSE()
		IF(WIN32)
			SET(SCRIPT_POSTFIX ".cmd")
		ELSEIF(APPLE)
			SET(SCRIPT_POSTFIX "-macos.sh")
		ELSEIF(UNIX)
			# Debian recommends that extensions such as .sh are not used; On Linux, scripts with such extensions shouldn't be packaged
			SET(SCRIPT_POSTFIX ".sh")
		ENDIF()
		# Extract the filename of the project executable, the variable in the script base will be replaced with it by CONFIGURE_FILE()
		SET(OV_CMD_EXECUTABLE "TMP_PROJECT_TARGET_PATH-NOTFOUND")
		IF(NOT OV_INSTALL_LAUNCH_SCRIPT_NOPROJECT)
			GET_TARGET_PROPERTY(TMP_PROJECT_TARGET_PATH ${OV_INSTALL_LAUNCH_SCRIPT_EXECUTABLE_NAME} LOCATION)
			GET_FILENAME_COMPONENT(OV_CMD_EXECUTABLE ${TMP_PROJECT_TARGET_PATH} NAME)
		ENDIF()

		IF(${OV_CMD_EXECUTABLE} STREQUAL "TMP_PROJECT_TARGET_PATH-NOTFOUND")
			SET(OV_CMD_EXECUTABLE ${OV_INSTALL_LAUNCH_SCRIPT_EXECUTABLE_NAME})
		# ELSE()
			# IF(WIN32)
				# SET(OV_CMD_EXECUTABLE "%OV_PATH_ROOT%\\bin\\${OV_CMD_EXECUTABLE}")
			# ENDIF()
		ENDIF()
		
		SET(SCRIPT_NAME ${OV_INSTALL_LAUNCH_SCRIPT_SCRIPT_PREFIX}${SCRIPT_POSTFIX})
		SET(OV_CMD_ARGS ${OV_INSTALL_LAUNCH_SCRIPT_PARAMETERS})
		
		IF(OV_INSTALL_LAUNCH_SCRIPT_PAUSE)
			SET(OV_PAUSE "PAUSE")
		ELSE()
			SET(OV_PAUSE "")
		ENDIF()
		
		CONFIGURE_FILE(${OV_LAUNCHER_SOURCE_PATH}/openvibe-launcher${SCRIPT_POSTFIX}-base ${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT_NAME} @ONLY)
		INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT_NAME} DESTINATION ${DIST_ROOT})
	ENDIF()
ENDFUNCTION()

#FUNCTION(OV_CONFIGURE_RC)
#	SET(options )
#	SET(oneValueArgs NAME ICON_PATH)
#	SET(multiValueArgs )
#	CMAKE_PARSE_ARGUMENTS(OV_CONFIGURE_RC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
#	SET(GENERATED_RC_FILE "${DIST_DATADIR}/resource-files/${OV_CONFIGURE_RC_NAME}.rc")
#	IF(OV_CONFIGURE_RC_ICON_PATH)
#		SET(CONFIGURE_ICON "ID_Icon ICON DISCARDABLE \"${OV_CONFIGURE_RC_ICON_PATH}\"")
#	ENDIF()
#	IF(NOT(PROJECT_PRODUCT_NAME))
#		SET(PROJECT_PRODUCT_NAME "${OV_CONFIGURE_RC_NAME}")
#	ENDIF()
#	SET(FILE_DESCRIPTION "${PROJECT_PRODUCT_NAME} for Win32")
	
#	CONFIGURE_FILE(
#		${OV_LAUNCHER_SOURCE_PATH}/resource-file.rc-base
#		${GENERATED_RC_FILE}
#		@ONLY)
#ENDFUNCTION()
