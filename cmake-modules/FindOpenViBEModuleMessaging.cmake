# ---------------------------------
# Finds module Messaging
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_OpenViBEModuleMessaging)

SET(INCLUDED_OV_SDK_COMPONENTS MESSAGING)
INCLUDE(AddOpenViBESDKComponents)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_OpenViBEModuleMessaging "Yes")

