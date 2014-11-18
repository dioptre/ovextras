project(test_ebml)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src)
LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH})

INCLUDE_DIRECTORIES(../src)
ADD_EXECUTABLE(${PROJECT_NAME} test_ebml_write.cpp)

INCLUDE("FindOpenViBEModuleEBML")

ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)

OV_INSTALL_LAUNCH_SCRIPT(${PROJECT_NAME})
INSTALL(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
#ADD_EXECUTABLE(test_ebml_write-dynamic test_ebml_write.cpp)
#ADD_EXECUTABLE(test_ebml_write-static test_ebml_write.cpp)
#ADD_EXECUTABLE(test_ebml_read-dynamic test_ebml_read.cpp)
#ADD_EXECUTABLE(test_ebml_read-static test_ebml_read.cpp)
#ADD_EXECUTABLE(test_ebml_tree-dynamic test_ebml_tree.cpp)
#ADD_EXECUTABLE(test_ebml_tree-static test_ebml_tree.cpp)
#TARGET_LINK_LIBRARIES(test_ebml_write-dynamic ${PROJECT_NAME})
#TARGET_LINK_LIBRARIES(test_ebml_write-static ${PROJECT_NAME}-static)
#TARGET_LINK_LIBRARIES(test_ebml_read-dynamic ${PROJECT_NAME})
#TARGET_LINK_LIBRARIES(test_ebml_read-static ${PROJECT_NAME}-static)
#TARGET_LINK_LIBRARIES(test_ebml_tree-dynamic ${PROJECT_NAME})
#TARGET_LINK_LIBRARIES(test_ebml_tree-static ${PROJECT_NAME}-static)
