@echo off
setlocal EnableDelayedExpansion
setlocal enableextensions 

REM no options / set to default
set BuildType=Release
set InitEnvScript=win32-init_env_command.cmd
set PAUSE=pause
set ov_script_dir=%CD%
set generator=-G"Ninja"
set builder=Ninja

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
) else if /i "%1"=="--sdk" (
	set sdk=-DOPENVIBE_SDK_PATH=%2
	set sdk_val=%2
	SHIFT
	SHIFT
	Goto parameter_parse
) else if /i "%1"=="--designer" (
	set designer=-DDESIGNER_SDK_PATH=%2
	set designer_val=%2
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
) else if /i "%1"=="--dependencies-dir" (
	set dependencies_path="-DLIST_DEPENDENCIES_PATH=%2"
	set dependencies_base=%2
	SHIFT
	SHIFT
	Goto parameter_parse
) else if /i "%1"=="--vsproject" (
	set vsgenerate=TRUE
	set builder=None
	SHIFT
	Goto parameter_parse
) else if /i "%1"=="--vsbuild" (
	set vsgenerate=TRUE
	set builder=Visual
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
if defined vsgenerate (
	echo Build type is set to: MultiType.
) else (
	echo Build type is set to: %BuildType%.
)
echo Init-env Script to be called: !InitEnvScript!.
REM #######################################################################################

call "!InitEnvScript!" %ov_script_dir%\..\dependencies %dependencies_base%

REM #######################################################################################

if defined vsgenerate (
	set generator=-G"%VSCMake%" -T "v120"
	if not defined build_dir (
		set build_dir=%root_dir%\..\openvibe-extras-build\vs-project
	)
	if not defined install_dir (
		set install_dir=%root_dir%\..\openvibe-extras-build\dist
	)
) else (
	set build_type="-DCMAKE_BUILD_TYPE=%BuildType%"
	if not defined build_dir (
		set build_dir=%root_dir%\..\openvibe-extras-build\build-%BuildType%
	)
	if not defined install_dir (
		set install_dir=%root_dir%\..\openvibe-extras-build\dist-%BuildType%
	)
)
if defined sdk (
	echo SDK is located at %sdk_val%
) else (
	echo "Using default for SDK path (check CMake for inferred value)"
)

if defined designer (
	echo Designer is located at %designer_val%
) else (
	echo "Using default for Designer path (check CMake for inferred value)"
)


echo.
echo _______________________________________________________________________________
echo.

mkdir %ov_build_dir% 2>NUL
cd /D %ov_build_dir%

echo Generating makefiles for %VSCMake%.
echo Building to %ov_build_dir% ...

cmake %ov_script_dir%\..  %generator% %build_type% -DCMAKE_INSTALL_PREFIX=%ov_install_dir% %designer% %sdk% %dependencies_path%
IF NOT "!ERRORLEVEL!" == "0" goto terminate_error

echo.
echo Building and installing ...
echo.

if !builder! == None (
	goto terminate_success
) else if !builder! == Ninja (
	ninja install
	if not "!ERRORLEVEL!" == "0" goto terminate_error
) else if !builder! == Visual (
	msbuild Openvibe.sln /p:Configuration=%BuildType%
	if not "!ERRORLEVEL!" == "0" goto terminate_error
	
	cmake --build . --config %BuildType% --target install
	if not "!ERRORLEVEL!" == "0" goto terminate_error
)

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

