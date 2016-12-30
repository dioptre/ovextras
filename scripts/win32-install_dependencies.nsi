	SetCompressor /FINAL /SOLID lzma
	SetCompressorDictSize 16

	!include "MUI.nsh"
	!include "Sections.nsh"
	!include "zipdll.nsh"

	;Name and file
	Name "OpenViBE dependencies"
	OutFile "win32-install_dependencies.exe"

	;Default installation folder
	InstallDir "$EXEDIR\..\dependencies"

;Interface Settings

	!define MUI_ABORTWARNING
	!define MUI_COMPONENTSPAGE_NODESC

;Pages

	!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_INSTFILES

;  !insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES

;Languages

	!insertmacro MUI_LANGUAGE "English"

;Installer and uninstaller icons

	Icon "${NSISDIR}\Contrib\Graphics\Icons\box-install.ico"
	UninstallIcon "${NSISDIR}\Contrib\Graphics\Icons\box-uninstall.ico"

;Visual studio suffix

	Var suffix

;##########################################################################################################################################################
;##########################################################################################################################################################
;##########################################################################################################################################################

Section "-base"

	CreateDirectory "$INSTDIR"
	CreateDirectory "$INSTDIR\arch"
	
	;Finds Microsoft Platform SDK

	ReadRegStr $r0 HKLM "SOFTWARE\Microsoft\Win32SDK\Directories" "Install Dir"
	StrCmp $r0 "" base_failed_to_find_sdk_1 base_found_sdk
base_failed_to_find_sdk_1:
	ReadRegStr $r0 HKLM "SOFTWARE\Microsoft\MicrosoftSDK\Directories" "Install Dir"
	StrCmp $r0 "" base_failed_to_find_sdk_2 base_found_sdk
base_failed_to_find_sdk_2:
	ReadRegStr $r0 HKLM "SOFTWARE\Microsoft\Microsoft SDKs\Windows" "CurrentInstallFolder"
	StrCmp $r0 "" base_failed_to_find_sdk_3 base_found_sdk
base_failed_to_find_sdk_3:
	goto base_failed_to_find_sdk

base_failed_to_find_sdk:
	MessageBox MB_OK|MB_ICONEXCLAMATION "Failed to find Microsoft Platform SDK$\nPlease update your win32-dependencies.cmd script by hand" /SD IDOK
	goto base_go_on
base_found_sdk:
	MessageBox MB_OK "Microsoft Platform SDK found at :$\n$r0" /SD IDOK
	goto base_go_on

base_go_on:

	;Create uninstaller
	WriteUninstaller "$INSTDIR\Uninstall.exe"

	;clears dependencies file
	FileOpen $0 "$EXEDIR\win32-dependencies.cmd" w
	FileWrite $0 "@echo off$\r$\n"
	FileWrite $0 "$\r$\n"
	FileWrite $0 "SET PATH=$r0\bin;%PATH%$\r$\n"
	FileClose $0

SectionEnd

;##########################################################################################################################################################
;##########################################################################################################################################################
;##########################################################################################################################################################

SectionGroup "!Compilation platform"

Section /o "Visual C++ 2010" vc100
	StrCpy $suffix "vc100"
SectionEnd

Section "Visual C++ 2013" vc120
	StrCpy $suffix "vc120"
SectionEnd
SectionGroupEnd

;##########################################################################################################################################################
;##########################################################################################################################################################
;##########################################################################################################################################################

SectionGroup "Dependencies"

Section "DirectX Runtime"
    SectionIn RO
	
	SetOutPath "$INSTDIR"

	IfFileExists "$SYSDIR\d3dx9_43.dll" no_need_to_install_directx
	IfFileExists "arch\directx-jun2010.exe" no_need_to_download_directx
	NSISdl::download "https://download.microsoft.com/download/8/4/A/84A35BF1-DAFE-4AE8-82AF-AD2AE20B6B14/directx_Jun2010_redist.exe" "arch\directx-jun2010.exe"
	Pop $R0 ; Get the return value
		StrCmp $R0 "success" +3
			MessageBox MB_OK "Download failed: $R0" /SD IDOK
			Quit
no_need_to_download_directx:
	ExecWait '"arch\directx-jun2010.exe" /T:"$INSTDIR\tmp" /Q'
	ExecWait '"$INSTDIR\tmp\DXSETUP.exe" /silent'
	RMDir /r "$INSTDIR\tmp"
no_need_to_install_directx:

SectionEnd

;##########################################################################################################################################################
;##########################################################################################################################################################
;##########################################################################################################################################################

Section "Visual C++ Redistributable Packages"
    SectionIn RO
	
	SetOutPath "$INSTDIR"

	IfFileExists "arch\vcredist-2010.exe" no_need_to_download_vc2010_redist
	NSISdl::download "http://download.microsoft.com/download/5/B/C/5BC5DBB3-652D-4DCE-B14A-475AB85EEF6E/vcredist_x86.exe" "arch\vcredist-2010.exe"
	Pop $R0 ; Get the return value
		StrCmp $R0 "success" +3
			MessageBox MB_OK "Download failed: $R0" /SD IDOK
			Quit
