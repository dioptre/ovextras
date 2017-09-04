# This file specifies the test which should be executed

# The file should be placed in the binary directory ${CTEST_BINARY_DIRECTORY}.

#Each test consists of:

#    The unique name of the test ( eg.: testname1 )
#    The full path to the executable of the test ( eg.: "$ENV{HOME}/bin/TEST_EXECUTABLE_1.sh" )
#    A List of arguments to the executable ( eg.: "ARGUMENT_1" "ARGUMENT_2" etc. ) 
	
# basic test (just for sample) check that binary directory is readable 
#ADD_TEST(LS_BINARY_PATH "ls" "-all")
#ADD_TEST(PWD_BINARY_PATH "pwd")


set(ENV{OV_BINARY_PATH} "@DIST_ROOT@")
set(OV_CONFIG_SUBDIR @OV_CONFIG_SUBDIR@) # This is used in the drt files

IF(WIN32)
  SET(ENV{OV_USERDATA} "$ENV{APPDATA}/${OV_CONFIG_SUBDIR}/")
ELSE()
  SET(ENV{OV_USERDATA} "$ENV{HOME}/.config/${OV_CONFIG_SUBDIR}/")
ENDIF()

set(CTEST_SOURCE_DIRECTORY "@CMAKE_SOURCE_DIR@")

# this is the folder where test scenarios can be run under
SET(ENV{OV_TEST_DEPLOY_PATH} "${CTEST_SOURCE_DIRECTORY}/local-tmp/test-deploy/")

# subdirs command is deprecated and should be replaced by add_subdirectory calls as per the documentation recommendations, 
# however the 2 command do not have the same behavior with ctest. Doing the change currently breaks tests.
subdirs("${CTEST_SOURCE_DIRECTORY}/contrib/plugins/server-extensions/tcp-tagging/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/acquisition/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/turbofieldtrip/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/tools/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/vrpn/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/stimulation/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/feature-extraction/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/signal-processing/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/simple-visualisation/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/file-io/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/data-generation/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/examples/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/classification/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/stream-codecs/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/python/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/streaming/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/matlab/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/plugins/processing/evaluation/test")
# subdirs("${CTEST_SOURCE_DIRECTORY}/applications/external-stimulation-connection-example/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/applications/platform/acquisition-server/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/applications/platform/designer/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/applications/demos/ssvep-demo/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/applications/demos/vr-demo/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/applications/developer-tools/id-generator/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/applications/developer-tools/skeleton-generator/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/applications/developer-tools/vrpn-simulator/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/applications/developer-tools/plugin-inspector/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/documentation/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/toolkit/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/scenarios/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/openvibe/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/kernel/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/modules/xml/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/modules/automaton/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/modules/stream/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/modules/ebml/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/modules/socket/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/modules/fs/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/modules/system/test")
subdirs("${CTEST_SOURCE_DIRECTORY}/scripts/software/tmp/vrpn/java_vrpn/test")
