# Guía de Pruebas y Validación Acústica — DashCompetitivoAudio

**DashCompetitivoAudio** cuenta con una arquitectura de pruebas exhaustiva organizada por subsistemas en el directorio `Tests/`.

---

## 1. Estructura de Suites de Pruebas

```text
Tests/
├── DashAudioCore/              # Pruebas de algoritmos de procesamiento DSP
│   ├── DynamicsProcessorTests.cpp   # Ponderación K, soft-knee, upward/downward
│   ├── TransientProcessorTests.cpp  # Crossover Linkwitz-Riley, supresión de disparos
│   ├── AmbienceProcessorTests.cpp   # Atenuación multibanda de ruido de fondo
│   ├── EQProcessorTests.cpp         # Ecualizador de 31 bandas ISO y paramétrico
│   ├── LimiterProcessorTests.cpp    # Limitador de pico a -0.5 dBFS y canal LFE
│   └── AudioGraphTests.cpp          # Integración de la cadena completa de 16 canales
│
├── DashAudioSpatial/           # Pruebas del motor espacial 3D
│   ├── ChannelLayoutTests.cpp       # Coordenadas esféricas y máscaras Windows
│   ├── ChannelMappingTests.cpp      # Matrices de mapeo y downmix
│   ├── ConvolutionTests.cpp         # Convolución directa FIR y doble búfer lock-free
│   ├── HRIRDatasetTests.cpp         # Síntesis Woodworth, ITD e ILD
│   ├── BinauralRendererTests.cpp    # Downmix 7.1 a estéreo y bypass de LFE
│   └── SpatialProcessorTests.cpp    # Modulación de anchura de escenario
│
├── Profiles/                   # Pruebas de persistencia y seguridad
│   ├── AudioProfileTests.cpp        # Valores nominales e integridad
│   ├── ProfileValidatorTests.cpp    # Detección de malware y anti-Path Traversal
│   ├── ProfileSerializerTests.cpp   # Prueba round-trip JSON
│   ├── ProfileMigratorTests.cpp     # Migración de esquemas v0 a v1
│   └── ProfileRepositoryTests.cpp   # Resolución segura de rutas en disco
│
└── App/                        # Pruebas de integración de sistema
    └── ApoConfigManagerTests.cpp    # Escritura atómica NTFS en config.txt
```

---

## 2. Ejecución Automatizada en Entornos Habilitados

Para compilar y ejecutar todas las pruebas en una máquina con compilador C++:

```powershell
# 1. Configurar CMake con pruebas activadas
cmake -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON

# 2. Compilar
cmake --build build --config Release --parallel

# 3. Ejecutar suite mediante CTest
ctest --test-dir build --output-on-failure -C Release
```

---

## 3. Estado de Ejecución
Conforme a la directiva de seguridad del proyecto en entornos de compilador deshabilitado, todas las pruebas se encuentran en estado **Awaiting Manual Validation** hasta su paso por el flujo automatizado de GitHub Actions.
