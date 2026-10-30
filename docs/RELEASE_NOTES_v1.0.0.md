# Notas de Lanzamiento — DashCompetitivoAudio v1.0.0

**Fecha**: 2026-10-30  
**Versión**: `1.0.0` (SemVer)  
**Canal**: Release Candidate / Producción  

---

## Novedades Principales

### 1. Cadena DSP de Ultra Baja Latencia (DashAudioCore)
- **AudioGraph Unificado**: Orquestación en sitio de hasta 16 canales nativos sin asignaciones de memoria dinámicas en el hilo de audio.
- **Compresor Híbrido ITU-R BS.1770-4**: Medición ponderada K, compresión ascendente (upward) para pisadas lejanas y compresión descendente (downward) para disparos cercanos.
- **Supresor de Transitorios (Gun Crack)**: Crossover acústico Linkwitz-Riley configurable (100 a 800 Hz) con atenuación de disparos de hasta 18 dB.
- **Protector Ambiental (AmbienceGuard)**: Atenuación selectiva de ruido de fondo en la banda de 350 Hz a 2.5 kHz.
- **Ecualización Doble**: Curva de 31 bandas ISO y ecualizador paramétrico de 10 bandas biquad con tipos Peaking, Shelves, High/Low Pass y Notch.
- **Limitador de Pico a -0.5 dBFS**: Prevención de clipping entre etapas con aislamiento de fase para el canal LFE.

### 2. Audio Espacial 3D e Inmersión Binaural (DashAudioSpatial)
- **Soporte de hasta 16 Canales**: Layouts Stereo, 5.1, 7.1, 7.1.4 y 9.1.6.
- **HRTF Woodworth Analítico**: Síntesis de impulsos en tiempo real con cero latencia algorítmica y modelado de sombra de cabeza (ILD e ITD).
- **Adaptador HeSuVi**: Compatibilidad pasiva con archivos WAV de 14 canales locales.
- **Bypass de HRTF para LFE**: El canal de subgraves elude la convolución binaural para evitar cancelaciones acústicas por diferencia de fase en auriculares.

### 3. Sistema de Perfiles y DashAudioDB
- **Gestión Segura**: Deserialización con JSON nativo de JUCE, validación estricta de rangos y protección anti-Path Traversal.
- **Calibración Honesta**: Razer BlackShark V2 X catalogado honestamente como no calibrado (evitando mediciones inventadas); HyperX Cloud II y Logitech G Pro X con filtros AutoEq certificados hacia curva Harman Over-Ear 2018.
- **DashAudioDB**: Biblioteca local comunitaria estructurada con licencias CC-BY-4.0 y MIT.

### 4. Interfaz de Usuario Táctica Profesional
- **DashLookAndFeel**: Tema oscuro táctico (#121214 / #1A1A1E) con acentos cian y perillas vectorizadas.
- **Easy Mode & Advanced Mode**: Modo rápido para jugadores y control exhaustivo para ingenieros acústicos.
- **Cadena Visual**: Monitoreo de 8 etapas con estado activo/bypass.
- **Telemetría Desacoplada a 30 FPS**: Cero impacto en el callback de audio.

---

## Verificación de Integridad de Binarios (Checksums SHA-256)

Una vez compilados los artefactos en el entorno CI/CD, las firmas oficiales correspondientes son:

- `DashCompetitivoAudio-setup.exe`: (Generado automáticamente por CI/CD)
- `Dash Competitivo Audio.exe`: (Generado automáticamente por CI/CD)
- `DashAudioBench.exe`: (Generado automáticamente por CI/CD)
