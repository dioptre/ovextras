# ---------------------------------
# Finds openvibe-toolkit
# Adds library to target
# Adds include path
# ---------------------------------
option(LINK_OPENVIBE_VISUALIZATION_TOOLKIT "By default, link openvibe-visualization-toolkit, otherwise only use the includes" ON)
option(DYNAMIC_LINK_OPENVIBE_VISUALIZATION_TOOLKIT "Dynamically link openvibe-visualization-toolkit" ON)

if(DYNAMIC_LINK_OPENVIBE_VISUALIZATION_TOOLKIT)
	set(OPENVIBE_VISUALIZATION_TOOLKIT_LINKING "")
	add_definitions(-DOVVIZ_Shared)
else()
	set(OPENVIBE_VISUALIZATION_TOOLKIT_LINKING "-static")
	add_definitions(-DOVVIZ_Static)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(DESIGNER_SDK_PATH "${OV_CUSTOM_DEPENDENCIES_PATH}/openvibe-designer-debug" CACHE STRING "OpenViBE Designer Path")
else()
        set(DESIGNER_SDK_PATH "${OV_CUSTOM_DEPENDENCIES_PATH}/openvibe-designer-release" CACHE STRING "OpenViBE Designer Path")
endif()

set(PATH_OPENVIBE_VISUALIZATION_TOOLKIT "PATH_OPENVIBE_VISUALIZATION_TOOLKIT-NOTFOUND")
find_path(PATH_OPENVIBE_VISUALIZATION_TOOLKIT visualization-toolkit/ovviz_all.h PATHS ${DESIGNER_SDK_PATH}/include/ NO_DEFAULT_PATH)
if(PATH_OPENVIBE_VISUALIZATION_TOOLKIT)
        message( "  Found openvibe-visualization-toolkit...  ${PATH_OPENVIBE_VISUALIZATION_TOOLKIT}")
	include_directories(${PATH_OPENVIBE_VISUALIZATION_TOOLKIT}/)
		
	if(LINK_OPENVIBE_VISUALIZATION_TOOLKIT)
	        find_library(VISUALIZATION_TOOLKIT_LIBRARY openvibe-visualization-toolkit${OPENVIBE_VISUALIZATION_TOOLKIT_LINKING} PATHS ${DESIGNER_SDK_PATH}/lib NO_DEFAULT_PATH)
		target_link_libraries(${PROJECT_NAME} ${VISUALIZATION_TOOLKIT_LIBRARY})
	endif()

	add_definitions(-DTARGET_HAS_OpenViBEVisualizationToolkit)
else()
	message(WARNING "  FAILED to find openvibe-visualization-toolkit...")
endif()
