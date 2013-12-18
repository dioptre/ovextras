 #
 # The gMobilab driver was contributed
 # by Lucie Daubigney from Supelec Metz
 #

# ---------------------------------
# Finds GTecMobiLabPlus+
# Adds library to target
# Adds include path
# ---------------------------------

IF(WIN32)
	MESSAGE(STATUS "  Skipping MobiLabPlus, not supported on Windows")
ENDIF(WIN32)

IF(UNIX)
	FIND_LIBRARY(gMOBIlabplus_LIBRARY NAMES "gMOBIlabplus" "gmobilabplusapi" PATHS "/usr/lib" "/usr/local/lib")
	IF(gMOBIlabplus_LIBRARY)
		MESSAGE(STATUS "  Found GMobiLabPlusAPI...")
		MESSAGE(STATUS "    [  OK  ] Third party lib ${gMOBIlabplus_LIBRARY}")
		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyGMobiLabPlusAPI)
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${gMOBIlabplus_LIBRARY} )
	ELSE(gMOBIlabplus_LIBRARY)
		MESSAGE(STATUS "  FAILED to find GMobiLabPlusAPI... ")
		MESSAGE(STATUS "   : If it should be found, see that 'gmobilabapi.so' link exists on the fs, with no numeric suffixes in the filename.")
		MESSAGE(STATUS "   : e.g. do 'cd /usr/lib/ ; ln -s libgmobilabplusapi.so.1.12 libgmobilabplusapi.so' ")
	ENDIF(gMOBIlabplus_LIBRARY)
ENDIF(UNIX)
