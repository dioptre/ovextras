PROJECT(test_matrix_read_write_txt)

IF(WIN32)
        ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
        ADD_DEFINITIONS(-DTARGET_OS_Windows)
        ADD_DEFINITIONS(-DTARGET_ARCHITECTURE_i386)
        ADD_DEFINITIONS(-DTARGET_COMPILER_VisualStudio)
ENDIF(WIN32)
IF(UNIX)
        # ADD_DEFINITIONS(-fvisibility=hidden) # This flag should be present... man gcc
        ADD_DEFINITIONS(-g)
        ADD_DEFINITIONS(-fnon-call-exceptions)
        ADD_DEFINITIONS(-DTARGET_OS_Linux)
        ADD_DEFINITIONS(-DTARGET_ARCHITECTURE_i386)
        ADD_DEFINITIONS(-DTARGET_COMPILER_GCC)
ENDIF(UNIX)

ADD_EXECUTABLE(${PROJECT_NAME} test_matrix_read_write_txt.cpp)

INCLUDE("FindOpenViBE")
INCLUDE("FindOpenViBEToolkit")
INCLUDE("FindOpenViBEModuleSystem")

# Unfortunately we need to install the tests as any application to find .dll/.so files
# on both Windows and Linux.
OV_INSTALL_LAUNCH_SCRIPT(${PROJECT_NAME})
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
	
