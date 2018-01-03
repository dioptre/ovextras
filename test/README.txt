
This directory contains scripts for running the automatic tests of OpenViBE.

Dependencies
------------
   
To use these tests, you need to have openvibe already successfully compiled with the NON-ide buid.

On windows, you need to have
   - ctest (cmake suite; usually in openvibe's dependencies)
   - git command-line tools. The git executable needs to be on PATH.

On Linux fedora, you need to install :

# yum install cmake git redhat-lsb gcc-c++ expect


Execute test scripts
--------------------
Call "ctest -T Test" in folder build/extras-Release/


How to add a new test
---------------------

You can add new test using a DartTestFile.txt placed in the test directory of a specific module.

For example, designer specific tests should be placed in DartTestFile.txt in applications/platform/designer/test/
And specific test for data generation plugin should be placed in DartTestFile.txt in plugins/processing/data-generation/test/
To be sure that the test is executed, you need to have the relevant subdirectories declared in file test/CTestfile.cmake.

For example,

...
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/applications/platform/designer/test")
SUBDIRS("${CTEST_SOURCE_DIRECTORY}/plugins/processing/data-generation/test")
...


There is an example of a test using Sinus Oscillator in plugins/processing/data-generation/test/DartTestFile.txt.


Test GUI using sikuli-ide
-------------------------
*** WARNING : THE SIKULI TESTS ARE NOT CURRENTLY MAINTAINED AND PROBABLY WILL NOT WORK ***

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



Some remarks
------------

    - The tests may run designer with no GUI. In Linux this still needs a X11 context. So you need to be sure that the test can access an X server. 
    If you run on a virtual machine that doesn't start X by default, getting X up can be achieved by automatic start-up of "Xorg -ac&" command to ensure that X server is available. 
    Thats also why we need to define DISPLAY environment variable before launching the tests.
	- For designing new tests, use the existing DartTestFile.txt files as a starting point


