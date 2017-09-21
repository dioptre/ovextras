# ---------------------------------
# Finds OpenViBE Designer binary distribution
# ---------------------------------


if(NOT CMAKE_BUILD_TYPE AND CMAKE_GENERATOR MATCHES "Visual Studio*")
	set(MULTI_BUILD TRUE)
elseif(CMAKE_BUILD_TYPE AND OV_PACKAGE)
	set(SOLO_PACKAGE TRUE)
elseif(CMAKE_BUILD_TYPE)
	set(SOLO_BUILD TRUE)
else()
	message(FATAL_ERROR "Build should specify a type or use a multi-type generator (like Visual Studio)")
endif()

if(NOT DEFINED TRIED_FIND_OVDESIGNER)
	if(MULTI_BUILD)
		set(SEEK_PATHS ${DESIGNER_SDK_PATH};${LIST_DEPENDENCIES_PATH})
		unset(DESIGNER_SDK_PATH CACHE)
		foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
			# set(OPENVIBE_SDK_PATH ${OPENVIBE_SDK_PATH}/$<UPPER_CASE:$<CONFIG>>)
			string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIGU )
			unset(DESIGNER_SDK_PATH_TMP CACHE)
			find_path(DESIGNER_SDK_PATH_TMP include/visualization-toolkit/ovviz_all.h PATHS ${SEEK_PATHS} PATH_SUFFIXES openvibe-designer-${OUTPUTCONFIG} ${OUTPUTCONFIG} NO_DEFAULT_PATH)
			set(DESIGNER_SDK_PATH_${OUTPUTCONFIGU} ${DESIGNER_SDK_PATH_TMP})
			if(DESIGNER_SDK_PATH_TMP)
				message(STATUS "Found ${OUTPUTCONFIG} of designer at ${DESIGNER_SDK_PATH_TMP}")
				string(CONCAT DESIGNER_SDK_PATH ${DESIGNER_SDK_PATH} $<$<CONFIG:${OUTPUTCONFIGU}>:${DESIGNER_SDK_PATH_TMP}>)
				set(AT_LEAST_ONE_DESIGNER_BUILD TRUE)
			endif()
		endforeach()
		if(NOT DEFINED AT_LEAST_ONE_DESIGNER_BUILD)
			message(FATAL_ERROR "Did not find any valid build of OpenViBE Designer")
		endif()
	else() # Regular build
		# find_path(DESIGNER_SDK_PATH bin PATHS ${DESIGNER_SDK_PATH} PATH_SUFFIXES designer NO_DEFAULT_PATH)
		find_path(DESIGNER_SDK_PATH include/visualization-toolkit/ovviz_all.h  PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES openvibe-designer-${CMAKE_BUILD_TYPE_LOWER} NO_DEFAULT_PATH)
		if(NOT DESIGNER_SDK_PATH)
			MESSAGE(ERROR "Could not find DESIGNER_SDK_PATH (value : ${OPENVIBE_SDK_PATH}). Please either specify it or put OpenViBE designer binaries into dependencies/openvibe-designer-{debug/release} folder")
		endif()
		string(REGEX REPLACE "\\\\+" "/" DESIGNER_SDK_PATH ${DESIGNER_SDK_PATH})
		message("  Found Designer... [${DESIGNER_SDK_PATH}]")
	endif()
	set(TRIED_FIND_OVDESIGNER TRUE)
endif()

