# Arquitectura del Sistema - Dash Competitivo Audio

Este documento describe la arquitectura técnica, el modelo de datos, la jerarquía de capas y el diseño de concurrencia de **Dash Competitivo Audio**.

---

## 1. Visión General del Sistema

Dash Competitivo Audio es una plataforma de procesamiento de audio táctico y baja latencia para Windows x64. El sistema opera mediante una arquitectura híbrida de dos niveles:

1. **Nivel de Orquestación del Sistema Operativo (`App/`):** Una aplicación de escritorio nativa en C++20/JUCE que interactúa directamente con el subsistema de audio de Windows (WASAPI / Core Audio COM) para conmutar dispositivos predeterminados e inyectar pipelines de procesamiento en Equalizer APO de manera atómica.
2. **Nivel de Procesamiento Digital de Señales Canónico (`DashAudioCore/` y `Plugins/`):** Una librería estática pura de cálculo DSP en tiempo real (`DashAudioCore`) que alimenta los plugins VST3 alojados por Equalizer APO o ejecutables en modo Standalone.

---

## 2. Flujo de Señal de Extremo a Extremo

```text
[ Videojuego / Aplicación (Render 5.1 / 7.1) ]
                      │
                      ▼
[ Endpoint WASAPI: Hi-Fi Cable Input ]  ◄── IPolicyConfig (eConsole / eMultimedia)
                      │
                      ▼
[ Pipeline del Sistema: Equalizer APO ] ◄── Escritura atómica NTFS en config.txt
                      │
       ┌──────────────┴──────────────────────────┐
       │                                         │
       ▼                                         ▼
1. Preamp (-6.0 dB)                     [ DashAudioCore Engine ]
       │                                         │
       ▼                                         ├─► TransientProcessor
2. TransientSuppressor.dll ──────────────────────┤   (Aísla Canal Central, LR4 @ 280Hz)
       │                                         │
       ▼                                         ├─► DynamicsProcessor + LimiterProcessor
3. LoudnessCompressor.dll  ──────────────────────┤   (BS.1770-4 K-Weighting, Up/Down, -0.5dBFS)
       │                                         │
       ▼                                         └─► AmbienceProcessor
4. AmbienceGuard.dll       ──────────────────────┘   (IIR Sidechain 350-2500Hz, 3 bandas)
       │
       ▼
5. GraphicEQ (31 bandas ISO) ◄── Calculado unificadamente por EQProcessor::computeBandGainsDb()
       │
       ▼
6. HeSuVi (Include HeSuVi\hesuvi.txt) ◄── Convolución HRIR binaural (7.1 Surround ➔ Estéreo)
       │
       ▼
[ Auriculares Físicos del Usuario ]
```

---

## 3. Descomposición por Capas y Responsabilidades

### Capa 1: Núcleo DSP Canónico (`DashAudioCore`)
Librería estática desacoplada de la interfaz gráfica y de APIs propietarias de plugins.
- **`DynamicsProcessor`:**
  - Implementa filtrado de ponderación perceptual K según **ITU-R BS.1770-4** (High-Shelf @ 1681.97 Hz + High-Pass @ 38.13 Hz).
  - Ponderación espacial: Exclusión estricta de canal LFE (ganancia 0.0) y ponderación de +1.5 dB (1.41) en canales laterales/traseros.
  - Compresión dinámica híbrida: *Downward* sobre explosiones (> -18 LUFS) y *Upward* con *soft-gate* sobre pasos y recargas (< -18 LUFS).
  - Encapsula `LimiterProcessor` para contención de picos de muestra exacta a -0.5 dBFS.
- **`TransientProcessor`:**
  - Separación de fase coherente con crossover Linkwitz-Riley de 4.º orden en 280 Hz:
    - *Gun Body* (< 280 Hz): Pasa intacto sin alteración tímbrica de graves.
    - *Gun Crack* (>= 280 Hz): Detección de envolvente diferencial de cresta (ataque 0.15 ms) y atenuación dinámica (~ -7 dB por defecto).
  - Focalización quirúrgica en el canal central (índice 2 en 5.1/7.1) o en el canal *Mid* en configuraciones estéreo.
- **`AmbienceProcessor`:**
  - Detector *sidechain* IIR pasa-banda configurable (por defecto 350 Hz a 2500 Hz).
  - Crossovers Linkwitz-Riley de 4.º orden para 3 bandas con alineación de fase All-Pass en graves.
  - Estimación de piso de ruido estacionario y reducción selectiva preservando la dinámica de eventos lejanos (*Distant Guard*).
- **`EQProcessor`:**
  - Motor IIR de 31 bandas ISO (20 Hz a 20 kHz, Q = 4.318).
  - Función estática pura `computeBandGainsDb(footstepsDb, bassDb)` que actúa como la **única fuente de verdad** para la modulación acústica de pasos (3.15 kHz y 6.3 kHz) y graves (80 Hz a 250 Hz).
- **`LimiterProcessor`:**
  - Limitador de muestra exacta con ataque instantáneo (0 muestras de latencia inducida) y relajación balística exponencial.

