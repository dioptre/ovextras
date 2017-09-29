# ---------------------------------
# Finds module XML
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBEModuleXML)

SET(INCLUDED_OV_SDK_COMPONENTS XML)
INCLUDE(AddOpenViBESDKComponents)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBEModuleXML "Yes")

