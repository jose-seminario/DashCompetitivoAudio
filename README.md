# DashCompetitivoAudio

Suite de escritorio en **C++20 nativo** y motores DSP **VST3/Standalone (JUCE 8)** diseñada para la orquestación y el procesamiento táctico de audio competitivo de ultra baja latencia en Windows x64, sin la sobrecarga de entornos basados en Electron/Chromium.

---

## Flujo de Señal y Arquitectura de Audio

```text
[ Audio del Juego (5.1 / 7.1) ]
               │
               ▼
[ Cable Virtual (VB-Audio / Hi-Fi Cable Input) ]  ◄── Conmutación automática COM (IPolicyConfig)
               │
               ▼
[ Equalizer APO (Motor WASAPI del Sistema) ]      ◄── Inyección atómica NTFS (config.txt)
               │
               ├──► 1. Preamp (-6.0 dB headroom)
               ├──► 2. TransientSuppressor.dll    (Supresor de disparos / Gun Crack >= 280 Hz)
               ├──► 3. LoudnessCompressor.dll     (Compresor híbrido Upward/Downward BS.1770-4)
               ├──► 4. AmbienceGuard.dll          (Protector ambiental multibanda 350 Hz - 2.5 kHz)
               ├──► 5. GraphicEQ (31 bandas)      (Ajuste dinámico de Pasos y Graves)
               └──► 6. HeSuVi (Include HRIR)      (Virtualización binaural 7.1 ➔ Estéreo)
               │
               ▼
[ Auriculares Físicos del Usuario ]
```

---

## Estructura del Proyecto

```text
DashCompetitivoAudio/
├── CMakeLists.txt                                      # Build raíz (C++20, LTCG/LTO, FetchContent JUCE 8.0.4)
├── LICENSE                                             # Texto legal íntegro de la licencia GNU AGPLv3
├── THIRD_PARTY_NOTICES.md                              # Avisos de licencias y dependencias externas
├── App/                                                # Aplicación de control nativa (GUI + COM + Equalizer APO)
│   ├── CMakeLists.txt
│   └── Source/
│       ├── Main.cpp
│       ├── MainComponent.h / .cpp                      # Panel táctico (perfiles Campaign/Eclipse + 7 perillas)
│       └── Core/
│           ├── AudioPolicyManager.h / .cpp             # Orquestador COM IPolicyConfig (eConsole + eMultimedia)
│           ├── ApoConfigInjector.h / .cpp              # Escritor atómico NTFS para Equalizer APO config.txt
│           └── SystemDependencyChecker.h / .cpp        # Verificador de APO, VB-Cable, HeSuVi y permisos UAC
├── Plugins/
│   ├── TransientSuppressor/                            # 1. Supresor de Transitorios de Disparos (Gun Crack)
│   │   ├── CMakeLists.txt
│   │   └── Source/
│   │       ├── PluginProcessor.h / .cpp
│   │       ├── PluginEditor.h / .cpp
│   │       └── DSP/CenterTransientSuppressor.h
│   ├── LoudnessCompressor/                             # 2. Compresor Dinámico Híbrido ITU-R BS.1770-4
│   │   ├── CMakeLists.txt
│   │   └── Source/
│   │       ├── PluginProcessor.h / .cpp
│   │       ├── PluginEditor.h / .cpp
│   │       └── DSP/
│   │           ├── KWeightingFilter.h
│   │           └── HybridLoudnessCompressor.h
│   └── AmbienceGuard/                                  # 3. Protector Multibanda de Frecuencias Ambientales
│       ├── CMakeLists.txt
│       └── Source/
│           ├── PluginProcessor.h / .cpp
│           ├── PluginEditor.h / .cpp
│           └── DSP/MultibandAmbienceGuard.h
└── Installer/
    └── installer.nsi                                   # Script NSIS para generar DashCompetitivoAudio-setup.exe
```

---

## Motores DSP Incluidos (`Plugins/`)

Los tres plugins están implementados en C++20 libre de asignaciones dinámicas en el hilo de audio (*lock-free / allocation-free* en `processBlock`) y soportan configuraciones **1.0, 2.0, 5.1 y 7.1**.

