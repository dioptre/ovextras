# ---------------------------------
# Finds module XML
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBEModuleXML)

SET(INCLUDED_CERTIVIBE_COMPONENTS XML)
INCLUDE(${OPENVIBE_SDK_PATH}/share/AddCertivibeComponents.cmake)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBEModuleXML "Yes")

