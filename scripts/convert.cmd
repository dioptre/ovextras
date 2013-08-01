set Extension=%~x1
REM remove the dot before the extension
set Extension=%Extension:~1%
set DestinationExtension=%~x2
set DestinationExtension=%DestinationExtension:~1%

setlocal EnableDelayedExpansion
set FormatFrom[0]=dat
set FormatFrom[1]=vhdr
set FormatFrom[2]=csv
set FormatFrom[3]=gdf
set FormatFrom[4]=ov

set From=false
set /a i=0

REM apparently there is no while in cmd
:checkExtension
echo %i% %From%
if %i% geq 5 (
goto ExtensionChecked)
 if !FormatFrom[%i%]!==%Extension% (
set From=true
goto ExtensionChecked) else (
set /a i=i+1
goto checkExtension)
:ExtensionChecked


set FormatTo[0]=csv
set FormatTo[1]=gdf
set FormatTo[2]=edf
set FormatTo[3]=ov

set To=false
set /a j=0

REM apparently there is no while in cmd
:checkDestinationExtension
echo %j% %From%
if %j% geq 4 (
goto DestinationExtensionChecked)
 if !FormatTo[%j%]!==%DestinationExtension% (
set To=true
goto DestinationExtensionChecked) else (
set /a j=j+1
goto checkDestinationExtension)
:DestinationExtensionChecked




if %From%==false (
echo 'There is no reader box for the %Extension% format in openvibe' )

if %To%==false (
echo 'There is no writer box for the %DestinationExtension% format in openvibe' )

if %From%==true (
if %To%==true (

REM TODO change to match the new folder structure
REM set "ScenarioFolder=C:\openvibe-0.12.0-svn3107-src\openvibe-scenarios\trunc\share\openvibe-scenarios\conversion\"
set "ScenarioFolder=..\conversion\"

set ScenarioToOpen=%Extension%2%DestinationExtension%.xml

set OV_CONVERT_SRC=%1
set OV_CONVERT_DEST=%2

REM launch designer
REM since I moved the scripts to script, I'll have to test that
start ..\dist\ov-designer.cmd --no-gui --no-session-management --play-fast !ScenarioFolder!!ScenarioToOpen!
)
)

endlocal

