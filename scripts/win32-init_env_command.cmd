@echo off

REM ########################################################################################################################

REM if exist "win32-dependencies.cmd" (
	REM call "win32-dependencies.cmd"
REM ) else (
	REM echo ERROR: win32-dependencies.cmd not found. Has the dependency installer been run?
	REM goto terminate
	
REM )
set "SCRIPT_PATH=%~dp0"

if "%1"=="" (
	set args=%SCRIPT_PATH%\..\dependencies
) else (
	set args=%*
)


SET PATH=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\bin;!PATH!
call :addToPathIfExists cmake\bin
call :addToPathIfExists ninja
call :addToPathIfExists expat\bin
call :addToPathIfExists itpp\bin
call :addToPathIfExists lua\lib
call :addToPathIfExists gtk\bin
call :addToPathIfExists cegui\bin
call :addToPathIfExists cegui\dependencies\bin
call :addToPathIfExists pthreads\lib
call :addToPathIfExists openal\libs\win32
call :addToPathIfExists freealut\lib
call :addToPathIfExists libogg\win32\bin\release
call :addToPathIfExists libogg\win32\bin\debug
call :addToPathIfExists libvorbis\win32\bin\release
call :addToPathIfExists libvorbis\win32\bin\debug
call :addToPathIfExists liblsl\lib
call :addToPathIfExists ogre\bin\release
call :addToPathIfExists ogre\bin\debug
call :addToPathIfExists vrpn\bin
call :addToPathIfExists openal\libs\Win32
call :addToPathIfExists liblsl\lib
call :addToPathIfExists sdk-brainproducts-actichamp
call :addToPathIfExists sdk-mcs\lib
call :addToPathIfExists xerces-c\lib
call :addToPathIfExists vcredist

REM pthread\lib
REM SET VRPNROOT=C:\c\openvibe\scripts\..\dependencies\vrpn


SET VSTOOLS=
SET VSCMake=

REM ########################################################################################################################
REM #
REM # By default, this script tries to find the newest VS first. If you want to use another version, execute e.g. GOTO VS2012
REM # 
REM GOTO VS2010

:VS2013
if exist "%VS120COMNTOOLS%vsvars32.bat" (
	echo Found VS120 tools at "%VS120COMNTOOLS%" ...
	CALL "%VS120COMNTOOLS%vsvars32.bat"
	SET VSCMake=Visual Studio 12
	SET OV_USE_VS2013=1
	goto terminate
)

:VS2012
if exist "%VS110COMNTOOLS%vsvars32.bat" (
	echo Found VS110 tools at "%VS110COMNTOOLS%" ...
	echo But VS2012 is not officially supported, if you want to try to use it, edit this script.
	REM echo Found VS110 tools at "%VS120COMNTOOLS%" ...
	REM CALL "%VS110COMNTOOLS%vsvars32.bat"
	REM SET VSCMake=Visual Studio 11
	REM SET OV_USE_VS2012=1
	REM goto terminate
)

:VS2010
if exist "%VS100COMNTOOLS%vsvars32.bat" (
	echo Found VS100 tools at "%VS100COMNTOOLS%" ...
	CALL "%VS100COMNTOOLS%vsvars32.bat"
	SET VSCMake=Visual Studio 10
	goto terminate
)


echo ######################################################################################
echo ##                                                                                  ##
echo ##  ERROR : Microsoft Visual Studio Common tools initialisation script not found    ##
echo ##  for supported VS version (2010 or 2013)                                         ##
echo ##                                                                                  ##
echo ######################################################################################
goto terminate

REM #######################################################################################

:addToPathIfExists
for %%A in (%args%) DO (
	if exist "%%A\%~1\" (
		set "PATH=%%A\%~1;!PATH!"
	)
)
exit /B 0


:terminate
