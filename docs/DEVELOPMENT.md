# Guía del Entorno de Desarrollo - Dash Competitivo Audio

Esta guía detalla los pasos para configurar, compilar y depurar **Dash Competitivo Audio** en estaciones de trabajo con Windows 10/11 x64.

---

## 1. Requisitos de Herramientas

1. **Compilador:** Visual Studio 2022 (Community, Professional o Enterprise) con la carga de trabajo **Desarrollo para el escritorio con C++** (Toolset MSVC v143 y Windows 10/11 SDK).
2. **Generador de Construcción:** CMake 3.22 o superior añadido al `PATH`.
3. **Control de Versiones:** Git 2.30+ en el `PATH`.
4. *(Opcional)* **NSIS 3.x:** Nullsoft Scriptable Install System para compilar el instalador `DashCompetitivoAudio-setup.exe`.

---

## 2. Configuración y Compilación Local

Abre una terminal *x64 Native Tools Command Prompt for VS 2022* o PowerShell:

### 2.1. Configuración de CMake
```powershell
# Desde la raíz del repositorio:
cmake -B build -G "Visual Studio 17 2022" -A x64
```
*Nota:* En la primera ejecución, CMake descargará automáticamente **JUCE 8.0.4** mediante `FetchContent` en la carpeta `build/_deps/juce-src`. Si ya tienes una copia local de JUCE para ahorrar ancho de banda y tiempo, pasa el argumento:
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_SOURCE_DIR="C:/Ruta/A/JUCE"
```

### 2.2. Compilación en Modo Release
```powershell
cmake --build build --config Release --parallel
```

### 2.3. Compilación de Modo Debug
```powershell
cmake --build build --config Debug --parallel
```

---

## 3. Depuración y Pruebas Manuales de Plugins

Los tres plugins se compilan simultáneamente en dos formatos:
1. **VST3 (`.vst3`):** Para su uso en hosts de producción o dentro de Equalizer APO.
2. **Standalone (`.exe`):** Ejecutable independiente con interfaz de audio WASAPI/DirectSound integrada, ideal para depurar algoritmos DSP en tiempo real sin requerir un DAW ni Equalizer APO:
   - `build/Plugins/LoudnessCompressor/LoudnessCompressor_artefacts/Release/Standalone/LoudnessCompressor.exe`
   - `build/Plugins/TransientSuppressor/TransientSuppressor_artefacts/Release/Standalone/TransientSuppressor.exe`
   - `build/Plugins/AmbienceGuard/AmbienceGuard_artefacts/Release/Standalone/AmbienceGuard.exe`

---

## 4. Estructura de Directorios del Código

- `DashAudioCore/`: Motor matemático canónico y algoritmos de procesamiento.
- `App/`: Aplicación principal de control, orquestación COM WASAPI y generación atómica de `config.txt`.
- `Plugins/`: Envoltorios JUCE VST3 y Standalone que exponen los procesadores de `DashAudioCore` a Equalizer APO.
- `Installer/`: Script NSIS para empaquetar el binario principal y desplegar los plugins en `C:\Program Files\EqualizerAPO\VSTPlugins`.
- `.github/workflows/`: Automatización de compilación e integración continua (CI/CD) en corredores Windows.
