# ---------------------------------
# Finds Emotiv SDK
# Adds library to target
# Adds include path
# ---------------------------------

# Put the path to the Emotiv SDK followed by *
FILE(GLOB PATH_Candidates
        "C:/Program Files/Emotiv Development Kit*"
        "C:/Program Files/Emotiv Research Edition*"
        "C:/Program Files (x86)/Emotiv Development Kit*"
        "C:/Program Files (x86)/Emotiv Research Edition*"
        "/home/{username}/EmotivResearch_2.0.0.20*"
        #Put here the path to the directory where you install the emotiv sdk
)
FOREACH(Candidate_folder ${PATH_Candidates})
        # MESSAGE(STATUS "Found path ${PATH_Candidate}")
        #Version 1 of sdk
        LIST(APPEND PATH_Candidates ${Candidate_folder}/doc/examples/include)
        #Version 2 of sdk
        LIST(APPEND PATH_Candidates ${Candidate_folder}/doc/examples_Qt/example5)
ENDFOREACH(Candidate_folder ${PATH_Candidates})
# MESSAGE(STATUS "Emotiv paths found ${PATH_Candidates}")

FIND_PATH(PATH_EmotivAPI edk.h  PATHS ${PATH_Candidates} ${OV_CUSTOM_DEPENDENCIES_PATH})

IF(PATH_EmotivAPI)
    MESSAGE(STATUS "  Found Emotiv API...")
    INCLUDE_DIRECTORIES(${PATH_EmotivAPI})
    FIND_LIBRARY(LIB_EmotivAPI edk PATHS "${PATH_EmotivAPI}/../lib" "${PATH_EmotivAPI}/../../lib" "${PATH_EmotivAPI}/../../../lib")
    IF(LIB_EmotivAPI)
        MESSAGE(STATUS "    [  OK  ] lib ${LIB_EmotivAPI}")
        TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_EmotivAPI} )
IF(WIN32)
            # To delay load EDK.dll and redistribute binary
            TARGET_LINK_LIBRARIES(${PROJECT_NAME} Delayimp.lib )
            SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/DELAYLOAD:edk.dll")
ENDIF(WIN32)
         ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyEmotivAPI)

ELSE(LIB_EmotivAPI)
    MESSAGE(STATUS "    [FAILED] lib Emotiv edk.lib")
ENDIF(LIB_EmotivAPI)

ELSE(PATH_EmotivAPI)
        MESSAGE(STATUS "  FAILED to find Emotiv API")
ENDIF(PATH_EmotivAPI)
