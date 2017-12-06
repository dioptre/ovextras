This directory contains scripts and configuration files for automatic checkout sources, building and potentially run tests of openViBe.
This files need to be placed in $HOME directory of test account.

Dependencies: 
------------

to run these scripts, you need ctest (cmake suit), sh-utils (coreutils on windows cygwin or gnuwin32), git command-line tools and a c++ compiler (C++ Visual Studio 2013 on windows) 

On Linux fedora you need to install :
yum install cmake git redhat-lsb gcc-c++ expect


Execute tests scripts:
----------------------
Call "ctest -T Test" in extras build folder

How to add new test:
-------------------

You can add new test using DartTestFile.txt placed in test directory from a specific module.

For example, designer specific tests will be placed in DartTestFile.txt in applications/platform/designer/test/
And specific test for data generation plugin will be placed in DartTestFile.txt in plugins/processing/data-generation/test/
To be sure that test are executed, you need to add this subdirectories in file trunk/test/CTestfile.cmake.
For last examples:
...
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/applications/platform/designer/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/data-generation/test")
...


There is an exemple of test using Sinus Oscillator in plugins/processing/data-generation/test/DartTestFile.txt :

SET(TEST_NAME SinusOscillator)
SET(SCENARIO_TO_TEST "${TEST_NAME}.xml")

IF(WIN32)
	SET(EXT cmd)
	SET(OS_FLAGS "--no-pause")
ELSE(WIN32)
	SET(EXT sh)
	SET(OS_FLAGS "")
ENDIF(WIN32)

ADD_TEST(run_${TEST_NAME} "$ENV{OV_BINARY_PATH}/openvibe-designer.${EXT}" ${OS_FLAGS} "--invisible" "--play" ${SCENARIO_TO_TEST})
ADD_TEST(comparator_${TEST_NAME} "git" "diff" "--no-index" "${TEST_NAME}.csv" "${TEST_NAME}.ref.csv")

## add some properties that help to debug 
SET_TESTS_PROPERTIES(run_${TEST_NAME} PROPERTIES ATTACHED_FILES_ON_FAIL "${OV_LOGFILE}"}
SET_TESTS_PROPERTIES(comparator_${TEST_NAME} PROPERTIES ATTACHED_FILES_ON_FAIL "${TEST_NAME}.csv"}
SET_TESTS_PROPERTIES(comparator_${TEST_NAME} PROPERTIES DEPENDS run_${TEST_NAME}}


*** WARNING : THIS IS UNTESTED ON OV2.0 ***
Test GUI using sikuli-ide :
-------------------------

If you have installed sikuli-ide in your linux machine them some GUI test are launch.

For GUI test with sikuli we need a complete gtk windows manager. Actually, we only test with gnome whole package.

You need to set the same GTK icon theme between machine that generate test and slave.

You can install (on ubuntu) :

sudo aptitude install gnome-tweak-tool  ubuntu-mono ttf-ubuntu-font-family light-themes dmz-cursor-theme

them lanch :

gnome-tweak-tool

    switch icon theme to Ubuntu-Mono-Dark
    switch GTK+ theme to Ambiance
    if there is Windows theme then switch to Ambiance
    switch Cursor theme to DMZ-White 





Some Remarks :
------------

    This test run designer with no GUI, but in Linux it still need a X11 context. So you need to be sure that test can access to a X server. That will be do by a automatic start-up of "Xorg -ac&" command to ensure that X server is launched at test moment. 
    That's why we need to define DISPLAY environment variable before launch test.
    This test run in path : plugins/processing/data-generation/test/.
    This test works with a specific scenario that content automatic stop set to a 1 second (using Clock stimulator+Player Controller),
    This test produce a CSV file output that contents output of Sinus oscillator for 1 second (using CSV File Writer)
    Second test is a "white box" test that compare current output signal file with a reference file using "diff" program as comparator. Signal reference file was obtained with the same scenario "a day when all work fine" (actually, non-regression test) 
    This DartTestFile.txt will be adapted to others boxes to produce same type of tests.

That's all 