### 1. `TransientSuppressor` (Supresor de Disparos)
Aísla el canal central en sistemas 5.1/7.1 (o la componente *Mid* en estéreo) y separa la señal mediante un crossover Linkwitz-Riley de 4.º orden en **280 Hz**. Mantiene intacto el cuerpo grave del arma (*Gun Body*, `< 280 Hz`) y atenúa únicamente el chasquido agudo impulsivo (*Gun Crack*, `>= 280 Hz`).

| Parámetro | Rango / Tipo | Valor Base | Función |
| :--- | :--- | :--- | :--- |
| `Gun` | `0.0 .. 1.0` | `0.45` (~`-7 dB`) | Techo de ganancia aplicado al transitorio agudo del disparo |
| `Steps` | `0.0 .. 1.0` | `0.0` | Margen adicional de factor de cresta para proteger micro-transitorios de pasos |
| `Front` | `Bool (0/1)` | `1` (Activo) | Limita la supresión al canal central (5.1/7.1) o *Mid* (2.0), preservando surrounds |
| `Restore` | `0.0 .. 1.0` | `0.0` | Compensación suave post-transitorio |
| `Release` | `0.0 .. 1.0` | `0.35` (~`76 ms`) | Tiempo de recuperación del seguidor de envolvente rápido |
| `Bypass` | `Bool (0/1)` | `0` | Desactiva el procesamiento del plugin |

### 2. `LoudnessCompressor` (Compresor Dinámico Híbrido ITU-R BS.1770-4)
Combina medición perceptual con ponderación K oficial (**Pre-Filter + RLB** y exclusión de canal LFE) con una etapa dual **Downward + Upward** y un limitador de techo de seguridad a `-0.5 dBFS`. Comprime explosiones que exceden la meta de sonoridad y eleva sonidos débiles (pasos, recargas) sin amplificar el ruido de fondo en silencio.

| Parámetro | Rango / Tipo | Valor Base | Función |
| :--- | :--- | :--- | :--- |
| `Amount` | `0.0 .. 1.0` | `0.75` | Intensidad global de compresión híbrida (ascendente y descendente) |
| `Target` | `-28 .. +8 LUFS` | `-18.0 LUFS` | Nivel objetivo de sonoridad ITU-R BS.1770-4 (`0.277778` normalizado) |
| `Release` | `0.0 .. 1.0` | `0.50` | Velocidad de recuperación de la computadora de ganancia |
| `DetRel` | `0.0 .. 1.0` | `0.583333` | Constante de relajación del detector RMS (~`400 ms` Momentary) |
| `LimRel` | `0.0 .. 1.0` | `0.612903` | Tiempo de relajación del limitador final de picos |
| `SCLow` / `SCHigh` | `0.0 .. 1.0` | `0.0` / `1.0` | Filtros TPT pasa-altos y pasa-bajos del detector *sidechain* (`20 Hz – 20 kHz`) |
| `Lift` | `0.0 .. 18.0 dB` | `9.0 dB` | Ganancia ascendente máxima (*Upward Compression*) para micro-detalles |
| `Gate` | `-80 .. -35 LUFS` | `-62.0 LUFS` | Umbral inferior con *soft-knee* para evitar elevar el piso de ruido |
| `Bypass` | `Bool (0/1)` | `0` | Desactiva el procesamiento del plugin |

### 3. `AmbienceGuard` (Protector Ambiental Multibanda)
Divide el espectro en 3 bandas coherentes en fase con filtros Linkwitz-Riley de 4.º orden (`< 350 Hz`, `350 Hz – 2.5 kHz`, `> 2.5 kHz`) y evalúa el índice de estacionariedad del sonido para atenuar ruidos constantes (viento, lluvia, motores) sin degradar pistas dinámicas lejanas.

| Parámetro | Rango / Tipo | Valor Base | Función |
| :--- | :--- | :--- | :--- |
| `AmbGuar` | `0.0 .. 1.0` | `1.0` | Profundidad de reducción sobre ruido ambiental estacionario |
| `Distant` | `Bool (0/1)` | `1` (Activo) | Preserva el margen dinámico de eventos no estacionarios (pasos/voces) en medios |
| `Target` | `0.0 .. 1.0` | `0.388889` | Umbral normalizado de compresión descendente suave (`-24 dBFS` RMS) |
| `SCLow` / `SCHigh` | `0.0 .. 1.0` | `0.2269` / `0.5293` | Cortes del *sidechain* IIR y crossovers (`350 Hz` y `2.5 kHz` por defecto) |
| `Release` / `DetRel` | `0.0 .. 1.0` | `0.0` / `0.458333` | Relajación de ganancia y constante del detector RMS (~`295 ms`) |
| `BandMod` | `Bool (0/1)` | `1` (Activo) | Conmuta entre procesamiento multibanda de 3 bandas (`1`) o banda completa (`0`) |
| `Bypass` | `Bool (0/1)` | `0` | Desactiva el procesamiento del plugin |

