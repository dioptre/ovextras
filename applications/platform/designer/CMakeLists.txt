PROJECT(openvibe-designer)

add_custom_target(${PROJECT_NAME}
        COMMAND openvibe-designer
        WORKING_DIRECTORY ${CMAKE_INSTALL_FULL_BINDIR})

# Path to designer
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	find_path(DESIGNER_SDK_PATH bin PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES openvibe-designer-debug NO_DEFAULT_PATH)
else()
	find_path(DESIGNER_SDK_PATH bin PATHS ${LIST_DEPENDENCIES_PATH} PATH_SUFFIXES openvibe-designer-release NO_DEFAULT_PATH)
endif()
if(NOT DESIGNER_SDK_PATH)
	MESSAGE(FATAL_ERROR "Could not find DESIGNER_SDK_PATH (DESIGNER_SDK_PATH). Please either specify it or put OpenViBE designer binaries into dependencies/openvibe-designer-{debug/release} folder")
endif()

message( "  Found Designer... [${DESIGNER_SDK_PATH}]")

OV_INSTALL_LAUNCH_SCRIPT(SCRIPT_PREFIX "openvibe-designer" EXECUTABLE_NAME  "${PROJECT_NAME}" ICON_PATH "${OV_ICON_PATH}/designer.ico")

install(DIRECTORY ${DESIGNER_SDK_PATH}/include/ DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
install(DIRECTORY ${DESIGNER_SDK_PATH}/share/openvibe/applications/designer DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/openvibe/applications)

SET(INCLUDED_CERTIVIBE_COMPONENTS DEPENDENCIES)

SET(CMAKE_MODULE_PATH "${OPENVIBE_SDK_PATH}/share")
INCLUDE("AddCertivibeComponents")

file(COPY ${DESIGNER_SDK_PATH}/doc-tmp/ DESTINATION ${DOCUMENTATION_TEMP_DIRECTORY})

if(WIN32)
        install(FILES
	        ${DESIGNER_SDK_PATH}/bin/openvibe-designer.exe
			${DESIGNER_SDK_PATH}/bin/openvibe-plugins-simple-visualization.dll
			${DESIGNER_SDK_PATH}/bin/openvibe-plugins-mensia-advanced-visualization.dll
			${DESIGNER_SDK_PATH}/bin/openvibe-visualization-toolkit.dll
                DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
        install(DIRECTORY
                ${OPENVIBE_SDK_PATH}/bin/
                DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} FILES_MATCHING PATTERN "certivibe-plugins*dll")
        install(DIRECTORY
			${DESIGNER_SDK_PATH}/bin/
                DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
                FILES_MATCHING
                PATTERN "freetype*.dll"
                PATTERN "*intl*.dll"
                PATTERN "*asfprintf*.dll"
                PATTERN "*cairo*.dll"
                PATTERN "*fontconfig*.dll"
                PATTERN "*gailutil*.dll"
                PATTERN "*gcc*.dll"
                PATTERN "*gdk*.dll"
                PATTERN "*gio*.dll"
                PATTERN "*glib*.dll"
                PATTERN "*gmodule*.dll"
                PATTERN "*gobject*.dll"
                PATTERN "*gthread*.dll"
                PATTERN "*gtk*.dll"
                PATTERN "*pango*.dll"
                PATTERN "*png*.dll"
                PATTERN "*expat-1*.dll"
                PATTERN "*zlib*.dll"
                PATTERN "*libatk*.dll"
                )
        install(
	        DIRECTORY ${DESIGNER_SDK_PATH}/lib/gtk-2.0/2.10.0/engines
                DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/gtk-2.0/2.10.0/
                FILES_MATCHING PATTERN "*.dll")
        install(
	        FILES ${DESIGNER_SDK_PATH}/etc/gtk-2.0/gtkrc
                DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/gtk-2.0/)
elseif(UNIX)
        install(DIRECTORY
	        ${DESIGNER_SDK_PATH}/bin/
                DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
                USE_SOURCE_PERMISSIONS
                PATTERN "openvibe-designer")
        install(DIRECTORY
	        ${DESIGNER_SDK_PATH}/lib/
                DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
                FILES_MATCHING
                PATTERN "libopenvibe-plugins-simple-visualization*so*"
                PATTERN "libopenvibe-plugins-mensia-advanced-visualization*so*"
                PATTERN "libopenvibe-visualization-toolkit*so*")
        install(DIRECTORY
                ${OPENVIBE_SDK_PATH}/lib/
                DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} FILES_MATCHING PATTERN "libcertivibe-plugins*so*")

endif()

install(DIRECTORY
        ${DESIGNER_SDK_PATH}/share/
	DESTINATION ${CMAKE_INSTALL_FULL_DATADIR})