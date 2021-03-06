# Author Yann Renard / INRIA
# Date 2008-10-15
#
# this CMake script iterates over several source documentation directories in
# order to compile it with doxygen. It has the ability to configure the
# doxyfile depending on some variables and to build documentation sources from
# computer generated templates (.dox-skeleton) and hand written documentation
# parts (.dox-part)

ADD_CUSTOM_TARGET(${PROJECT_NAME} ALL)

# look for doxygen, if not present, no need to generate documentation
FIND_PROGRAM(doxygen_bin "doxygen" PATHS ${LIST_DEPENDENCIES_PATH}/bin NO_DEFAULT_PATH)
FIND_PROGRAM(doxygen_bin "doxygen" PATHS ${LIST_DEPENDENCIES_PATH}/bin "C:/Program Files/doxygen/bin")

IF(doxygen_bin)

	MESSAGE(STATUS "  Found doxygen...")

	# intializes the variable that will be used in the doxyfile for input
	# directories
	STRING(REPLACE "\\" "/" ov_doxy_input "${OV_BASE_DIR}/cmake-modules")
	SET(ov_doxy_input "\\\"${ov_doxy_input}\\\"")

	# intializes the variable that will contain the list of resource files to
	# copy to the target directory
	SET(resource_files "")

	SET(DOX_PART_FILES "")
	# iterates on each project we have
	#
	# for each project, we look at its resources and store them in a list
	# for each project, we look at partial documentation files (.dox-part) and
	# parse them to get |ov[a-zA-Z0-9_]*_begin| or |ov[a-zA-Z0-9_]*_end|
	# tokens. This tokens will later be included in the skeleton doxumentation
	# files (.dox-skeleton)
	GET_PROPERTY(CURRENT_PROJECTS GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS)
	
	FOREACH(current_project ${CURRENT_PROJECTS})
		# MESSAGE(STATUS "    [  OK  ] Project ${current_project}")
		
		STRING(REGEX REPLACE " +$" "" current_project ${current_project})

		# updates the doxyfile variable for input directories
		IF(EXISTS "${current_project}/include")
			SET(current_project_include "${current_project}/include")
			SET(ov_doxy_input "${ov_doxy_input} \\\"${current_project_include}\\\"")
		ENDIF(EXISTS "${current_project}/include")

		IF(EXISTS "${current_project}/src")
			# MESSAGE(STATUS "    [  OK  ]   Candidate src directory found ${current_project_src}")
			SET(current_project_src "${current_project}/src")
			SET(ov_doxy_input "${ov_doxy_input} \\\"${current_project_src}\\\"")
		ENDIF(EXISTS "${current_project}/src")

		IF(EXISTS "${current_project}/doc")
			# MESSAGE(STATUS "    [  OK  ]   Candidate doc directory found ${current_project_doc}")
			SET(current_project_doc "${current_project}/doc")
			SET(ov_doxy_input "${ov_doxy_input} \\\"${current_project_doc}\\\"")

			# looks for resources and stores them in a list
			FILE(GLOB_RECURSE resource_files_tmp "${current_project_doc}/*.png" "${current_project_doc}/*.svg" "${current_project_doc}/*.css" "${current_project_doc}/*.php")
			SET(resource_files ${resource_files} ${resource_files_tmp})

			# looks for partial hand written documentation
			FILE(GLOB_RECURSE doxs "${current_project_doc}/*.dox-part")
			SET(DOX_PART_FILES "${DOX_PART_FILES};${doxs}")
		ENDIF(EXISTS "${current_project}/doc")
		
		IF(EXISTS "${current_project}/box-algorithm-doc")
		
			LIST(APPEND dir_list "." "box-algorithm-doc/dox-part")
			FOREACH(file_path ${dir_list})						
				SET(current_project_doc "${current_project}/${file_path}")
				MESSAGE(STATUS "    [  OK ]   Candidate doc directory found ${current_project_doc}")
				SET(ov_doxy_input "${ov_doxy_input} \\\"${current_project_doc}\\\"")
	
				# looks for resources and stores them in a list
				FILE(GLOB_RECURSE resource_files_tmp "${current_project_doc}/*.png" "${current_project_doc}/*.svg" "${current_project_doc}/*.css" "${current_project_doc}/*.php")
				SET(resource_files ${resource_files} ${resource_files_tmp})
	
				# looks for partial hand written documentation
				FILE(GLOB_RECURSE doxs "${current_project_doc}/*.dox-part")
				SET(DOX_PART_FILES "${DOX_PART_FILES};${doxs}")
			ENDFOREACH(file_path ${dir_list})
			
		ENDIF(EXISTS "${current_project}/box-algorithm-doc")
		
		
		
	ENDFOREACH(current_project)
	
	# the final doxyfile filename is generated, platform compliantly
	SET(ov_doxy_final "${CMAKE_CURRENT_BINARY_DIR}/doxyfile")
	IF(WIN32)
		STRING(REPLACE "/" "\\\\" ov_doxy_final ${ov_doxy_final})
	ENDIF(WIN32)
	
	# these two lines configure the variables used to configure the doxyfile
	SET(ov_doxy_input "${ov_doxy_input} \\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"")
	SET(ov_doxy_input "${ov_doxy_input} \\\"${CMAKE_CURRENT_BINARY_DIR}\\\"")
	SET(ov_doxy_strip_from_path ${ov_doxy_input})
	SET(ov_doxy_version ${PROJECT_VERSION})

	# then the doxyfile is configured

	GET_PROPERTY(CURRENT_PROJECTS GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS_BUILD_DIR)
	FOREACH(current_project ${CURRENT_PROJECTS})
#		MESSAGE(STATUS "DUH ${current_project}")
		STRING(REGEX REPLACE " +$" "" current_project ${current_project})
		SET(current_project_src "${current_project}/src")
		# updates the doxyfile variable for input directories
		SET(ov_plugin_inspector_load_path "${ov_plugin_inspector_load_path}:${current_project}")
	ENDFOREACH(current_project)
	
	# create folder to put the output from doxygen to
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../doc")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../doc/html")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/algorithm-doc")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/algorithm-snapshots")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/box-algorithm-doc")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/box-algorithm-snapshots")
	# then the doxyfile is configured
	
	CONFIGURE_FILE(
		CreateDoxygen.cmake-skeleton
		CreateDoxygen.cmake
		@ONLY)

	CONFIGURE_FILE(
		openvibe.dox-base
		openvibe.dox
		@ONLY)	
		
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET ${PROJECT_NAME}
		POST_BUILD
                COMMAND ${LIST_DEPENDENCIES_PATH}/cmake/bin/cmake.exe -P CreateDoxygen.cmake
		WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)
ELSEIF(UNIX)
        ADD_CUSTOM_COMMAND(
                TARGET ${PROJECT_NAME}
                POST_BUILD
                COMMAND cmake -P CreateDoxygen.cmake
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )
ENDIF(WIN32)
	
INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../doc/" DESTINATION ${DIST_DOCDIR})

ELSE(doxygen_bin)

	MESSAGE(STATUS "  FAILED to find doxygen...")

ENDIF(doxygen_bin)


