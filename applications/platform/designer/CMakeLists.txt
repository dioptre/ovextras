PROJECT(openvibe-designer)

add_custom_target(${PROJECT_NAME}
        COMMAND openvibe-designer
        WORKING_DIRECTORY ${CMAKE_INSTALL_FULL_BINDIR})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(DESIGNER_SDK_PATH "${OV_CUSTOM_DEPENDENCIES_PATH}/openvibe-designer-debug" CACHE STRING "OpenViBE Designer Path")
else()
        set(DESIGNER_SDK_PATH "${OV_CUSTOM_DEPENDENCIES_PATH}/openvibe-designer-release" CACHE STRING "OpenViBE Designer Path")
endif()

set(PATH_DESIGNER "PATH_DESIGNER-NOTFOUND")
find_path(PATH_DESIGNER include/visualization-toolkit/ovviz_all.h PATHS ${DESIGNER_SDK_PATH} NO_DEFAULT_PATH)

if(${PATH_DESIGNER} STREQUAL "PATH_DESIGNER-NOTFOUND")
        message(FATAL_ERROR "  FAILED to find Designer [${DESIGNER_SDK_PATH}]")
endif()

message( "  Found Designer... [${PATH_DESIGNER}]")

OV_INSTALL_LAUNCH_SCRIPT(SCRIPT_PREFIX "openvibe-designer" EXECUTABLE_NAME  "${PROJECT_NAME}" ICON_PATH "${OV_ICON_PATH}/designer.ico")

install(DIRECTORY ${PATH_DESIGNER}/include/ DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
install(DIRECTORY ${PATH_DESIGNER}/share/openvibe/applications/designer DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/openvibe/applications)

SET(INCLUDED_CERTIVIBE_COMPONENTS DEPENDENCIES)

SET(CMAKE_MODULE_PATH "${OPENVIBE_SDK_PATH}/share")
INCLUDE("AddCertivibeComponents")

file(COPY ${PATH_DESIGNER}/doc-tmp/ DESTINATION ${DOCUMENTATION_TEMP_DIRECTORY})

if(WIN32)
        install(FILES
	        ${PATH_DESIGNER}/bin/openvibe-designer.exe
		${PATH_DESIGNER}/bin/openvibe-plugins-simple-visualization.dll
		${PATH_DESIGNER}/bin/openvibe-plugins-mensia-advanced-visualization.dll
		${PATH_DESIGNER}/bin/openvibe-visualization-toolkit.dll
                DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
        install(DIRECTORY
                ${OPENVIBE_SDK_PATH}/bin/
                DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} FILES_MATCHING PATTERN "certivibe-plugins*dll")
        install(DIRECTORY
	        ${PATH_DESIGNER}/bin/
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
	        DIRECTORY ${PATH_DESIGNER}/lib/gtk-2.0/2.10.0/engines
                DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/gtk-2.0/2.10.0/
                FILES_MATCHING PATTERN "*.dll")
        install(
	        FILES ${PATH_DESIGNER}/etc/gtk-2.0/gtkrc
                DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/gtk-2.0/)
elseif(UNIX)
        install(DIRECTORY
	        ${PATH_DESIGNER}/bin/
                DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
                USE_SOURCE_PERMISSIONS
                PATTERN "openvibe-designer")
        install(DIRECTORY
	        ${PATH_DESIGNER}/lib/
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
        ${PATH_DESIGNER}/share/
	DESTINATION ${CMAKE_INSTALL_FULL_DATADIR})