no_need_to_download_vc2010_redist:
	ExecWait '"arch\vcredist-2010.exe" /q'

	IfFileExists "arch\vcredist-2013_x86.exe" no_need_to_download_vc2013_redist
	NSISdl::download "http://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x86.exe" "arch\vcredist-2013_x86.exe"
	Pop $R0 ; Get the return value
		StrCmp $R0 "success" +3
			MessageBox MB_OK "Download failed: $R0" /SD IDOK
			Quit
no_need_to_download_vc2013_redist:			
	ExecWait '"arch\vcredist-2013_x86.exe" /Q'

SectionEnd

;##########################################################################################################################################################
;##########################################################################################################################################################
;##########################################################################################################################################################

Section "Everything else"
    SectionIn RO
	
	SetOutPath "$INSTDIR"

	; Note that the dependencies package version number corresponds to the *first* openvibe 
	; version that requires this package. Later openvibes may use it as well.
	; OV 1.2.x -> dependencies-1.2.0
	; OV 1.3.0 -> dependencies-1.2.0
	IfFileExists "arch\ov-dependencies-1.2.0-$suffix-dev.zip" no_need_to_download_dependencies_dev
	NSISdl::download http://openvibe.inria.fr/dependencies/win32/ov-dependencies-1.2.0-$suffix-dev.zip "arch\ov-dependencies-1.2.0-$suffix-dev.zip"
	Pop $R0 ; Get the return value
		StrCmp $R0 "success" +3
			MessageBox MB_OK "Download failed: $R0" /SD IDOK
			Quit
no_need_to_download_dependencies_dev:
	ZipDLL::extractall "arch\ov-dependencies-1.2.0-$suffix-dev.zip" ""

	IfFileExists "arch\ov-dependencies-1.2.0-$suffix-runtime.zip" no_need_to_download_dependencies_runtime
	NSISdl::download http://openvibe.inria.fr/dependencies/win32/ov-dependencies-1.2.0-$suffix-runtime.zip "arch\ov-dependencies-1.2.0-$suffix-runtime.zip"
	Pop $R0 ; Get the return value
		StrCmp $R0 "success" +3
			MessageBox MB_OK "Download failed: $R0" /SD IDOK
			Quit
no_need_to_download_dependencies_runtime:
	ZipDLL::extractall "arch\ov-dependencies-1.2.0-$suffix-runtime.zip" ""
	
; Note: Do NOT put cmake bin on the PATH, as/if it contains MSVC* libraries. These have been
; observed to have been picked up by third-party software like the python interpreter
; and caused hard-to-trace manifest errors.
	
	FileOpen $0 "$EXEDIR\win32-dependencies.cmd" a
	FileSeek $0 0 END
	
; Generic dependencies

	FileWrite $0 "SET PATH=$INSTDIR\expat\bin;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\itpp\bin;%PATH%$\r$\n"	
	FileWrite $0 "SET PATH=$INSTDIR\lua\lib;%PATH%$\r$\n"	
	FileWrite $0 "SET PATH=$INSTDIR\gtk\bin;%PATH%$\r$\n"		
	FileWrite $0 "SET OGRE_HOME=$INSTDIR\ogre$\r$\n"	
	FileWrite $0 "SET PATH=%OGRE_HOME%\bin\release;%OGRE_HOME%\bin\debug;%PATH%$\r$\n"	
	FileWrite $0 "SET PATH=$INSTDIR\cegui\bin;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\cegui\dependencies\bin;%PATH%$\r$\n"	
	FileWrite $0 "SET VRPNROOT=$INSTDIR\vrpn$\r$\n"
	FileWrite $0 "SET PATH=%VRPNROOT%\bin;%PATH%$\r$\n"	
	FileWrite $0 "SET PATH=$INSTDIR\pthreads\lib;%PATH%$\r$\n"	
	FileWrite $0 "SET PATH=$INSTDIR\openal\libs\win32;%PATH%$\r$\n"	
	FileWrite $0 "SET PATH=$INSTDIR\freealut\lib;%PATH%$\r$\n"	
	FileWrite $0 "SET PATH=$INSTDIR\libogg\win32\bin\release;$INSTDIR\libogg\win32\bin\debug;%PATH%$\r$\n"	
	FileWrite $0 "SET PATH=$INSTDIR\libvorbis\win32\bin\release;$INSTDIR\libvorbis\win32\bin\debug;%PATH%$\r$\n"	
	FileWrite $0 "SET PATH=$INSTDIR\liblsl\lib\;%PATH%$\r$\n"	
	
	FileClose $0
	
