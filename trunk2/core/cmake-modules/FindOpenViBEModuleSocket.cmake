# ---------------------------------
# Finds module Socket
# Adds library to target
# Adds include path
# ---------------------------------
OPTION(DYNAMIC_LINK_OPENVIBE_MODULE_SOCKET "Dynamically link OpenViBE module Socket" ON)

IF(DYNAMIC_LINK_OPENVIBE_MODULE_SOCKET)
	SET(OPENVIBE_MODULE_SOCKET_LINKING "")
ELSE(DYNAMIC_LINK_OPENVIBE_MODULE_SOCKET)
	SET(OPENVIBE_MODULE_SOCKET_LINKING "-static")
ENDIF(DYNAMIC_LINK_OPENVIBE_MODULE_SOCKET)

set(SRC_DIR ${OV_BASE_DIR}/modules/socket/)

FIND_PATH(PATH_OPENVIBE_MODULES_SOCKET include/defines.h PATHS ${SRC_DIR})
IF(PATH_OPENVIBE_MODULES_SOCKET)
	MESSAGE(STATUS "  Found OpenViBE module Socket...")
	INCLUDE_DIRECTORIES(${PATH_OPENVIBE_MODULES_SOCKET}/../)

	TARGET_LINK_LIBRARIES(${PROJECT_NAME} openvibe-module-socket${OPENVIBE_MODULE_SOCKET_LINKING})

	ADD_DEFINITIONS(-DTARGET_HAS_Socket)
ELSE(PATH_OPENVIBE_MODULES_SOCKET)
	MESSAGE(STATUS "  FAILED to find OpenViBE module Socket...")
ENDIF(PATH_OPENVIBE_MODULES_SOCKET)
