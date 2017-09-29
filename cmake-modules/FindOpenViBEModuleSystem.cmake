# ---------------------------------
# Finds module System
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBEModuleSystem)

SET(INCLUDED_OV_SDK_COMPONENTS SYSTEM)
INCLUDE(AddOpenViBESDKComponents)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBEModuleSystem "Yes")

