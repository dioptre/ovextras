# ---------------------------------
# Finds Neurosky ThinkGear library
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyThinkGearAPI)

IF(WIN32)
	FIND_PATH(PATH_ThinkGearAPIOld thinkgear.h PATHS "C:/Program Files/MindSet Development Tools/tgcd/win32"  ${OV_CUSTOM_DEPENDENCIES_PATH})
	IF(PATH_ThinkGearAPIOld)
		OV_PRINT(OV_PRINTED "  Found a ThinkGear API, but the version seems inferior to 2.1.")
	ENDIF(PATH_ThinkGearAPIOld)
	
	FIND_PATH(PATH_ThinkGearAPI thinkgear.h PATHS "C:/Program Files/MindSet Development Tools/ThinkGear Communications Driver/win32" "C:/Program Files (x86)/MindSet Development Tools/ThinkGear Communications Driver/win32" ${OV_CUSTOM_DEPENDENCIES_PATH})
	IF(PATH_ThinkGearAPI)
		OV_PRINT(OV_PRINTED "  Found ThinkGear API...")
		INCLUDE_DIRECTORIES(${PATH_ThinkGearAPI})
		FIND_LIBRARY(LIB_ThinkGearAPI thinkgear PATHS ${PATH_ThinkGearAPI} )
		IF(LIB_ThinkGearAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_ThinkGearAPI}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_ThinkGearAPI} )
		ELSE(LIB_ThinkGearAPI)
			OV_PRINT(OV_PRINTED "    [FAILED] lib thinkgear")
		ENDIF(LIB_ThinkGearAPI)

		# Copy the DLL file at install
		INSTALL(PROGRAMS "${PATH_ThinkGearAPI}/thinkgear.dll" DESTINATION "bin")

		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyThinkGearAPI)
	ELSE(PATH_ThinkGearAPI)
		OV_PRINT(OV_PRINTED "  FAILED to find a valid ThinkGear API - cmake looked in 'C:/Program Files/MindSet Development Tools/ThinkGear Communications Driver/win32' and 'C:/Program Files (x86)/MindSet Development Tools/ThinkGear Communications Driver/win32'")
	ENDIF(PATH_ThinkGearAPI)
ENDIF(WIN32)

IF(UNIX)
	OV_PRINT(OV_PRINTED "  Skipped ThinkGear API for Neurosky MindSet, only available on windows.")
ENDIF(UNIX)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyThinkGearAPI "Yes")