### Capa 2: Wrappers de Plugins VST3 (`Plugins/`)
- Adaptan la API de JUCE `AudioProcessor` y `AudioProcessorValueTreeState` (APVTS) a los procesadores de `DashAudioCore`.
- Cero lógica matemática duplicada: reciben el buffer de audio del host y delegan directamente en `process(buffer.getArrayOfWritePointers(), numChannels, numSamples)`.
- Exponen parámetros normalizados y telemetría atómica (LUFS, reducción de ganancia) para los editores gráficos.

### Capa 3: Orquestación del Sistema y Control (`App/`)
- **`AudioPolicyManager`:**
  - Instancia COM de `CLSID_PolicyConfigClient` e interfaz `IPolicyConfig`.
  - Invoca `SetDefaultEndpoint` para ambos roles de Windows: `eConsole` (juegos y comunicación interactiva) y `eMultimedia` (reproducción multimedia).
  - Captura y restaura el endpoint físico original al desactivar el perfil táctico.
- **`ApoConfigInjector`:**
  - Generador del archivo `config.txt` para Equalizer APO.
  - Escritura atómica con estrategia *Write-Through*: `CreateFileW` con archivo temporal único, vaciado de buffers con `FlushFileBuffers` y reemplazo atómico mediante `MoveFileExW(MOVEFILE_REPLACE_EXISTING)`.
  - Mecanismo de fallback directo con reporte de permisos denegados (UAC).
- **`SystemDependencyChecker`:**
  - Inspecciona la presencia de `EqualizerAPO.dll`, controladores virtuales VB-Audio/VoiceMeeter y HeSuVi.
  - Comprueba privilegios de token mediante `TokenElevation`.
  - Asiste en la descarga automática de dependencias oficiales vía `URLDownloadToFileW` y relanzamiento con elevación `runas`.

---

## 4. Modelo de Concurrencia y Tiempo Real

El sistema está diseñado bajo estrictas restricciones de procesamiento de audio en tiempo real (*Hard Real-Time constraints*):

1. **Hilo de Audio (Audio Render Thread):**
   - **Cero asignaciones dinámicas en el heap:** Todas las tablas, filtros, envolventes y buffers auxiliares se pre-asignan en `prepareToPlay()` / `prepare()`.
   - **Cero bloqueos o primitivas de sincronización bloqueantes:** No se utilizan `std::mutex`, `CRITICAL_SECTION` ni operaciones de espera.
   - **Acceso atómico relajado:** La comunicación entre el hilo de audio y la UI se realiza exclusivamente mediante `std::atomic<float>` con ordenamiento de memoria `std::memory_order_relaxed`.
2. **Hilo de Mensajes / UI (Message Thread):**
   - Los componentes de la interfaz de usuario se actualizan mediante temporizadores a 30 Hz (`juce::Timer`), consultando valores atómicos del procesador.
   - Las modificaciones de parámetros en la GUI aplican un temporizador de *debounce* de 120 ms antes de escribir en disco, evitando saturar el sistema de archivos con escrituras recurrentes de `config.txt`.

---

## 5. Matriz de Soporte Multicanal

| Procesador | Mono (1.0) | Estéreo (2.0) | 5.1 Surround | 7.1 Surround | Canal LFE |
| :--- | :---: | :---: | :---: | :---: | :---: |
| `TransientProcessor` | Procesamiento directo | División Mid/Side (trata Mid si Front=1) | Aísla Canal Central (ch 2) | Aísla Canal Central (ch 2) | Pasa intacto |
| `DynamicsProcessor` | Ponderación K 1.0 | Ponderación K (L/R = 1.0) | BS.1770-4 (L,R,C=1.0, Surr=1.41) | BS.1770-4 (L,R,C=1.0, Surr=1.41) | Excluido (peso 0.0) |
| `AmbienceProcessor` | Pasa-banda + 3 bandas | Procesamiento estéreo vinculado | Multibanda 6 canales vinculados | Multibanda 8 canales vinculados | Pasa intacto |
| `EQProcessor` | 31 bandas mono | 31 bandas estéreo | 31 bandas en los 6 canales | 31 bandas en los 8 canales | Filtrado según curva |
| `LimiterProcessor` | Techo -0.5 dBFS | Enlace de pico estéreo | Enlace de pico en 6 canales | Enlace de pico en 8 canales | Protegido |

---

## 6. Grafo de Dependencias de Compilación en CMake

```text
               [ JUCE 8.0.4 (FetchContent) ]
                             │
       ┌─────────────────────┼─────────────────────┐
       ▼                     ▼                     ▼
[ juce::juce_core ]   [ juce::juce_dsp ]   [ juce::juce_gui_basics ]
       │                     │                     │
       │ (Headers Privados)  │ (INTERFACE)         │
       └──────────┬──────────┘                     │
                  ▼                                │
         [ DashAudioCore ]                         │
                  │                                │
       ┌──────────┼──────────────────┐             │
       ▼          ▼                  ▼             │
[ LoudnessComp ] [ TransientSupp ] [ AmbienceGuard ]│
       ▲                  ▲                  ▲     │
       └──────────────────┼──────────────────┘     │
                          ▼                        ▼
                [ Target: DashCompetitivoAudio (App) ]
```