; Package configuration
	
	FileOpen $0 "$INSTDIR\gtk\lib\pkgconfig\gtk+-win32-2.0.pc" w
	FileWrite $0 "prefix=$INSTDIR\gtk$\r$\n"
	FileWrite $0 "exec_prefix=$${prefix}$\r$\n"
	FileWrite $0 "libdir=$${exec_prefix}/lib$\r$\n"
	FileWrite $0 "includedir=$${prefix}/include$\r$\n"
	FileWrite $0 "target=win32$\r$\n"
	FileWrite $0 "$\r$\n"
	FileWrite $0 "Name: GTK+$\r$\n"
	FileWrite $0 "Description: GTK+ Graphical UI Library ($${target} target)$\r$\n"
	FileWrite $0 "Version: 2.22.1$\r$\n"
	FileWrite $0 "Requires: gdk-$${target}-2.0 atk cairo gdk-pixbuf-2.0 gio-2.0$\r$\n"
	FileWrite $0 "Libs: -L$${libdir} -lgtk-$${target}-2.0$\r$\n"
	FileWrite $0 "Cflags: -I$${includedir}/gtk-2.0$\r$\n"
	FileClose $0

	FileOpen $0 "$INSTDIR\gtk\lib\pkgconfig\gthread-2.0.pc" w
	FileWrite $0 "prefix=$INSTDIR\gtk$\r$\n"
	FileWrite $0 "exec_prefix=$${prefix}$\r$\n"
	FileWrite $0 "libdir=$${exec_prefix}/lib$\r$\n"
	FileWrite $0 "includedir=$${prefix}/include$\r$\n"
	FileWrite $0 "$\r$\n"
	FileWrite $0 "Name: GThread$\r$\n"
	FileWrite $0 "Description: Thread support for GLib$\r$\n"
	FileWrite $0 "Requires: glib-2.0$\r$\n"
	FileWrite $0 "Version: 2.26.0$\r$\n"
	FileWrite $0 "Libs: -L$${libdir} -lgthread-2.0$\r$\n"
	FileWrite $0 "Cflags:$\r$\n"
	FileClose $0

	FileOpen $0 "$INSTDIR\gtk\etc\gtk-2.0\gtkrc" w
	FileWrite $0 "gtk-theme-name = $\"Redmond$\"$\r$\n"
	FileWrite $0 "style $\"user-font$\"$\r$\n"
	FileWrite $0 "{$\r$\n"
	FileWrite $0 "	font_name=$\"Sans 8$\"$\r$\n"
	FileWrite $0 "}$\r$\n"
	FileWrite $0 "widget_class $\"*$\" style $\"user-font$\"$\r$\n"
	FileClose $0
	
	FileOpen $0 "$INSTDIR\cegui\resources.cfg" w
	FileWrite $0 "FileSystem=$INSTDIR\cegui\datafiles\configs$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\cegui\datafiles\fonts$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\cegui\datafiles\imagesets$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\cegui\datafiles\layouts$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\cegui\datafiles\looknfeel$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\cegui\datafiles\lua_scripts$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\cegui\datafiles\schemes$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\cegui\datafiles\xml_schemes$\r$\n"
	FileClose $0
	
SectionEnd

;##########################################################################################################################################################
;##########################################################################################################################################################
;##########################################################################################################################################################

SectionGroupEnd

Section "Uninstall"

	RMDir /r "$INSTDIR\alut"
	RMDir /r "$INSTDIR\boost"
	RMDir /r "$INSTDIR\cmake"	
	RMDir /r "$INSTDIR\cegui"		
	RMDir /r "$INSTDIR\expat"	
	RMDir /r "$INSTDIR\gtk"
	RMDir /r "$INSTDIR\itpp"
	RMDir /r "$INSTDIR\libogg"
	RMDir /r "$INSTDIR\libvorbis"
	RMDir /r "$INSTDIR\liblsl"	
	RMDir /r "$INSTDIR\lua"
	RMDir /r "$INSTDIR\ogre"
	RMDir /r "$INSTDIR\openal"	
	RMDir /r "$INSTDIR\pthreads"	
	RMDir /r "$INSTDIR\vrpn"
		
	Delete "$INSTDIR\..\scripts\win32-dependencies.cmd"

	Delete "$INSTDIR\Uninstall.exe"

	RMDir "$INSTDIR"

SectionEnd

;##########################################################################################################################################################
;##########################################################################################################################################################
;##########################################################################################################################################################

Function .onInit
  StrCpy $9 ${vc100}

  ; Note that for logging to work, you will need a logging-enabled build of nsis. 
  ; At the time of writing this, you could get one from http://nsis.sourceforge.net/Special_Builds 
  LogSet on
	
FunctionEnd

Function .onSelChange
  !insertmacro StartRadioButtons $9
    !insertmacro RadioButton ${vc100}
    !insertmacro RadioButton ${vc120}	
  !insertmacro EndRadioButtons
FunctionEnd
