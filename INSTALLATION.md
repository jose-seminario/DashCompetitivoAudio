# Guía de Instalación y Despliegue — DashCompetitivoAudio

Esta guía detalla los métodos oficiales para instalar y compilar **DashCompetitivoAudio** en sistemas Windows 10 y Windows 11 de 64 bits (x64).

---

## 1. Instalación mediante Instalador Comercial (Recomendado)

1. Descarga la versión más reciente del instalador oficial: `DashCompetitivoAudio-setup.exe` desde la sección de Releases.
2. Ejecuta el instalador con permisos de Administrador (solicitará elevación UAC para registrar la directiva de Equalizer APO y claves de desinstalación).
3. El instalador:
   - Desplegará la aplicación en `%LOCALAPPDATA%\DashCompetitivoAudio`.
   - Copiará los plugins VST3 en `%COMMONPROGRAMFILES%\VST3` y en `Program Files\EqualizerAPO\VSTPlugins` (si Equalizer APO está presente).
   - Creará accesos directos en el Menú de Inicio y Escritorio.
   - **No alterará ni vaciará** tus ecualizaciones previas en `config.txt`.
   - **No forzará** la instalación de controladores virtuales de terceros.

---

## 2. Compilación Manual desde Código Fuente

### Requisitos del Sistema
- **Sistema Operativo**: Windows 10 (21H2+) o Windows 11 x64.
- **Compilador**: Visual Studio 2022 (MSVC v143) con la carga de trabajo "Desarrollo para el escritorio con C++".
- **CMake**: Versión 3.22 o superior en el `PATH`.
- **Git**: Para clonar el repositorio y descargar JUCE mediante FetchContent.
- **NSIS 3.x** *(Opcional)*: Para empaquetar el instalador `.exe`.

### Pasos de Compilación
Abre una terminal PowerShell o *x64 Native Tools Command Prompt for VS 2022*:

```powershell
# 1. Clonar el repositorio
git clone https://github.com/jose-seminario/DashCompetitivoAudio.git
cd DashCompetitivoAudio

# 2. Configurar la solución CMake con soporte AVX2 (descarga JUCE 8.0.4 automáticamente)
cmake -B build -G "Visual Studio 17 2022" -A x64

# 3. Compilar en modo Release optimizado
cmake --build build --config Release --parallel
```

### Artefactos Generados
- Aplicación Principal: `build\App\DashCompetitivoAudio_artefacts\Release\Dash Competitivo Audio.exe`
- Herramienta de Benchmark: `build\Tools\DashAudioBench\Release\DashAudioBench.exe`
- Plugins VST3: `build\Plugins\*\Release\VST3\*.vst3`

---

## 3. Desinstalación Limpia
Para desinstalar:
- Abre *Configuración de Windows > Aplicaciones instaladas > Dash Competitivo Audio > Desinstalar*, o ejecuta `Uninstall.exe` en la carpeta de la aplicación.
- El desinstalador retirará exclusivamente los binarios instalados por DashCompetitivoAudio.
- **Tus perfiles personalizados** en `%LOCALAPPDATA%\DashCompetitivoAudio\Profiles` se conservarán para no perder calibraciones acústicas personales.
