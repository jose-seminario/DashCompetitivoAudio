; ==============================================================================
; Dash Competitivo Audio - Script Oficial de Instalación (NSIS 3.x)
; Arquitectura de despliegue del motor táctico:
;   - Instalación de aplicación en %LOCALAPPDATA%\DashCompetitivoAudio
;   - Despliegue automático de los 3 plugins .vst3 en EqualizerAPO\VSTPlugins
;   - Registro en HKCU\Software\DashCompetitivoAudio y HKLM\SOFTWARE\DashCompetitivoAudio
;   - Elevación UAC (RequestExecutionLevel admin) y accesos directos completos
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

; Directorio de instalación por defecto (%LOCALAPPDATA%\DashCompetitivoAudio)
InstallDir "$LOCALAPPDATA\${APP_SHORT_KEY}"
InstallDirRegKey HKCU "Software\${APP_SHORT_KEY}" "InstallLocation"

; Solicitar permisos de Administrador (UAC) para escribir en Program Files\EqualizerAPO\VSTPlugins y HKLM
RequestExecutionLevel admin

BrandingText "${APP_NAME} v${APP_VERSION} - Tactical Audio Engine"

; ------------------------------------------------------------------------------
; Configuración de Interfaz Moderna (MUI2)
; ------------------------------------------------------------------------------
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

