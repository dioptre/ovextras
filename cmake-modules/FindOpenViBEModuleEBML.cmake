# ---------------------------------
# Finds module EBML
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBEModuleEBML)

SET(INCLUDED_OV_SDK_COMPONENTS EBML)
INCLUDE(AddOpenViBESDKComponents)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBEModuleEBML "Yes")

