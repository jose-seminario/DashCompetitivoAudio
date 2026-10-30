# DashCompetitivoAudio

**DashCompetitivoAudio** es una plataforma de ingeniería acústica de escritorio en **C++20 nativo** y motor DSP multicanal (hasta 16 canales) para Windows x64. Diseñada para la optimización táctica del audio competitivo en juegos de disparos (como *Call of Duty: Warzone*), ofrece ultra baja latencia sin la sobrecarga de entornos basados en Electron/Chromium.

---

## Características Principales

- **Cadena DSP Canónica (`DashAudioCore`)**:
  - Compresor Híbrido ITU-R BS.1770-4 (Upward/Downward con compuerta acústica y soft-knee).
  - Supresor de *Gun Crack* con crossover Linkwitz-Riley configurable (100 a 800 Hz).
  - Protector Ambiental multibanda *AmbienceGuard* (350 Hz a 2.5 kHz).
  - Ecualizador Dual: 31 bandas ISO estándar y ecualizador paramétrico de 10 bandas biquad.
  - Limitador de pico a -0.5 dBFS con canal LFE aislado de fase.
- **Motor Espacial 3D (`DashAudioSpatial`)**:
  - Soporte de hasta 16 canales (Stereo, 5.1, 7.1, 7.1.4 Atmos, 9.1.6).
  - Síntesis analítica Woodworth con cero latencia y modelado de sombra de cabeza (ILD/ITD).
  - Renderizador binaural con bypass automático de HRTF en subgraves (< 120 Hz) para erradicar el *comb-filtering*.
- **Gestión de Perfiles y Calibración (`Profiles` / `DashAudioDB`)**:
  - Serializador y validador JSON nativo seguro contra Path Traversal e inyección de ejecutables.
  - Calibraciones AutoEq certificadas hacia curva Harman Over-Ear 2018 para HyperX y Logitech.
  - Honestidad técnica: perfiles no medidos (ej: Razer BlackShark V2 X) catalogados sin curvas falsas.
  - Biblioteca comunitaria local `DashAudioDB` bajo licencias CC-BY-4.0 y MIT.
- **Interfaz Táctica Profesional (JUCE 8)**:
  - Tema táctico oscuro `DashLookAndFeel` (#121214, #1A1A1E, acentos cian/azul).
  - Conmutador **Easy Mode** (jugador) y **Advanced Mode** (ingeniero).
  - Monitoreo de cadena visual interactiva de 8 nodos con telemetría desacoplada a 30 FPS.
- **Suite de Pruebas y Benchmark (`Tools/DashAudioBench`)**:
  - Medición de CPU (percentiles p50, p95, p99), latencia de búfer y calidad acústica (THD, clipping).
  - Comparación A/B con compensación automática de sonoridad (*Loudness Matching*).

---

## Arquitectura del Proyecto

```text
DashCompetitivoAudio/
├── CMakeLists.txt              # Build raíz (C++20, AVX2, LTO/LTCG, FetchContent JUCE 8.0.4)
├── LICENSE                     # Licencia GNU General Public License v3.0
├── THIRD_PARTY_NOTICES.md      # Licencias de componentes de terceros
├── ARCHITECTURE.md             # Documentación exhaustiva de arquitectura de datos
├── CONTRIBUTING.md             # Guía de contribución y código en tiempo real
├── CODE_OF_CONDUCT.md          # Código de conducta de la comunidad
├── SECURITY.md                 # Política de seguridad y reporte de vulnerabilidades
├── CHANGELOG.md                # Registro de versiones (SemVer)
├── INSTALLATION.md             # Guía de instalación y compilación
├── TESTING.md                  # Guía de suites de pruebas
├── BENCHMARK.md                # Metodología de análisis de latencia y CPU
│
├── DashAudioCore/              # Motor DSP Central Multicanal (1 a 16 canales)
├── DashAudioSpatial/           # Motor Espacial 3D, HRTF/HRIR y Renderizador Binaural
├── Profiles/                   # Esquemas JSON Draft 2020-12 y perfiles de fábrica
├── DashAudioDB/                # Biblioteca comunitaria local abierta
├── App/                        # Aplicación de escritorio JUCE con UI profesional
├── Tools/DashAudioBench/       # Herramienta CLI/Benchmarking y QA A/B
├── Tests/                      # Suites de pruebas preparadas por módulo
├── Installer/                  # Script NSIS para instalador comercial seguro
└── docs/                       # Matriz de compatibilidad, SBOM y Project Maturity
```

---

## Compilación Rápida en Windows

Abre una consola *Developer Command Prompt for VS 2022* o PowerShell con CMake:

```powershell
# 1. Configurar solución CMake
cmake -B build -G "Visual Studio 17 2022" -A x64

# 2. Compilar en modo Release optimizado
cmake --build build --config Release --parallel
```

Los binarios e instalador quedarán listos en `build\App` y `build\Installer`.

---

## Licencia

- **Código Fuente**: Licenciado bajo la [GNU General Public License v3.0 (GPLv3)](LICENSE).
- **Perfiles y Calibraciones de DashAudioDB**: Licenciados bajo [Creative Commons Attribution 4.0 International (CC-BY-4.0)](DashAudioDB/LICENSES.md).
- **Filtros AutoEq**: Licenciados bajo la [Licencia MIT](THIRD_PARTY_NOTICES.md).