if(INSTALL_DESIGNER)
	if(MULTI_BUILD) # Replace with generator expression in CMake 3.5+
		foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIGU)
			file(GLOB EXE_SCRIPT_LIST "${DESIGNER_SDK_PATH_${OUTPUTCONFIGU}}/*.cmd" "${DESIGNER_SDK_PATH_${OUTPUTCONFIGU}}/*.sh")
			if(EXE_SCRIPT_LIST)
				foreach(SCRIPT IN LISTS EXE_SCRIPT_LIST)
					get_filename_component(base_name ${SCRIPT} NAME_WE)
					if(WIN32)
						set(exe_name "${base_name}.exe")
					else()
						set(exe_name ${base_name})
					endif()
					if(WIN32)
						SET(SCRIPT_POSTFIX ".cmd")
					elseif(APPLE)
						set(SCRIPT_POSTFIX "-macos.sh")
					elseif(UNIX)
						# Debian recommends that extensions such as .sh are not used; On Linux, scripts with such extensions shouldn't be packaged
						set(SCRIPT_POSTFIX ".sh")
					endif()
					set(OV_CMD_EXECUTABLE ${exe_name})
					# IF(WIN32)
						# SET(OV_CMD_EXECUTABLE "%OV_PATH_ROOT%/bin/${exe_name}")
					# ENDIF()					
					set(SCRIPT_NAME ${base_name}${SCRIPT_POSTFIX})
					set(OV_CMD_ARGS "")
					set(OV_PAUSE "")
					if(WIN32 AND base_name STREQUAL "openvibe-designer" )
						set(PY_FIX_PREFIX "type NUL > %Temp%\\designer_error.txt")
						set(PY_FIX_REDIR "2>%Temp%\\designer_error.txt")
						set(PY_FIX_POSTFIX 
"if %errorlevel% == 1 (\n\
	type %Temp%\\designer_error.txt\n\
	for /f \"tokens=*\" %%c in (%Temp%\\designer_error.txt) do ( set errors=%%c )\n\
	if \"!errors!\" == \"SyntaxError: invalid syntax \" (\n\
		echo This is a python compatibility error. To solve this, please correct your PYTHONHOME/PYTHONPATH. Current values :\n\
		echo PYTHONHOME = %PYTHONHOME%\n\
		echo PYTHONPATH = %PYTHONPATH%\n\
		echo Now trying to launch Designer without PYTHONHOME/PYTHONPATH :\n\
		set PYTHONHOME=\n\
		set PYTHONPATH=\n\
		%OV_RUN_IN_BG% \"%OV_PATH_ROOT%\\bin\\${OV_CMD_EXECUTABLE}\" ${OV_CMD_ARGS} %ARGS%\n\
	)\n\
)\n\
DEL /Q %Temp%\\designer_error.txt")
					else()
						unset(PY_FIX_PREFIX)
						unset(PY_FIX_REDIR)
						unset(PY_FIX_POSTFIX)
					endif()
					configure_file(${OV_LAUNCHER_SOURCE_PATH}/openvibe-launcher${SCRIPT_POSTFIX}-base ${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT_NAME} @ONLY)
					install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT_NAME} DESTINATION ${DIST_ROOT})
				endforeach()
			endif()
			install(DIRECTORY ${DESIGNER_SDK_PATH_${OUTPUTCONFIGU}}/include/ DESTINATION ${DIST_INCLUDEDIR} CONFIGURATIONS ${OUTPUTCONFIG})
			install(DIRECTORY ${DESIGNER_SDK_PATH_${OUTPUTCONFIGU}}/bin/ DESTINATION ${DIST_BINDIR} CONFIGURATIONS ${OUTPUTCONFIG} USE_SOURCE_PERMISSIONS) # FILES_MATCHING PATTERN "openvibe-plugins*dll") or *so*
			install(DIRECTORY ${DESIGNER_SDK_PATH_${OUTPUTCONFIGU}}/lib/ DESTINATION ${DIST_LIBDIR} CONFIGURATIONS ${OUTPUTCONFIG}) # FILES_MATCHING PATTERN "openvibe-plugins*dll")
			install(DIRECTORY ${DESIGNER_SDK_PATH_${OUTPUTCONFIGU}}/etc/ DESTINATION ${DIST_SYSCONFDIR} CONFIGURATIONS ${OUTPUTCONFIG} OPTIONAL)
			install(DIRECTORY ${DESIGNER_SDK_PATH_${OUTPUTCONFIGU}}/share/ DESTINATION ${DIST_DATADIR} CONFIGURATIONS ${OUTPUTCONFIG})
		endforeach()
	else()
		file(GLOB EXE_SCRIPT_LIST "${DESIGNER_SDK_PATH}/*.cmd" "${DESIGNER_SDK_PATH}/*.sh")
		foreach(SCRIPT IN LISTS EXE_SCRIPT_LIST)
			get_filename_component(base_name ${SCRIPT} NAME_WE)
			if(WIN32)
				set(exe_name "${base_name}.exe")
			else()
				set(exe_name ${base_name})
			endif()
			OV_INSTALL_LAUNCH_SCRIPT(SCRIPT_PREFIX ${base_name} EXECUTABLE_NAME ${exe_name} NOPROJECT)
		endforeach()
		install(DIRECTORY ${DESIGNER_SDK_PATH}/include/ DESTINATION ${DIST_INCLUDEDIR})
		install(DIRECTORY ${DESIGNER_SDK_PATH}/bin/ DESTINATION ${DIST_BINDIR} USE_SOURCE_PERMISSIONS) # FILES_MATCHING PATTERN "openvibe-plugins*dll") or *so*
		install(DIRECTORY ${DESIGNER_SDK_PATH}/lib/ DESTINATION ${DIST_LIBDIR}) # FILES_MATCHING PATTERN "openvibe-plugins*dll")
		install(DIRECTORY ${DESIGNER_SDK_PATH}/etc/ DESTINATION ${DIST_SYSCONFDIR} OPTIONAL)
		install(DIRECTORY ${DESIGNER_SDK_PATH}/share/ DESTINATION ${DIST_DATADIR})
	endif()
endif()

