	SetCompressor /FINAL /SOLID lzma
	SetCompressorDictSize 16

	!include "MUI.nsh"
	!include "zipdll.nsh"
  
	;Name and file
	Name "OpenViBE 2.0.0-beta"
	OutFile "openvibe-2.0.0-beta-setup.exe"

	;Default installation folder
	InstallDir "$PROGRAMFILES\openvibe-beta"
	Var OLDINSTDIR
	Var DIRECTX_MISSING

;Interface Settings

	!define MUI_ABORTWARNING
	
;Pages

	!insertmacro MUI_PAGE_WELCOME
	!insertmacro MUI_PAGE_LICENSE "..\COPYING"
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_COMPONENTS	
	!insertmacro MUI_PAGE_INSTFILES
	!insertmacro MUI_PAGE_FINISH

	!insertmacro MUI_UNPAGE_WELCOME
	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES
	!insertmacro MUI_UNPAGE_FINISH


	
;Languages

	!insertmacro MUI_LANGUAGE "English"

;Installer and uninstaller icons

	Icon "${NSISDIR}\Contrib\Graphics\Icons\box-install.ico"
	UninstallIcon "${NSISDIR}\Contrib\Graphics\Icons\box-uninstall.ico"

;##########################################################################################################################################################
;##########################################################################################################################################################
;##########################################################################################################################################################

  
Function .onInit

	; Blurp

	MessageBox MB_OK "WARNING: This is installer for OpenViBE 2.0.0 BETA. As a BETA version, it may have various issues. The last stable version of OpenViBE is 1.3.0. If you decide to install this BETA anyway, please read the release notes carefully on the OpenViBE forum and webpage."
	
	; Note that for logging to work, you will need a logging-enabled build of nsis. 
	; At the time of writing this, you could get one from http://nsis.sourceforge.net/Special_Builds 
	LogSet on
  
	UserInfo::GetAccountType
	Pop $R1
	StrCmp $R1 "Admin" has_admin_rights 0
		MessageBox MB_OK "You must be administrator to install OpenViBE" /SD IDOK
		Quit
has_admin_rights:

	ReadRegStr $0 HKLM SOFTWARE\openvibe-beta InstallDir

	${If} $0 != ""
		IfFileExists "$0\Uninstall.exe" +1 +5
			MessageBox MB_YESNO "A previous installation of OpenViBE is installed under $0.$\nContinuing the install procedure will remove previous installation of OpenViBE (including all files you eventually added in the installation directory).$\nWould you like to accept this removal and continue on installation process ?" /SD IDYES IDNO +1 IDYES +2
			Abort
		StrCpy $OLDINSTDIR $0
		StrCpy $INSTDIR $0
	${EndIf}

	; Make OpenViBE section mandatory
	IntOp $0 ${SF_SELECTED} | ${SF_RO}
	IntOp $0 $0 | ${SF_BOLD}
    SectionSetFlags "Section1" $0
	
FunctionEnd

; Returns characters before -
Function GetFirstStrPart
  Exch $R0
  Push $R1
  Push $R2
  StrLen $R1 $R0
  IntOp $R1 $R1 + 1
  loop:
    IntOp $R1 $R1 - 1
    StrCpy $R2 $R0 1 -$R1
    StrCmp $R2 "" exit2
    StrCmp $R2 "-" exit1 
  Goto loop
  exit1:
    StrCpy $R0 $R0 -$R1
  exit2:
    Pop $R2
    Pop $R1
    Exch $R0
FunctionEnd

;##########################################################################################################################################################
;##########################################################################################################################################################
;##########################################################################################################################################################

Section "!OpenViBE" Section1

	LogSet on
	
	${If} $OLDINSTDIR != ""
		RMDir /r $OLDINSTDIR
		RMDir /r "$SMPROGRAMS\OpenViBE-beta"
	${EndIf}

	SetOutPath $INSTDIR
	WriteRegStr HKLM "SOFTWARE\openvibe-beta" "InstallDir" "$INSTDIR"
	WriteUninstaller Uninstall.exe

	CreateDirectory "$INSTDIR\dependencies\arch"
	StrCpy $DIRECTX_MISSING "false"

	SetOutPath "$INSTDIR\dependencies"
	IfFileExists "$SYSDIR\d3dx9_43.dll" no_need_to_install_directx
	NSISdl::download "https://download.microsoft.com/download/8/4/A/84A35BF1-DAFE-4AE8-82AF-AD2AE20B6B14/directx_Jun2010_redist.exe" "arch\directx-jun2010.exe"
	Pop $R0 ; Get the return value
		StrCmp $R0 "success" +4
			MessageBox MB_OK "Download failed: $R0$\nCheck your Internet connection and your firewall settings.$\nDirect X won't be installed and 3D functionalities won't be available...$\nYou can install DirectX later to enable 3D functionalities !" /SD IDOK
			StrCpy $DIRECTX_MISSING "true"
			Goto no_need_to_install_directx ; Quit
	ExecWait '"arch\directx-jun2010.exe" /T:"$INSTDIR\tmp" /Q'
	ExecWait '"$INSTDIR\tmp\DXSETUP.exe" /silent'
	RMDir /r "$INSTDIR\tmp"