---

## Nota Técnica: Compatibilidad Universal x64 y SIMD (AVX2)

Se ha eliminado la bandera global de compilación `/arch:AVX2` en el `CMakeLists.txt` raíz para garantizar **compatibilidad universal con cualquier procesador x64 estándar** (evitando fallos por instrucción ilegal `0xC000001D` en CPUs anteriores a Haswell/Excavator o entornos virtualizados restringidos).

Como línea de evolución de rendimiento, en futuras versiones se implementará **Runtime Dispatch** (detección dinámica de capacidades de CPU en tiempo de ejecución vía `CPUID`), permitiendo despachar kernels vectorizados SSE4.1/AVX2 en procesadores modernos manteniendo una ruta base x64 100% compatible.

---

## Requisitos y Comandos de Compilación en Windows

1. **Visual Studio 2022** con la carga de trabajo **Desarrollo para el escritorio con C++** (MSVC v143 + Windows 10/11 SDK).
2. **CMake 3.22+** y **Git** en el `PATH`.
3. *(Opcional)* **NSIS 3.x** para construir el instalador `DashCompetitivoAudio-setup.exe`.

Abre una terminal PowerShell o *x64 Native Tools Command Prompt for VS 2022*:

```powershell
# 1. Generar solución Visual Studio 2022 x64 (descarga JUCE 8.0.4 automáticamente la primera vez)
cmake -B build -G "Visual Studio 17 2022" -A x64

# 2. Compilar en modo Release optimizado (/O2 + LTO estándar x64)
cmake --build build --config Release --parallel
```

*(Opcional)* Si dispones de una copia local de JUCE en disco para evitar la descarga inicial:

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_SOURCE_DIR="C:/Ruta/A/JUCE"
```

### Artefactos Generados

- **Aplicación Principal:**
  `build\App\DashCompetitivoAudio_artefacts\Release\Dash Competitivo Audio.exe`
- **Plugins VST3:**
  - `build\Plugins\TransientSuppressor\TransientSuppressor_artefacts\Release\VST3\TransientSuppressor.vst3`
  - `build\Plugins\LoudnessCompressor\LoudnessCompressor_artefacts\Release\VST3\LoudnessCompressor.vst3`
  - `build\Plugins\AmbienceGuard\AmbienceGuard_artefacts\Release\VST3\AmbienceGuard.vst3`
- **Instalador (si NSIS está instalado / GitHub Actions):**
  `build\Installer\DashCompetitivoAudio-setup.exe`

---

## Licencia

Este proyecto es software libre de código abierto distribuido bajo los términos de la licencia **GNU Affero General Public License v3.0 (AGPLv3)**. Consulta el texto legal completo en [`LICENSE`](./LICENSE) y el detalle de componentes de terceros en [`THIRD_PARTY_NOTICES.md`](./THIRD_PARTY_NOTICES.md).

### ¿Por qué AGPLv3?
El proyecto utiliza **JUCE 8** (descargado vía CMake `FetchContent`) tanto para la interfaz gráfica nativa de escritorio como para la infraestructura DSP y el empaquetado de plugins VST3. Dado que JUCE 8 se ofrece bajo un esquema de licencia dual (AGPLv3 o licencia comercial de pago), adoptamos **AGPLv3** de forma deliberada y transparente porque:
1. Garantiza el cumplimiento legal estricto con JUCE 8 y el SDK de VST3 (GPLv3) sin incurrir en costes de licenciamiento propietario para un proyecto académico y abierto.
2. Evita el sobrecoste técnico y el riesgo de regresiones de reescribir desde cero la capa de UI de escritorio y el *hosting*/envoltorio de los tres plugins sobre bibliotecas permisivas separadas, permitiéndonos concentrar la ingeniería en la calidad del procesamiento DSP en tiempo real.
