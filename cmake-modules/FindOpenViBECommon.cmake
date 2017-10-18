# ---------------------------------
# Finds OpenViBE common include files
# Adds dependency to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBECommon)

SET(INCLUDED_OV_SDK_COMPONENTS COMMON)
INCLUDE(AddOpenViBESDKComponents)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBECommon "Yes")

