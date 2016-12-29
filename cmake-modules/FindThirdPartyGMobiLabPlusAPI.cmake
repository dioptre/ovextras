#
# The gMobilab driver (Linux) was contributed by Lucie Daubigney from Supelec Metz
#
# Windows-compatibility added by Jussi T. Lindgren / Inria
#

# ---------------------------------
# Finds GTecMobiLabPlus+
# Adds library to target
# Adds include path
# ---------------------------------

GET_PROPERTY(OV_PRINTED GLOBAL PROPERTY OV_TRIED_ThirdPartyGMobiLabPlusAPI)

IF(WIN32)
	# note that the API must be 32bit with OpenViBE
	FIND_PATH(PATH_GMobiLabCAPI GMobiLabPlus.h PATHS 
		"C:/Program Files/gtec/GMobiLabCAPI/Lib" 
		"C:/Program Files (x86)/gtec/GMobiLabCAPI/Lib" 
		${OV_CUSTOM_DEPENDENCIES_PATH})
	# We need to copy the DLL on install
	FIND_PATH(PATH_GMobiLabDLL gMOBIlabplus.dll PATHS 
		"C:/Windows/System32" 
		"C:/Windows/SysWOW64" 
		${OV_CUSTOM_DEPENDENCIES_PATH})		
	FIND_LIBRARY(LIB_GMobiLabCAPI GMobiLabplus PATHS ${PATH_GMobiLabCAPI}/x86)
	
	IF(PATH_GMobiLabCAPI AND PATH_GMobiLabDLL AND LIB_GMobiLabCAPI)
		OV_PRINT(OV_PRINTED "  Found GMobiLabCAPI ...")
		OV_PRINT(OV_PRINTED "    [  OK  ] lib ${LIB_GMobiLabCAPI}")
			
		INCLUDE_DIRECTORIES(${PATH_GMobiLabCAPI})
			
		# Do not link to the dll! Its opened runtime with dlopen()
		# TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_GMobiLabCAPI} )
			
		INSTALL(PROGRAMS ${PATH_GMobiLabDLL}/gMOBIlabplus.dll DESTINATION "bin")
		
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyGMobiLabPlusAPI)
		SET(OV_ThirdPartyGMobilab "YES")

	ELSE(PATH_GMobiLabCAPI AND PATH_GMobiLabDLL AND LIB_GMobiLabCAPI)
		OV_PRINT(OV_PRINTED "  FAILED to find GMobiLabPlusAPI + lib + dll")
		OV_PRINT(OV_PRINTED "    Results were ${PATH_GMobiLabCAPI} AND ${PATH_GMobiLabDLL} AND ${LIB_GMobiLabCAPI}")
	ENDIF(PATH_GMobiLabCAPI AND PATH_GMobiLabDLL AND LIB_GMobiLabCAPI)
ENDIF(WIN32)

IF(UNIX)
	FIND_LIBRARY(gMOBIlabplus_LIBRARY NAMES "gMOBIlabplus" "gmobilabplusapi" PATHS "/usr/lib" "/usr/local/lib")
	IF(gMOBIlabplus_LIBRARY)
		OV_PRINT(OV_PRINTED "  Found GMobiLabPlusAPI...")
		OV_PRINT(OV_PRINTED "    [  OK  ] Third party lib ${gMOBIlabplus_LIBRARY}")
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyGMobiLabPlusAPI)
		# Do not link to the dll! Its opened runtime with dlopen()
		# TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${gMOBIlabplus_LIBRARY} )
	ELSE(gMOBIlabplus_LIBRARY)
		OV_PRINT(OV_PRINTED "  FAILED to find GMobiLabPlusAPI... (optional)")
		OV_PRINT(OV_PRINTED "   : If it should be found, see that 'gmobilabapi.so' link exists on the fs, with no numeric suffixes in the filename.")
		OV_PRINT(OV_PRINTED "   : e.g. do 'cd /usr/lib/ ; ln -s libgmobilabplusapi.so.1.12 libgmobilabplusapi.so' ")
	ENDIF(gMOBIlabplus_LIBRARY)
ENDIF(UNIX)

SET_PROPERTY(GLOBAL PROPERTY OV_TRIED_ThirdPartyGMobiLabPlusAPI "Yes")

