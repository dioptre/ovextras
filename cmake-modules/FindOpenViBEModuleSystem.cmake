# ---------------------------------
# Finds module System
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBEModuleSystem)

SET(INCLUDED_CERTIVIBE_COMPONENTS SYSTEM)
INCLUDE(${OPENVIBE_SDK_PATH}/share/AddCertivibeComponents.cmake)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBEModuleSystem "Yes")

