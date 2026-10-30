# Hoja de Ruta Técnica (Technical Roadmap) - 6 Fases

Este documento define la planificación estratégica y técnica para evolucionar **Dash Competitivo Audio** desde su base canónica actual hacia una plataforma profesional de audio competitivo para Windows.

---

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                       PLAN DE EVOLUCIÓN EN 6 FASES                          │
└─────────────────────────────────────────────────────────────────────────────┘
  [ Fase 1 ] Cimientos Técnicos y Desacoplamiento (DashAudioCore)  <== ACTUAL
      │
      ▼
  [ Fase 2 ] Harness de Pruebas Unitarias Offline sin Hardware de Audio
      │
      ▼
  [ Fase 3 ] Vectorización SIMD y Despacho Dinámico (Runtime Dispatch CPUID)
      │
      ▼
  [ Fase 4 ] Resiliencia WASAPI y Notificaciones de Dispositivos en Vivo
      │
      ▼
  [ Fase 5 ] Serialización de Perfiles Tácticos por Videojuego (JSON/YAML)
      │
      ▼
  [ Fase 6 ] Automatización de CI/CD, Firma Digital y Empaquetado Comercial
```

---

## Fase 1: Cimientos Técnicos y Desacoplamiento (Estado: En Auditoría)
- **Objetivo:** Separar completamente la matemática DSP de los wrappers VST3 y de la interfaz gráfica.
- **Entregables:**
  - Librería estática `DashAudioCore` con 5 procesadores (`DynamicsProcessor`, `TransientProcessor`, `AmbienceProcessor`, `EQProcessor`, `LimiterProcessor`).
  - API pública 100% C++20 estándar sin tipos JUCE.
  - Eliminación de headers antiguos duplicados en los plugins.
  - Configuración limpia en CMake sin colisiones de símbolos (`LNK2005`).

---

## Fase 2: Harness de Pruebas Unitarias Offline (DSP Verification)
- **Objetivo:** Verificar matemáticamente el comportamiento de cada algoritmo sin depender de hardware físico ni tarjetas de sonido.
- **Tareas Clave:**
  - Crear un target ejecutable ligero de test (Catch2 / GoogleTest o harness C++20 propio sin dependencias pesadas).
  - Pruebas de respuesta al impulso de los crossovers Linkwitz-Riley a 280 Hz, 350 Hz y 2500 Hz (suma de potencia plana).
  - Verificación de la ponderación K (BS.1770-4) contra tonos puros calibrados (1 kHz @ 0 dBFS => -3.01 LUFS).
  - Validación de contención del limitador (ninguna muestra debe exceder el umbral de -0.5 dBFS).
  - Verificación de no-asignación dinámica (monitoreo de operadores `new`/`malloc` en el bucle de procesamiento).

---

## Fase 3: Vectorización SIMD y Despacho Dinámico (Runtime Dispatch)
- **Objetivo:** Reducir drásticamente el uso de CPU (< 0.5% en juegos competitivos) sin romper la compatibilidad con procesadores x64 estándar.
- **Tareas Clave:**
  - Implementar módulo `CpuFeatures` que consulte `__cpuid` en tiempo de inicialización (detección de SSE4.1, AVX, AVX2, FMA).
  - Crear kernels vectorizados para los biquads IIR de 31 bandas y las etapas de convolución/ganancia.
  - Punteros a funciones de despacho seleccionados en `prepare()`, garantizando cero saltos condicionales en el bucle de muestras.

---

## Fase 4: Resiliencia de Audio WASAPI y Notificaciones en Vivo
- **Objetivo:** Evitar cuelgues o pérdida de audio ante desconexiones de auriculares, cambios de frecuencia de muestreo o suspensión del sistema.
- **Tareas Clave:**
  - Implementar la interfaz COM `IMMNotificationClient` en `AudioPolicyManager`.
  - Captura en tiempo real de eventos `OnDefaultDeviceChanged` y `OnDeviceStateChanged`.
  - Reconexión y regeneración suave del flujo de audio sin requerir reiniciar la aplicación de control.

---

## Fase 5: Serialización de Perfiles Tácticos por Videojuego
- **Objetivo:** Permitir a los jugadores crear, exportar y compartir perfiles específicos por juego (Warzone, CS2, Valorant, Apex Legends).
- **Tareas Clave:**
  - Diseñar un esquema de serialización ligero y legible en formato JSON o YAML.
  - Soporte para detección automática del proceso en ejecución (ej. detectar `cod.exe` y activar automáticamente el perfil *Warzone Pro*).
  - Interfaz de ajuste fino para bandas acústicas específicas (pasos en madera, hormigón, sonido de defusado de bomba).

---

## Fase 6: Pipeline de Empaquetado, Firma y CI/CD Comercial
- **Objetivo:** Distribución robusta de instaladores de un solo clic listos para usuarios finales.
- **Tareas Clave:**
  - Integración en GitHub Actions de artefactos firmados digitalmente.
  - Instalador NSIS mejorado con comprobación previa automática de dependencias y configuración guiada de endpoints de audio.
  - Sistema de autodiagnóstico incorporado para verificar si Equalizer APO está interceptando correctamente el dispositivo virtual.
