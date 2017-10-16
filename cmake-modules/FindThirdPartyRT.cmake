# ---------------------------------
# Finds third party rt
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyRT)

IF(UNIX)
	# For Ubuntu 13.04 (interprocess/ipc/message_queue.hpp in ovasCPluginExternalStimulations.cpp caused dep)
	FIND_LIBRARY(LIB_STANDARD_MODULE_RT rt)
	IF(LIB_STANDARD_MODULE_RT)
		OV_PRINT(OV_PRINTED "  Found rt...")
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_STANDARD_MODULE_RT})
	ELSE(LIB_STANDARD_MODULE_RT)
		OV_PRINT(OV_PRINTED "  FAILED to find rt...")
	ENDIF(LIB_STANDARD_MODULE_RT)
	
ENDIF(UNIX)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyBoost_RT "Yes")

