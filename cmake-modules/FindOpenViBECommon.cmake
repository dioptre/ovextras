# ---------------------------------
# Finds OpenViBE common include files
# Adds dependency to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBECommon)

SET(INCLUDED_CERTIVIBE_COMPONENTS COMMON)
INCLUDE(${OPENVIBE_SDK_PATH}/share/AddCertivibeComponents.cmake)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBECommon "Yes")