no_need_to_install_directx:

	SetOutPath "$INSTDIR\dependencies\arch"
	File "..\dependencies\arch\*-runtime.zip"
	File "..\dependencies\arch\freealut*.zip"
	File "..\dependencies\arch\expat*.zip"	
	File "..\dependencies\arch\liblsl*.zip"		
	File "..\dependencies\arch\libogg*.zip"	
	File "..\dependencies\arch\libvorbis*.zip"		
	File "..\dependencies\arch\lua*.zip"			
	File "..\dependencies\arch\pthread*.zip"		
	
	; The OV SDK depends on a smaller zip at the moment, we inherit that
	; File "..\dependencies\arch\vcredist-2010.exe"
	; File "..\dependencies\arch\vcredist-2013_x86.exe"
	; ExecWait '"vcredist-2010.exe" /q'
	; ExecWait '"vcredist-2013_x86.exe" /install /quiet'
	File "/oname=vcredist-x.x.zip" "..\dependencies\arch\vcredist.zip"
	
	SetOutPath "$INSTDIR\dependencies"
	
	; Extract all the zip archives
	ClearErrors
	FindFirst $R0 $R1 "arch\*.zip"
	ZipLoop:
		IfErrors ZipDone

		; find the base name, push to R2
		Push "$R1"
		Call GetFirstStrPart
		Pop "$R2"

		ZipDLL::extractall "arch\$R1" "$R2"

		ClearErrors
		FindNext $R0 $R1
		Goto ZipLoop
	ZipDone:
	FindClose $R0

	; Zip extract hopefully done now
	
	SetOutPath "$INSTDIR"
	; Export binaries
	File /nonfatal /r ..\..\dist\extras-Release\bin
	; Export launch scripts
	File /nonfatal ..\..\dist\extras-Release\*.cmd
	; File /nonfatal /r ..\dist\doc
	; File /nonfatal /r ..\dist\etc
	; File /nonfatal /r ..\dist\include
	; File /nonfatal /r ..\dist\lib
	File /nonfatal /r ..\..\dist\extras-Release\log
	File /nonfatal /r ..\..\dist\extras-Release\share
	; File /nonfatal /r ..\dist\tmp

	StrCmp $DIRECTX_MISSING "false" no_need_to_patch_3d_functionnality
	FileOpen $0 "$INSTDIR\share\openvibe\kernel\openvibe.conf" a	
	FileSeek $0 0 END
	FileWrite $0 "$\r$\n"
	FileWrite $0 "#####################################################################################$\r$\n"
	FileWrite $0 "# Patched by installer because DirectX is missing$\r$\n"
	FileWrite $0 "#####################################################################################$\r$\n"
	FileWrite $0 "Kernel_3DVisualisationEnabled = false$\r$\n"
	FileClose $0
