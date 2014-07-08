# This file specifies the test which should be executed

# The file should be placed in the binary directory ${CTEST_BINARY_DIRECTORY}.

#Each test consists of:

#    The unique name of the test ( eg.: testname1 )
#    The full path to the executable of the test ( eg.: "$ENV{HOME}/bin/TEST_EXECUTABLE_1.sh" )
#    A List of arguments to the executable ( eg.: "ARGUMENT_1" "ARGUMENT_2" etc. ) 
	
# basic test (just for sample) check that binary directory is readable 
#ADD_TEST(LS_BINARY_PATH "ls" "-all")
#ADD_TEST(PWD_BINARY_PATH "pwd")

## -- Other Tests : Place a file named DartTestfile.txt in path with tests.

# ${TEST_AUTOMATIC} is replaced by "TRUE" only by automatic test call (via "openVibeTests.cmake")
IF(${TEST_AUTOMATIC} MATCHES "TRUE")
	# for automatic tests, no changes (at the moment
	MESSAGE("Mode 'automatic', OV_ROOT_DIR is ${OV_ROOT_DIR}")
	
ELSE(${TEST_AUTOMATIC} MATCHES "TRUE")
	# for local tests, we rework some paths
	
	IF(WIN32)
		# tricky way to get absolute path for ../. directory in windows with cygwin
		# todo: try to find a cmake way to get this for simple call to ctest in OV_ROOT_DIR\test directory in the aim to run all test
		exec_program("cygpath" ARGS "-a -w ../." OUTPUT_VARIABLE "OV_ROOT_DIR")
	ELSE(WIN32)
		set(OV_ROOT_DIR              "$ENV{PWD}/..")
	ENDIF(WIN32)
	
	set(CTEST_SOURCE_DIRECTORY      "${OV_ROOT_DIR}")

	MESSAGE("Mode 'local', OV_ROOT_DIR is ${OV_ROOT_DIR}")	
ENDIF(${TEST_AUTOMATIC} MATCHES "TRUE")

# passthrough a environment variable to binary path to tests
SET(ENV{OV_BINARY_PATH} "${CTEST_SOURCE_DIRECTORY}/dist")
MESSAGE("Running test executables from $ENV{OV_BINARY_PATH}")
	
# This is the userspace path for openvibe logs etc	
IF(WIN32)
  SET(ENV{OV_USERDATA} "$ENV{APPDATA}/openvibe/")
ELSE(WIN32)
  SET(ENV{OV_USERDATA} "$ENV{HOME}/.config/openvibe/")
ENDIF(WIN32)
MESSAGE("OpenViBE logs are expected in $ENV{OV_USERDATA}")

# this is the folder where test scenarios can be run under
SET(ENV{OV_TEST_DEPLOY_PATH} "${OV_ROOT_DIR}/local-tmp/test-deploy/")
MESSAGE("Test temporary files should be written to $ENV{OV_TEST_DEPLOY_PATH}")

get_cmake_property(_variableNames VARIABLES)
message("Dumping cmake variables...")
foreach (_variableName ${_variableNames})
    message("  ${_variableName}=${${_variableName}}")
endforeach()

SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/acquisition/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/turbofieldtrip/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/tools/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/vrpn/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/stimulation/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/feature-extraction/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/classification-gpl/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/signal-processing/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/simple-visualisation/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/file-io/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/samples/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/examples/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/classification/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/stream-codecs/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/python/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/streaming/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/signal-processing-gpl/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/matlab/test")
# SUBDIRS("${CTEST_SOURCE_DIRECTORY}/applications/external-stimulation-connection-example/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/applications/platform/acquisition-server/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/applications/platform/designer/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/applications/demos/ssvep-demo/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/applications/demos/vr-demo/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/applications/developer-tools/id-generator/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/applications/developer-tools/skeleton-generator/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/applications/developer-tools/vrpn-simulator/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/applications/developer-tools/plugin-inspector/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/documentation/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/toolkit/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/scenarios/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/openvibe/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/kernel/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/modules/xml/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/modules/automaton/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/modules/stream/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/modules/ebml/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/modules/socket/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/modules/fs/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/modules/system/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/scripts/software/tmp/vrpn/java_vrpn/test")