; ------------------------------------------------------------------------------
; Inicialización en 64 bits
; ------------------------------------------------------------------------------
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
Section "Aplicación Principal y Motores DSP VST3" SecCore
    SectionIn RO

    ; Asegurar que los accesos directos del Menú de Inicio se creen para el usuario actual/global coherentemente
    SetShellVarContext current

    ; 1. Instalar ejecutable principal en %LOCALAPPDATA%\DashCompetitivoAudio
    SetOutPath "$INSTDIR"
    File "${BUILD_DIR}\App\DashCompetitivoAudio_artefacts\Release\${APP_EXE}"

    ; 2. Empaquetar y extraer los 3 plugins .vst3 en %LOCALAPPDATA%\DashCompetitivoAudio\resources\apo-vst
    SetOutPath "$INSTDIR\resources\apo-vst"
    File /r "${BUILD_DIR}\Plugins\LoudnessCompressor\LoudnessCompressor_artefacts\Release\VST3\LoudnessCompressor.vst3"
    File /r "${BUILD_DIR}\Plugins\TransientSuppressor\TransientSuppressor_artefacts\Release\VST3\TransientSuppressor.vst3"
    File /r "${BUILD_DIR}\Plugins\AmbienceGuard\AmbienceGuard_artefacts\Release\VST3\AmbienceGuard.vst3"

    ; 3. Copiar los 3 plugins .vst3 a la carpeta de plugins de Equalizer APO (C:\Program Files\EqualizerAPO\VSTPlugins)
    ReadRegStr $R0 HKLM "SOFTWARE\EqualizerAPO" "InstallPath"
    ${If} $R0 == ""
        StrCpy $R0 "$PROGRAMFILES64\EqualizerAPO"
    ${EndIf}
    StrCpy $R1 "$R0\VSTPlugins"
    CreateDirectory "$R1"

    CopyFiles /SILENT "$INSTDIR\resources\apo-vst\LoudnessCompressor.vst3" "$R1"
    CopyFiles /SILENT "$INSTDIR\resources\apo-vst\TransientSuppressor.vst3" "$R1"
    CopyFiles /SILENT "$INSTDIR\resources\apo-vst\AmbienceGuard.vst3" "$R1"

    ; Copia directa de las DLLs PE32+ internas para compatibilidad inmediata con directivas VSTPlugin de Equalizer APO
    ${If} ${FileExists} "$INSTDIR\resources\apo-vst\LoudnessCompressor.vst3\Contents\x86_64-win\LoudnessCompressor.vst3"
        CopyFiles /SILENT "$INSTDIR\resources\apo-vst\LoudnessCompressor.vst3\Contents\x86_64-win\LoudnessCompressor.vst3" "$R1\LoudnessCompressor.dll"
    ${EndIf}
    ${If} ${FileExists} "$INSTDIR\resources\apo-vst\TransientSuppressor.vst3\Contents\x86_64-win\TransientSuppressor.vst3"
        CopyFiles /SILENT "$INSTDIR\resources\apo-vst\TransientSuppressor.vst3\Contents\x86_64-win\TransientSuppressor.vst3" "$R1\TransientSuppressor.dll"
    ${EndIf}
    ${If} ${FileExists} "$INSTDIR\resources\apo-vst\AmbienceGuard.vst3\Contents\x86_64-win\AmbienceGuard.vst3"
        CopyFiles /SILENT "$INSTDIR\resources\apo-vst\AmbienceGuard.vst3\Contents\x86_64-win\AmbienceGuard.vst3" "$R1\AmbienceGuard.dll"
    ${EndIf}

    ; 4. Crear claves de registro en HKCU y HKLM
    WriteRegStr HKCU "Software\${APP_SHORT_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "Software\${APP_SHORT_KEY}" "Version"         "${APP_VERSION}"
    WriteRegStr HKCU "Software\${APP_SHORT_KEY}" "Publisher"       "${APP_COMPANY}"
    WriteRegStr HKCU "Software\${APP_SHORT_KEY}" "ApoVstDir"       "$R1"

    WriteRegStr HKLM "SOFTWARE\${APP_SHORT_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "SOFTWARE\${APP_SHORT_KEY}" "Version"         "${APP_VERSION}"
    WriteRegStr HKLM "SOFTWARE\${APP_SHORT_KEY}" "Publisher"       "${APP_COMPANY}"
    WriteRegStr HKLM "SOFTWARE\${APP_SHORT_KEY}" "ApoVstDir"       "$R1"

    ; Registro del Desinstalador en Panel de Control / Configuración de Windows
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayName"          "${APP_NAME}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "UninstallString"      "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayIcon"          "$INSTDIR\${APP_EXE}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "Publisher"            "${APP_COMPANY}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayVersion"       "${APP_VERSION}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "InstallLocation"      "$INSTDIR"
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify"             1
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair"             1

    ; 5. Crear accesos directos en Escritorio y Menú de Inicio (con directorio de inicio en $INSTDIR)
    SetOutPath "$INSTDIR"
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortcut  "$SMPROGRAMS\${APP_NAME}\Desinstalar ${APP_NAME}.lnk" "$INSTDIR\Uninstall.exe"
    CreateShortcut  "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
SectionEnd

; ------------------------------------------------------------------------------
; Sección de Desinstalación Limpia
; ------------------------------------------------------------------------------
Section "Uninstall"
    SetShellVarContext current

    ReadRegStr $R1 HKLM "SOFTWARE\${APP_SHORT_KEY}" "ApoVstDir"
    ${If} $R1 != ""
        RMDir /r "$R1\LoudnessCompressor.vst3"
        RMDir /r "$R1\TransientSuppressor.vst3"
        RMDir /r "$R1\AmbienceGuard.vst3"
        Delete "$R1\LoudnessCompressor.dll"
        Delete "$R1\TransientSuppressor.dll"
        Delete "$R1\AmbienceGuard.dll"
    ${EndIf}

    Delete "$DESKTOP\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Desinstalar ${APP_NAME}.lnk"
    RMDir  "$SMPROGRAMS\${APP_NAME}"

    RMDir /r "$INSTDIR\resources"
    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir  "$INSTDIR"

    DeleteRegKey HKCU "Software\${APP_SHORT_KEY}"
    DeleteRegKey HKLM "SOFTWARE\${APP_SHORT_KEY}"
    DeleteRegKey HKLM "${UNINSTALL_KEY}"
SectionEnd