no_need_to_patch_3d_functionnality:

	; Overwrite the file that may be in share/, as it contains local definitions to the build machine
	FileOpen $0 "$INSTDIR\bin\openvibe-set-env.cmd" w
	FileWrite $0 "@echo off$\r$\n"
	FileWrite $0 "$\r$\n"

	FileWrite $0 "$\r$\n"
	FileWrite $0 "SET OGRE_HOME=$INSTDIR\dependencies\ogre$\r$\n"
	FileWrite $0 "SET VRPNROOT=$INSTDIR\dependencies\vrpn$\r$\n"
	FileWrite $0 "$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\lua\lib;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\itpp\bin;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\expat\bin;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\gtk\bin;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\itpp\bin;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\cegui\bin;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\cegui\dependencies\bin;%PATH%$\r$\n"	
	FileWrite $0 "SET PATH=%OGRE_HOME%\bin\release;%OGRE_HOME%\bin\debug;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=%VRPNROOT%\bin;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\pthread\lib;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\openal\libs\Win32;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\freealut\lib;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\libvorbis\win32\bin\release;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\libogg\win32\bin\release\;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\liblsl\lib\;%PATH%$\r$\n"
	FileWrite $0 "SET PATH=$INSTDIR\dependencies\vcredist;%PATH%$\r$\n"	
	FileClose $0
	
	FileOpen $0 "$INSTDIR\dependencies\cegui\resources.cfg" w
	FileWrite $0 "FileSystem=$INSTDIR\dependencies\cegui\datafiles\configs$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\dependencies\cegui\datafiles\fonts$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\dependencies\cegui\datafiles\imagesets$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\dependencies\cegui\datafiles\layouts$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\dependencies\cegui\datafiles\looknfeel$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\dependencies\cegui\datafiles\lua_scripts$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\dependencies\cegui\datafiles\schemes$\r$\n"
	FileWrite $0 "FileSystem=$INSTDIR\dependencies\cegui\datafiles\xml_schemes$\r$\n"
	FileClose $0

	FileOpen $0 "$INSTDIR\dependencies\gtk\etc\gtk-2.0\gtkrc" w
	FileWrite $0 "gtk-theme-name = $\"Redmond$\"$\r$\n"
	FileWrite $0 "style $\"user-font$\"$\r$\n"
	FileWrite $0 "{$\r$\n"
	FileWrite $0 "	font_name=$\"Sans 8$\"$\r$\n"
	FileWrite $0 "}$\r$\n"
	FileWrite $0 "widget_class $\"*$\" style $\"user-font$\"$\r$\n"
	FileClose $0

	CreateDirectory "$SMPROGRAMS\OpenViBE-beta"
	CreateDirectory "$SMPROGRAMS\OpenViBE-beta\Developer tools"
	CreateShortCut "$SMPROGRAMS\OpenViBE-beta\Developer tools\openvibe id generator.lnk"       "$INSTDIR\openvibe-id-generator.cmd"        "" "%SystemRoot%\system32\shell32.dll" 57
	CreateShortCut "$SMPROGRAMS\OpenViBE-beta\Developer tools\openvibe plugin inspector.lnk"   "$INSTDIR\openvibe-plugin-inspector.cmd"    "" "%SystemRoot%\system32\shell32.dll" 57
	CreateShortCut "$SMPROGRAMS\OpenViBE-beta\Developer tools\openvibe skeleton generator.lnk" "$INSTDIR\openvibe-skeleton-generator.cmd"  "" "%SystemRoot%\system32\shell32.dll" 57
	CreateShortCut "$SMPROGRAMS\OpenViBE-beta\openvibe designer.lnk"                           "$INSTDIR\openvibe-designer.cmd"            "" "%SystemRoot%\system32\shell32.dll" 137
	CreateShortCut "$SMPROGRAMS\OpenViBE-beta\openvibe acquisition server.lnk"                 "$INSTDIR\openvibe-acquisition-server.cmd"  "" "%SystemRoot%\system32\shell32.dll" 18
	CreateShortCut "$SMPROGRAMS\OpenViBE-beta\openvibe vr-demo spaceship.lnk"                  "$INSTDIR\openvibe-vr-demo-spaceship.cmd"   "" "%SystemRoot%\system32\shell32.dll" 200
	CreateShortCut "$SMPROGRAMS\OpenViBE-beta\openvibe vr-demo handball.lnk"                   "$INSTDIR\openvibe-vr-demo-handball.cmd"    "" "%SystemRoot%\system32\shell32.dll" 200
	CreateShortCut "$SMPROGRAMS\OpenViBE-beta\uninstall.lnk"                                   "$INSTDIR\Uninstall.exe"

	
	; AccessControl::EnableFileInheritance "$INSTDIR"
	; AccessControl::GrantOnFile "$INSTDIR" "(BU)" "GenericRead + GenericWrite + GenericExecute + Delete" ; to ensure windows XP back compatibility
	; AccessControl::GrantOnFile "$INSTDIR" "(S-1-5-32-545)" "GenericRead + GenericWrite + GenericExecute + Delete" ; (BU) user group (builtin users) does not exist on win7. this SID replaces it.
SectionEnd

Section "Uninstall"

	RMDir /r $INSTDIR
	RMDir /r "$SMPROGRAMS\OpenViBE"

SectionEnd

LangString DESC_Section1 ${LANG_ENGLISH} "The OpenViBE package: Designer, Acquisition Server, drivers, examples, etc."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${Section1} $(DESC_Section1)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

