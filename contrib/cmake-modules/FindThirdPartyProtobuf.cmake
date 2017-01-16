# ---------------------------------
# Finds Google Protocol Buffers Library
#
# Sets PROTOBUF_FOUND
# Sets PROTOBUF_INCLUDE_DIRS
# Sets PROTOBUF_LIBRARIES
# ---------------------------------

IF(UNIX)
	INCLUDE(FindProtobuf)
	FIND_PACKAGE(Protobuf)
	
	IF(PROTOBUF_FOUND)
		MESSAGE(STATUS "  Found Protobuf...")

		INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROTOBUF_LIBRARIES})
		ADD_DEFINITIONS(-DTARGET_HAS_Protobuf)
	ELSE(PROTOBUF_FOUND)
		MESSAGE(STATUS "   Failed to find Protobuf... (optional, for muse reader box)")
	ENDIF(PROTOBUF_FOUND)
ENDIF(UNIX)

IF(WIN32)
	MESSAGE(STATUS "  FAILED to find Protobuf (optional, for muse reader box)")
ENDIF(WIN32)
