Unicode True
!include "version.nsh"
!include "MUI2.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
Name "Thrusty"
OutFile "dist/Thrusty-${THRUSTY_VERSION}-Setup.exe"
InstallDir "$LOCALAPPDATA\Programs\Thrusty"
RequestExecutionLevel user
SetCompressor /SOLID lzma
!define MUI_ICON "src/thrusty.ico"
!define MUI_UNICON "src/thrusty.ico"
!define MUI_WELCOMEPAGE_TEXT "Thrusty maps a Thrustmaster wheel to an Xbox 360 controller, with optional rumble-based wheel feedback.$\r$\n$\r$\nThrusty requires Windows 10/11 x64, the Thrustmaster driver and ViGEmBus.$\r$\n$\r$\nAfter installation, open Feedback & setup to install the bundled ViGEmBus driver. Driver installation requests administrator approval separately."
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.html"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Open setup and calibration guide"
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
Function .onInit
 System::Call 'kernel32::OpenMutexW(i 0x100000, i 0, w "Local\Thrusty.ControlPanel") p.r0'
 StrCmp $0 0 +4
 System::Call 'kernel32::CloseHandle(p r0)'
 MessageBox MB_ICONEXCLAMATION "Close Thrusty before continuing."
 Abort
 ${IfNot} ${AtLeastWin10}
  MessageBox MB_ICONSTOP "Thrusty requires Windows 10 or later."
  Abort
 ${EndIf}
 ${IfNot} ${RunningX64}
  MessageBox MB_ICONSTOP "Thrusty requires 64-bit Windows 10 or 11."
  Abort
 ${EndIf}
FunctionEnd
Section "Thrusty"
 SetOutPath "$INSTDIR"
 File "bin/Thrusty.exe"
 File "ViGEmClient.dll"
 File "README.html"
 File "LICENSE.txt"
 File "VALIDATION.txt"
 File "CREDITS.md"
 File /r "licenses"
 File /r "drivers"
 CreateDirectory "$SMPROGRAMS\Thrusty"
 CreateShortcut "$SMPROGRAMS\Thrusty\Thrusty.lnk" "$INSTDIR\Thrusty.exe"
 CreateShortcut "$SMPROGRAMS\Thrusty\Setup Guide.lnk" "$INSTDIR\README.html"
 CreateShortcut "$DESKTOP\Thrusty.lnk" "$INSTDIR\Thrusty.exe"
 WriteUninstaller "$INSTDIR\Uninstall.exe"
 WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Thrusty" "DisplayName" "Thrusty"
 WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Thrusty" "DisplayVersion" "${THRUSTY_VERSION}"
 WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Thrusty" "UninstallString" '"$INSTDIR\Uninstall.exe"'
 WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Thrusty" "DisplayIcon" "$INSTDIR\Thrusty.exe"
 WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Thrusty" "NoModify" 1
 WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Thrusty" "NoRepair" 1
SectionEnd
Function un.onInit
 System::Call 'kernel32::OpenMutexW(i 0x100000, i 0, w "Local\Thrusty.ControlPanel") p.r0'
 StrCmp $0 0 +4
 System::Call 'kernel32::CloseHandle(p r0)'
 MessageBox MB_ICONEXCLAMATION "Close Thrusty before continuing."
 Abort
FunctionEnd
Section "Uninstall"
 Delete "$INSTDIR\Thrusty.exe"
 Delete "$INSTDIR\ViGEmClient.dll"
 Delete "$INSTDIR\README.html"
 Delete "$INSTDIR\LICENSE.txt"
 Delete "$INSTDIR\VALIDATION.txt"
 Delete "$INSTDIR\CREDITS.md"
 Delete "$INSTDIR\licenses\ViGEmClient-LICENSE.txt"
 Delete "$INSTDIR\licenses\ViGEmBus-LICENSE.txt"
 Delete "$INSTDIR\licenses\THIRD-PARTY.txt"
 RMDir "$INSTDIR\licenses"
 Delete "$INSTDIR\drivers\ViGEmBus_1.22.0.exe"
 RMDir "$INSTDIR\drivers"
 Delete "$INSTDIR\Uninstall.exe"
 RMDir "$INSTDIR"
 Delete "$DESKTOP\Thrusty.lnk"
 Delete "$SMPROGRAMS\Thrusty\Thrusty.lnk"
 Delete "$SMPROGRAMS\Thrusty\Setup Guide.lnk"
 RMDir "$SMPROGRAMS\Thrusty"
 DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Thrusty"
SectionEnd
