# ---------------------------------
# Finds Emotiv SDK
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyEmotivAPI)

# Put the path to the Emotiv SDK followed by *
FILE(GLOB PATH_Candidates
        "C:/Program Files/Emotiv Development Kit*"
        "C:/Program Files/Emotiv Research Edition*"	
        "C:/Program Files/Emotiv SDK Premium Edition*"			
        "C:/Program Files (x86)/Emotiv Development Kit*"
        "C:/Program Files (x86)/Emotiv Research Edition*"	
        "C:/Program Files (x86)/Emotiv SDK Premium Edition*"		
        "/home/{username}/EmotivResearch_2.0.0.20*"
        #Put here the path to the directory where you installed the emotiv sdk
)
FOREACH(Candidate_folder ${PATH_Candidates})
        # OV_PRINT(OV_PRINTED "Found path ${PATH_Candidate}")
        #Version 1 of sdk
        LIST(APPEND PATH_Candidates ${Candidate_folder}/doc/examples/include)
        #Version 2 of sdk
        LIST(APPEND PATH_Candidates ${Candidate_folder}/doc/examples_Qt/example5)
        #Version 3 of sdk
        LIST(APPEND PATH_Candidates ${Candidate_folder}/EDK/Header\ files/)
ENDFOREACH(Candidate_folder ${PATH_Candidates})

SET(PATH_EmotivAPI "-NOTFOUND")
FIND_PATH(PATH_EmotivAPI1 edk.h PATHS ${PATH_Candidates} ${OV_CUSTOM_DEPENDENCIES_PATH})
IF(PATH_EmotivAPI1)
    OV_PRINT(OV_PRINTED "  Found Emotiv Research API 1.x ...")
	SET(OV_EMOTIV_VERSION "research-1")
	SET(PATH_EmotivAPI ${PATH_EmotivAPI1})
	SET(OV_EMOTIV_PATHS "${PATH_EmotivAPI}/../lib" "${PATH_EmotivAPI}/../../lib" "${PATH_EmotivAPI}/../../../lib")
ENDIF(PATH_EmotivAPI1)
	
FIND_PATH(PATH_EmotivAPI2 IEdk.h PATHS ${PATH_Candidates} ${OV_CUSTOM_DEPENDENCIES_PATH})
IF(PATH_EmotivAPI2)
    OV_PRINT(OV_PRINTED "  Found Emotiv API 3.x ...")
	SET(OV_EMOTIV_VERSION "community")
	SET(PATH_EmotivAPI ${PATH_EmotivAPI2})	
	SET(OV_EMOTIV_PATHS "${PATH_EmotivAPI}/../x86" "${PATH_EmotivAPI}/../../x86" "${PATH_EmotivAPI}/../../../x86")	
ENDIF(PATH_EmotivAPI2)
	
FIND_PATH(PATH_EmotivAPI3 IEegData.h PATHS ${PATH_Candidates} ${OV_CUSTOM_DEPENDENCIES_PATH})
IF(PATH_EmotivAPI3)
    OV_PRINT(OV_PRINTED "  Found Emotiv Research API 3.x ...")
	SET(OV_EMOTIV_VERSION "research-3")
	SET(PATH_EmotivAPI ${PATH_EmotivAPI3})		
	SET(OV_EMOTIV_PATHS "${PATH_EmotivAPI}/../x86" "${PATH_EmotivAPI}/../../x86" "${PATH_EmotivAPI}/../../../x86")
ENDIF(PATH_EmotivAPI3)

IF(PATH_EmotivAPI)
    FIND_LIBRARY(LIB_EmotivAPI edk PATHS ${OV_EMOTIV_PATHS})
    IF(LIB_EmotivAPI)
        OV_PRINT(OV_PRINTED "    [  OK  ] api ${PATH_EmotivAPI}")
        OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_EmotivAPI}")
		INCLUDE_DIRECTORIES(${PATH_EmotivAPI})
        TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_EmotivAPI} )
IF(WIN32)
            # To delay load EDK.dll and redistribute binary
            TARGET_LINK_LIBRARIES(${PROJECT_NAME} Delayimp.lib )
            SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/DELAYLOAD:edk.dll /DELAY:UNLOAD")
ENDIF(WIN32)
		IF(${OV_EMOTIV_VERSION} STREQUAL "community")
			# This doesn't work by itself yet...
			ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEmotivCommunityAPI)
		ELSEIF(${OV_EMOTIV_VERSION} STREQUAL "research-1")	
			ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEmotivAPI)		
			ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEmotivResearchAPI1x)
		ELSEIF(${OV_EMOTIV_VERSION} STREQUAL "research-3")
			ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEmotivAPI)
			ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEmotivResearchAPI3x)		
		ENDIF(${OV_EMOTIV_VERSION} STREQUAL "community")
ELSE(LIB_EmotivAPI)
    OV_PRINT(OV_PRINTED "    [FAILED] lib Emotiv edk.lib")
ENDIF(LIB_EmotivAPI)

ELSE(PATH_EmotivAPI)
        OV_PRINT(OV_PRINTED "  FAILED to find Emotiv API (optional)")
ENDIF(PATH_EmotivAPI)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyEmotivAPI "Yes")

