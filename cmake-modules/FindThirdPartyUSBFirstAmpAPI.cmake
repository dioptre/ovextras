# ---------------------------------
# Finds VAmp FirstAmp library
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyUSBFirstAmpAPI)

IF(WIN32)
	FIND_PATH(PATH_USBFirstAmpAPI FirstAmp.h PATHS "C:/Program Files/FaSDK" "C:/Program Files (x86)/FaSDK" ${OV_CUSTOM_DEPENDENCIES_PATH})
	IF(PATH_USBFirstAmpAPI)
		OV_PRINT(OV_PRINTED "  Found FirstAmp API...")
		INCLUDE_DIRECTORIES(${PATH_USBFirstAmpAPI})
		FIND_LIBRARY(LIB_USBFirstAmpAPI FirstAmp PATHS ${PATH_USBFirstAmpAPI} )
		IF(LIB_USBFirstAmpAPI)
			OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_USBFirstAmpAPI}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_USBFirstAmpAPI} )
		ELSE(LIB_USBFirstAmpAPI)
			OV_PRINT(OV_PRINTED "    [FAILED] lib FirstAmp")
		ENDIF(LIB_USBFirstAmpAPI)

		# Copy the DLL file at install
		INSTALL(PROGRAMS "${PATH_USBFirstAmpAPI}/FirstAmp.dll" DESTINATION "bin")
		
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyUSBFirstAmpAPI)
	ELSE(PATH_USBFirstAmpAPI)
		OV_PRINT(OV_PRINTED "  FAILED to find FirstAmp API - cmake looked in 'C:/Program Files/FaSDK' and 'C:/Program Files (x86)/FaSDK'")
	ENDIF(PATH_USBFirstAmpAPI)
ENDIF(WIN32)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyUSBFirstAmpAPI "Yes")

