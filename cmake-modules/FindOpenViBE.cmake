# ---------------------------------
# Finds OpenViBE
# Adds library to target
# Adds include path
# ---------------------------------
GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBE)

SET(INCLUDED_CERTIVIBE_COMPONENTS MAIN)
INCLUDE(${OPENVIBE_SDK_PATH}/share/AddCertivibeComponents.cmake)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBE "Yes")

