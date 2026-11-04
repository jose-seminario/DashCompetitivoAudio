; ==============================================================================
; Dash Competitivo Audio - Script Oficial de Instalación Comercial (NSIS 3.x)
; Ecualizador Táctico Estéreo de 6 Bandas (Sin VST3, sin perfiles JSON externos)
; ==============================================================================

Unicode True
SetCompressor /SOLID lzma

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

!define APP_NAME        "Dash Competitivo Audio"
!define APP_SHORT_KEY   "DashCompetitivoAudio"
!define APP_EXE         "Dash Competitivo Audio.exe"
!define APP_COMPANY     "DashAudio"
!define APP_VERSION     "1.0.0"
!define UNINSTALL_KEY   "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${APP_SHORT_KEY}"

!ifndef BUILD_DIR
    !define BUILD_DIR "..\build"
!endif

!ifndef OUT_FILE
    !define OUT_FILE "${BUILD_DIR}\DashCompetitivoAudio-setup.exe"
!endif

Name "${APP_NAME} ${APP_VERSION}"
OutFile "${OUT_FILE}"

Icon "icon.ico"
UninstallIcon "icon.ico"
!define MUI_ICON "icon.ico"
!define MUI_UNICON "icon.ico"

InstallDir "$PROGRAMFILES64\${APP_SHORT_KEY}"
InstallDirRegKey HKLM "Software\${APP_SHORT_KEY}" "InstallLocation"

RequestExecutionLevel admin

BrandingText "${APP_NAME} v${APP_VERSION} - Tactical Stereo EQ"

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Iniciar ${APP_NAME}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "Spanish"

Function .onInit
    ${If} ${RunningX64}
        SetRegView 64
    ${Else}
        MessageBox MB_ICONSTOP "${APP_NAME} requiere Windows de 64 bits (x64)."
        Abort
    ${EndIf}
FunctionEnd

Function un.onInit
    ${If} ${RunningX64}
        SetRegView 64
    ${EndIf}
FunctionEnd

; ------------------------------------------------------------------------------
; Sección Principal de Instalación
; ------------------------------------------------------------------------------
Section "Ecualizador Táctico Estéreo" SecCore
    SectionIn RO

    SetShellVarContext all

    SetOutPath "$INSTDIR"
    File "${BUILD_DIR}\App\DashCompetitivoAudio_artefacts\Release\${APP_EXE}"

    WriteRegStr HKCU "Software\${APP_SHORT_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "Software\${APP_SHORT_KEY}" "Version"         "${APP_VERSION}"
    WriteRegStr HKCU "Software\${APP_SHORT_KEY}" "Publisher"       "${APP_COMPANY}"

    WriteRegStr HKLM "SOFTWARE\${APP_SHORT_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "SOFTWARE\${APP_SHORT_KEY}" "Version"         "${APP_VERSION}"
    WriteRegStr HKLM "SOFTWARE\${APP_SHORT_KEY}" "Publisher"       "${APP_COMPANY}"

    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayName"          "${APP_NAME}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "UninstallString"      "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayIcon"          "$INSTDIR\${APP_EXE}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "Publisher"            "${APP_COMPANY}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayVersion"       "${APP_VERSION}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "InstallLocation"      "$INSTDIR"
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify"             1
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair"             1

    SetOutPath "$INSTDIR"
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortcut  "$SMPROGRAMS\${APP_NAME}\Desinstalar ${APP_NAME}.lnk" "$INSTDIR\Uninstall.exe"
    CreateShortcut  "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
SectionEnd

; ------------------------------------------------------------------------------
; Sección de Desinstalación Segura
; ------------------------------------------------------------------------------
Section "Uninstall"
    SetShellVarContext all

    ; Limpiar bloque administrado de Dash en config.txt de Equalizer APO sin alterar filtros ajenos
    ReadRegStr $R0 HKLM "SOFTWARE\EqualizerAPO" "InstallPath"
    ${If} $R0 == ""
        StrCpy $R0 "$PROGRAMFILES64\EqualizerAPO"
    ${EndIf}
    StrCpy $R2 "$R0\config\config.txt"
    ${If} ${FileExists} "$R2"
        nsExec::Exec 'powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$$p = ''$R2''; if (Test-Path $$p) { $$t = [System.IO.File]::ReadAllText($$p); $$r = [System.Text.RegularExpressions.Regex]::Replace($$t, ''(?s)\r?\n?# BEGIN DASHCOMPETITIVOAUDIO MANAGED BLOCK.*?# END DASHCOMPETITIVOAUDIO MANAGED BLOCK\r?\n?'', ''''); [System.IO.File]::WriteAllText($$p, $$r) }"'
    ${EndIf}

    Delete "$DESKTOP\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Desinstalar ${APP_NAME}.lnk"
    RMDir  "$SMPROGRAMS\${APP_NAME}"

    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir  "$INSTDIR"

    DeleteRegKey HKCU "Software\${APP_SHORT_KEY}"
    DeleteRegKey HKLM "SOFTWARE\${APP_SHORT_KEY}"
    DeleteRegKey HKLM "${UNINSTALL_KEY}"
SectionEnd
