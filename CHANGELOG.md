# Registro de Cambios (Changelog) — DashCompetitivoAudio

Todos los cambios notables en este proyecto se documentarán en este archivo.
El formato se basa en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/) y este proyecto se adhiere a [Semantic Versioning (SemVer)](https://semver.org/lang/es/).

---

## [1.0.0] - 2026-10-30

### Añadido
- **Fase 1**: Auditoría completa de arquitectura nativa C++20, eliminación de duplicidades y corrección de estándares de compilación MSVC con AVX2.
- **Fase 2**: Implementación de la cadena canónica `AudioGraph`, abstracción de backend `IAudioBackend`, e inyección atómica no destructiva `ApoConfigManager`.
- **Fase 3**: Refinamiento matemático de motores DSP (`DynamicsProcessor` BS.1770-4, `TransientProcessor` con crossover Linkwitz-Riley configurable, `AmbienceProcessor`, `EQProcessor` y nuevo `ParametricEQProcessor` de 10 bandas biquad).
- **Fase 4**: Creación del módulo espacial `DashAudioSpatial`, soporte de hasta 16 canales simultáneos, coordenadas esféricas 3D en `ChannelLayout`, convolución FIR analítica Woodworth y bypass acústico de HRTF para el canal LFE.
- **Fase 5**: Arquitectura de perfiles desacoplada (`AudioProfile`, `ProfileValidator`, `ProfileSerializer`, `ProfileMigrator`, `ProfileRepository`), esquemas JSON Draft 2020-12, calibraciones AutoEq para HyperX/Logitech y catálogo comunitario `DashAudioDB`.
- **Fase 6**: Interfaz táctica profesional en JUCE (`DashLookAndFeel`), conmutador `Easy Mode` / `Advanced Mode`, monitoreo de cadena visual de 8 nodos y 9 paneles modulares desacoplados.
- **Fase 7**: Herramienta de benchmark `Tools/DashAudioBench/` con análisis de CPU (p50/p95/p99), latencia, calidad acústica y comparación A/B con Loudness Matching; actualización del instalador NSIS y documentación completa Open Source.

### Seguridad
- Validador estricto `ProfileValidator` contra Path Traversal e inyección de ejecutables.
- Prohibición de escritura destructiva en `config.txt` de Equalizer APO.
- Desinstalador seguro que preserva perfiles y configuraciones personales.
