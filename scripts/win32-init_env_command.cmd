@echo off

REM ########################################################################################################################

if exist "win32-dependencies.cmd" (
	call "win32-dependencies.cmd"
) else (
	echo ERROR: win32-dependencies.cmd not found. Has the dependency installer been run?
	goto terminate
	
)

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

:terminate
