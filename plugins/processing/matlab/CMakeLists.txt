PROJECT(openvibe-plugins-matlab)

SET(PROJECT_VERSION_MAJOR ${OV_GLOBAL_VERSION_MAJOR})
SET(PROJECT_VERSION_MINOR ${OV_GLOBAL_VERSION_MINOR})
SET(PROJECT_VERSION_PATCH ${OV_GLOBAL_VERSION_PATCH})
SET(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/share/OV_stimulations.m
   DEPENDS openvibe-stimulation-generator ${OPENVIBE_SDK_PATH}/share/openvibe/toolkit/stimulation_list.txt
   COMMAND openvibe-stimulation-generator
   --matlab
   ${OPENVIBE_SDK_PATH}/share/openvibe/toolkit/stimulation_list.txt
   ${CMAKE_CURRENT_SOURCE_DIR}/share/OV_stimulations.m
   COMMENT "Generating matlab stimulation sources..." )
    
   add_custom_target(generate_matlab_stimulations_file DEPENDS share/OV_stimulations.m)


FILE(GLOB_RECURSE source_files src/*.cpp src/*.h src/*.inl)
ADD_LIBRARY(${PROJECT_NAME} SHARED ${source_files})
add_dependencies(openvibe-plugins-matlab generate_matlab_stimulations_file)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared")

INCLUDE("FindOpenViBE")
INCLUDE("FindOpenViBECommon")
INCLUDE("FindOpenViBEToolkit")
INCLUDE("FindOpenViBEModuleSystem")
INCLUDE("FindOpenViBEModuleFS")

INCLUDE("FindThirdPartyMatlab")

# ---------------------------------
# Target macros
# Defines target operating system
# Defines target architecture
# Defines target compiler
# ---------------------------------
SET_BUILD_PLATFORM()

# -----------------------------
# Install files
# -----------------------------
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

INSTALL(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/plugins/matlab)

INSTALL(DIRECTORY box-tutorials DESTINATION ${DIST_DATADIR}/openvibe/scenarios/)
