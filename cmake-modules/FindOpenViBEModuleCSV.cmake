# ---------------------------------
# Finds module CSV
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBEModuleCSV)

SET(INCLUDED_OV_SDK_COMPONENTS CSV)
INCLUDE(AddOpenViBESDKComponents)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBEModuleCSV "Yes")

