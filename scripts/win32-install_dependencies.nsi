--- scripts/win32-install_dependencies.nsi
+++ scripts/win32-install_dependencies.nsi
@@ -230,6 +230,27 @@
 ;##########################################################################################################################################################
 ;##########################################################################################################################################################
 
+Section "GLFW"
+
+	SetOutPath "$INSTDIR"
+	CreateDirectory "$INSTDIR\arch"
+
+	IfFileExists "arch\glfw-3.0.4-ov.zip" no_need_to_download_boost
+	NSISdl::download http://openvibe.inria.fr/dependencies/win32/glfw-3.0.4-$suffix.zip "arch\glfw-3.0.4-$suffix.zip"
+	Pop $R0 ; Get the return value
+		StrCmp $R0 "success" +3
+			MessageBox MB_OK "Download failed: $R0" /SD IDOK
+			Quit
+no_need_to_download_boost:
+	ZipDLL::extractall "arch\glfw-3.0.4-$suffix.zip" ""
+
+
+SectionEnd
+
+;##########################################################################################################################################################
+;##########################################################################################################################################################
+;##########################################################################################################################################################
+
 Section "GTK+"
 
 	SetOutPath "$INSTDIR"
