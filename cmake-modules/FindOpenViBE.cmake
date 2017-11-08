# ---------------------------------
# Finds OpenViBE
# Adds library to target
# Adds include path
# ---------------------------------
GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBE)

SET(INCLUDED_OV_SDK_COMPONENTS MAIN)
INCLUDE(AddOpenViBESDKComponents)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBE "Yes")

