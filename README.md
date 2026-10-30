# DashCompetitivoAudio

Aplicación de escritorio en **C++20 nativo** y motor DSP **VST3/Standalone (JUCE 8)** diseñada para procesamiento de audio competitivo de ultra baja latencia en Windows sin la sobrecarga de Electron/Chromium.

## Arquitectura del Proyecto

```text
DashCompetitivoAudio/
├── CMakeLists.txt                                      # Build raíz (C++20, AVX2, LTCG, FetchContent JUCE 8.0.4)
├── App/                                                # Aplicación de control nativa (GUI + COM + Equalizer APO)
│   ├── CMakeLists.txt
│   └── Source/
│       ├── Main.cpp
│       ├── MainComponent.h / .cpp
│       └── Core/
│           ├── AudioPolicyManager.h / .cpp             # Orquestador COM IPolicyConfig (eConsole + eMultimedia)
│           └── ApoConfigInjector.h / .cpp              # Escritor atómico NTFS para Equalizer APO config.txt
└── Plugins/
    └── LoudnessCompressor/                             # Plugin VST3 / Standalone de Compresión Híbrida
        ├── CMakeLists.txt
        └── Source/
            ├── PluginProcessor.h / .cpp                # APVTS + Buses 2.0 / 5.1 / 7.1
            ├── PluginEditor.h / .cpp                   # UI rotativa + telemetría LUFS a 30 Hz
            └── DSP/
                ├── KWeightingFilter.h                  # Ponderación K oficial ITU-R BS.1770-4 (Pre-Filter + RLB)
                └── HybridLoudnessCompressor.h          # Motor RMS BS.1770 + Upward/Downward + Peak Limiter
```

## Requisitos de Compilación en Windows

1. **Visual Studio 2022** con la carga de trabajo **Desarrollo para el escritorio con C++** (MSVC v143 + Windows 10/11 SDK).
2. **CMake 3.22+** y **Git** en el `PATH`.

## Comandos de Compilación

Abre una terminal PowerShell o *x64 Native Tools Command Prompt for VS 2022*:

```powershell
cd C:\Users\USUARIO\Documents\Proyectos\DashCompetitivoAudio

# 1. Generar solución Visual Studio 2022 x64 (descarga JUCE 8.0.4 automáticamente la primera vez)
cmake -B build -G "Visual Studio 17 2022" -A x64

# 2. Compilar en modo Release optimizado (/O2 /GL /arch:AVX2)
cmake --build build --config Release --parallel
```

*(Opcional)* Si ya dispones de una copia local de JUCE en disco para evitar la descarga inicial:
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_SOURCE_DIR="C:/Ruta/A/JUCE"
```

## Artefactos Generados

- **Aplicación Principal**:
  `build\App\DashCompetitivoAudio_artefacts\Release\Dash Competitivo Audio.exe`
- **Plugin VST3**:
  `build\Plugins\LoudnessCompressor\LoudnessCompressor_artefacts\Release\VST3\LoudnessCompressor.vst3`
