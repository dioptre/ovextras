PROJECT(test_tagstream)

IF(WIN32)
        ADD_DEFINITIONS(-DTARGET_OS_Windows)
ENDIF(WIN32)
IF(UNIX)
        ADD_DEFINITIONS(-DTARGET_OS_Linux)
ENDIF(UNIX)

INCLUDE_DIRECTORIES(../)
ADD_EXECUTABLE(${PROJECT_NAME} test_tagstream.cpp ../ovasCTagStream.cpp)

INCLUDE("FindOpenViBE")
INCLUDE("FindThirdPartyBoost")
INCLUDE("FindThirdPartyBoost_System")
INCLUDE("FindThirdPartyBoost_Thread")

# Unfortunately we need to install the tests as any application to find .dll/.so files
# on both Windows and Linux.
OV_INSTALL_LAUNCH_SCRIPT(SCRIPT_PREFIX "${PROJECT_NAME}" EXECUTABLE_NAME  "${PROJECT_NAME}")
INSTALL(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})

