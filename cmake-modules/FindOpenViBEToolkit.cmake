# ---------------------------------
# Finds openvibe-toolkit
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBEToolkit)

SET(INCLUDED_OV_SDK_COMPONENTS TOOLKIT)
INCLUDE(${OPENVIBE_SDK_PATH}/share/AddOpenViBESDKComponents.cmake)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBEToolkit "Yes")

