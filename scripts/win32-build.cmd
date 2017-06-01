@echo off
setlocal EnableDelayedExpansion
setlocal enableextensions 

REM no options / set to default
set BuildType=Release
set InitEnvScript=win32-init_env_command.cmd
set PAUSE=pause

echo  (%*)
:parameter_parse
if /i "%1"=="-h" (
	echo Usage: win32-build.cmd [Build Type] [Init-env Script]
	echo -- Build Type option can be : --release (-r^), --debug (-d^) or --debug-symbols. Default is Release.
	echo -- Default Init-env script is: win32-init_env_command.cmd
	pause
	exit 0
) else if /i "%1"=="--help" (
	echo Usage: win32-build.cmd [Build Type] [Init-env Script]
	echo -- Build Type option can be : --release (-r^), --debug (-d^) or --debug-symbols. Default is Release.
	echo -- Default Init-env script is: win32-init_env_command.cmd
	pause
	exit 0
) else if /i "%1"=="--no-pause" (
	set PAUSE=echo ""
	SHIFT
	Goto parameter_parse
) else if /i "%1"=="-d" (
	set BuildType=Debug
	REM set !InitEnvScript!=%2
	SHIFT
	Goto parameter_parse
) else if /i "%1"=="--debug" (
	set BuildType=Debug
	REM set !InitEnvScript!=%2
	SHIFT
	Goto parameter_parse
) else if /i "%1"=="--debug-symbols" (
	set BuildType=RelWithDebInfo
	REM set !InitEnvScript!=%2
	SHIFT
	Goto parameter_parse
) else if /i "%1"=="-r" (
	set BuildType=Release
	REM set !InitEnvScript!=%2
	SHIFT
	Goto parameter_parse
) else if /i "%1"=="--release" (
	set BuildType=Release
	REM set !InitEnvScript!=%2
	SHIFT
	Goto parameter_parse
) else if /i "%1"=="--studiosdk" (
	set studiosdk="-DSTUDIO_SDK_PATH=%2"
	SHIFT
	SHIFT
	Goto parameter_parse
) else if /i "%1"=="--build-dir" (
	set ov_build_dir=%2
	SHIFT
	SHIFT
	Goto parameter_parse
) else if /i "%1"=="--install-dir" (
	set ov_install_dir=%2
	SHIFT
	SHIFT
	Goto parameter_parse
) else if not "%1" == "" (
	echo unrecognized option [%1]
	Goto terminate_error
)

REM else if not "%1"=="" (
	REM set BuildType=Release
	REM set !InitEnvScript!=%1
	REM SHIFT
	REM Goto parameter_parse
REM )

if /i "!InitEnvScript!"=="win32-init_env_command.cmd" (
	echo No script specified. Default will be used.
)

echo --
echo build type is set to: %BuildType%.
echo Init-env Script to be called: !InitEnvScript!.
echo bd %ov_build_dir%
echo sdk %studiosdk%
echo --

REM #######################################################################################

call "!InitEnvScript!"

REM #######################################################################################

set ov_script_dir=%CD%

if not defined ov_build_dir (
	set ov_build_dir=%ov_script_dir%\..\local-tmp\nmake-%BuildType%
)
if not defined ov_install_dir (
	set ov_install_dir=%ov_script_dir%\..\dist
)
echo.
echo _______________________________________________________________________________
echo.

mkdir %ov_build_dir% 2>NUL
cd /D %ov_build_dir%

echo Generating makefiles for %VSCMake% using %BuildType% config.
echo Building to %ov_build_dir% ...

%ov_script_dir%\..\dependencies\cmake\bin\cmake %ov_script_dir%\.. -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=%BuildType% -DCMAKE_INSTALL_PREFIX=%ov_install_dir% %studiosdk%
IF NOT "!ERRORLEVEL!" == "0" goto terminate_error

echo.
echo Building and installing ...
echo.

nmake install
IF NOT "!ERRORLEVEL!" == "0" goto terminate_error

echo.
echo Install completed !
echo.

echo.
echo _______________________________________________________________________________
echo.

goto terminate_success

REM #######################################################################################

:terminate_error

echo.
echo An error occured during building process !
echo.
%PAUSE%

goto terminate

REM #######################################################################################

:terminate_success

%PAUSE%

goto terminate

REM #######################################################################################

:terminate

cd %ov_script_dir%

