@echo off

REM ########################################################################################################################

if exist "win32-dependencies.cmd" (
	call "win32-dependencies.cmd"
) else (
	echo ERROR: win32-dependencies.cmd not found. Has the dependency installer been run?
	goto terminate
	
)

REM ########################################################################################################################

REM # To force build with VS 2008 with VS 2010 / VS 2012 installed as well, set the following to 1
SET SKIP_VS2010=0
SET SKIP_VS2012=0

SET VSTOOLS=
SET VSCMake=
SET OV_USE_VS2012=

if %SKIP_VS2012% == 1 (
	echo Visual Studio 2012 detection skipped as requested
) else (
	if exist "%VS110COMNTOOLS%vsvars32.bat" (
		echo Found VS110 tools at "%VS110COMNTOOLS%" ...
		CALL "%VS110COMNTOOLS%vsvars32.bat"
		SET VSCMake=Visual Studio 11
		SET OV_USE_VS2012=1
		goto terminate
	)
)

if %SKIP_VS2010% == 1 (
	echo Visual Studio 2010 detection skipped as requested
) else (
	if exist "%VS100COMNTOOLS%vsvars32.bat" (
		echo Found VS100 tools at "%VS100COMNTOOLS%" ...
		CALL "%VS100COMNTOOLS%vsvars32.bat"
		SET VSCMake=Visual Studio 10
		goto terminate
	)
)

if exist "%VS90COMNTOOLS%vsvars32.bat" (
	echo Found VS90 tools at "%VS90COMNTOOLS%" ...
	CALL "%VS90COMNTOOLS%vsvars32.bat"
	SET VSCMake=Visual Studio 9 2008
	goto terminate
)

echo ######################################################################################
echo ##                                                                                  ##
echo ##  ERROR : Microsoft Visual Studio Common tools initialisation script not found    ##
echo ##                                                                                  ##
echo ######################################################################################
goto terminate

REM #######################################################################################

:terminate
