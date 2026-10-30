; ==============================================================================
; Dash Competitivo Audio - Script Oficial de Instalación Comercial (NSIS 3.x)
; Reglas de Auditoría de Seguridad (Fase 7):
;   1. Instalar ÚNICAMENTE archivos propios de DashCompetitivoAudio.
;   2. NO instalar controladores de terceros automáticamente (cero drivers forzados).
;   3. NO sobrescribir plugins ajenos en directorios VST compartidos.
;   4. NO vaciar ni alterar las configuraciones preexistentes de Equalizer APO (config.txt).
;   5. Preservar intactos los perfiles personales y calibraciones del usuario.
;   6. Desinstalación segura y limpia: eliminar exclusivamente lo instalado por Dash.
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

; Solicitar permisos de Administrador (UAC) para registrar VST3 y claves de desinstalación
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

    SetShellVarContext current

    ; 1. Instalar ejecutable propio en %LOCALAPPDATA%\DashCompetitivoAudio
    SetOutPath "$INSTDIR"
    File "${BUILD_DIR}\App\DashCompetitivoAudio_artefacts\Release\${APP_EXE}"

    ; 2. Instalar biblioteca comunitaria y esquemas propios
    SetOutPath "$INSTDIR\Profiles\Schema"
    File /nonfatal "..\Profiles\Schema\*.json"

    SetOutPath "$INSTDIR\Profiles\Games\Warzone\Competitive"
    File /nonfatal "..\Profiles\Games\Warzone\Competitive\*.json"
    SetOutPath "$INSTDIR\Profiles\Games\Warzone\Balanced"
    File /nonfatal "..\Profiles\Games\Warzone\Balanced\*.json"
    SetOutPath "$INSTDIR\Profiles\Games\Warzone\Immersive"
    File /nonfatal "..\Profiles\Games\Warzone\Immersive\*.json"

    SetOutPath "$INSTDIR\Profiles\Headphones\Razer"
    File /nonfatal "..\Profiles\Headphones\Razer\*.json"
    SetOutPath "$INSTDIR\Profiles\Headphones\HyperX"
    File /nonfatal "..\Profiles\Headphones\HyperX\*.json"
    SetOutPath "$INSTDIR\Profiles\Headphones\Logitech"
    File /nonfatal "..\Profiles\Headphones\Logitech\*.json"

    ; 3. Despliegue de los 3 plugins .vst3 propios en recursos locales
    SetOutPath "$INSTDIR\resources\apo-vst"
    File /r "${BUILD_DIR}\Plugins\LoudnessCompressor\LoudnessCompressor_artefacts\Release\VST3\LoudnessCompressor.vst3"
    File /r "${BUILD_DIR}\Plugins\TransientSuppressor\TransientSuppressor_artefacts\Release\VST3\TransientSuppressor.vst3"
    File /r "${BUILD_DIR}\Plugins\AmbienceGuard\AmbienceGuard_artefacts\Release\VST3\AmbienceGuard.vst3"

    ; 4. Despliegue en la carpeta estándar del sistema para VST3 (%COMMONPROGRAMFILES%\VST3)
    CreateDirectory "$COMMONFILES64\VST3"
    CopyFiles /SILENT "$INSTDIR\resources\apo-vst\LoudnessCompressor.vst3" "$COMMONFILES64\VST3"
    CopyFiles /SILENT "$INSTDIR\resources\apo-vst\TransientSuppressor.vst3" "$COMMONFILES64\VST3"
    CopyFiles /SILENT "$INSTDIR\resources\apo-vst\AmbienceGuard.vst3" "$COMMONFILES64\VST3"

    ; 5. Si Equalizer APO está presente, copiar exclusivamente los 3 bundles propios en VSTPlugins
    ReadRegStr $R0 HKLM "SOFTWARE\EqualizerAPO" "InstallPath"
    ${If} $R0 == ""
        StrCpy $R0 "$PROGRAMFILES64\EqualizerAPO"
    ${EndIf}
    StrCpy $R1 "$R0\VSTPlugins"
    ${If} ${FileExists} "$R0"
        CreateDirectory "$R1"
        CopyFiles /SILENT "$INSTDIR\resources\apo-vst\LoudnessCompressor.vst3" "$R1"
        CopyFiles /SILENT "$INSTDIR\resources\apo-vst\TransientSuppressor.vst3" "$R1"
        CopyFiles /SILENT "$INSTDIR\resources\apo-vst\AmbienceGuard.vst3" "$R1"
    ${EndIf}

    ; 6. Claves de registro de usuario y de máquina
    WriteRegStr HKCU "Software\${APP_SHORT_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "Software\${APP_SHORT_KEY}" "Version"         "${APP_VERSION}"
    WriteRegStr HKCU "Software\${APP_SHORT_KEY}" "Publisher"       "${APP_COMPANY}"
    WriteRegStr HKCU "Software\${APP_SHORT_KEY}" "ApoVstDir"       "$R1"

    WriteRegStr HKLM "SOFTWARE\${APP_SHORT_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "SOFTWARE\${APP_SHORT_KEY}" "Version"         "${APP_VERSION}"
    WriteRegStr HKLM "SOFTWARE\${APP_SHORT_KEY}" "Publisher"       "${APP_COMPANY}"
    WriteRegStr HKLM "SOFTWARE\${APP_SHORT_KEY}" "ApoVstDir"       "$R1"

    ; Registro del Desinstalador oficial en Configuración de Windows
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayName"          "${APP_NAME}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "UninstallString"      "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayIcon"          "$INSTDIR\${APP_EXE}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "Publisher"            "${APP_COMPANY}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayVersion"       "${APP_VERSION}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "InstallLocation"      "$INSTDIR"
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify"             1
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair"             1

    ; 7. Accesos directos limpios
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
    SetShellVarContext current

    ; 1. Eliminar EXCLUSIVAMENTE los 3 bundles propios en EqualizerAPO\VSTPlugins (sin vaciar config.txt ni tocar otros plugins)
    ReadRegStr $R1 HKLM "SOFTWARE\${APP_SHORT_KEY}" "ApoVstDir"
    ${If} $R1 != ""
        RMDir /r "$R1\LoudnessCompressor.vst3"
        RMDir /r "$R1\TransientSuppressor.vst3"
        RMDir /r "$R1\AmbienceGuard.vst3"
        Delete "$R1\LoudnessCompressor.dll"
        Delete "$R1\TransientSuppressor.dll"
        Delete "$R1\AmbienceGuard.dll"
    ${EndIf}

    ; 2. Eliminar únicamente los bundles propios de %COMMONPROGRAMFILES%\VST3
    RMDir /r "$COMMONFILES64\VST3\LoudnessCompressor.vst3"
    RMDir /r "$COMMONFILES64\VST3\TransientSuppressor.vst3"
    RMDir /r "$COMMONFILES64\VST3\AmbienceGuard.vst3"

    ; 3. Eliminar accesos directos
    Delete "$DESKTOP\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Desinstalar ${APP_NAME}.lnk"
    RMDir  "$SMPROGRAMS\${APP_NAME}"

    ; 4. Eliminar ejecutables y recursos propios de la aplicación
    RMDir /r "$INSTDIR\resources"
    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\Uninstall.exe"

    ; IMPORTANTE: NO borrar la carpeta de perfiles personales si el usuario creó perfiles propios
    ${If} ${FileExists} "$INSTDIR\Profiles\User"
        ; Se preservan los perfiles personales del usuario
    ${Else}
        RMDir /r "$INSTDIR\Profiles"
    ${EndIf}

    RMDir "$INSTDIR" ; Solo se borra si quedó vacía

    DeleteRegKey HKCU "Software\${APP_SHORT_KEY}"
    DeleteRegKey HKLM "SOFTWARE\${APP_SHORT_KEY}"
    DeleteRegKey HKLM "${UNINSTALL_KEY}"
SectionEnd
