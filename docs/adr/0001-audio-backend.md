# ADR 0001: Selección del Backend de Procesamiento de Audio en Windows

## Estado
**Aceptado / En Transición Arquitectónica**

## Contexto y Planteamiento del Problema
**Dash Competitivo Audio** busca procesar el audio del sistema operativo (especialmente streams multicanal 5.1 y 7.1 de videojuegos competitivos) con la menor latencia posible (< 10 ms), máxima estabilidad y sin requerir controladores de kernel inestables ni depender de hacks frágiles.

Históricamente, el prototipo inicial recurrió a inyectar directivas de texto en el archivo `config.txt` de **Equalizer APO**, copiando binarios `.vst3` renombrados como `.dll` en la carpeta `C:\Program Files\EqualizerAPO\VSTPlugins`. Esta práctica presenta severas deficiencias arquitectónicas:
1. **Incompatibilidad de Formato VST3 vs VST2:** La directiva `VSTPlugin:` de Equalizer APO fue diseñada históricamente para plugins VST2 de 64 bits empaquetados como una DLL PE32+ pura con punto de entrada `VSTPluginMain`. Renombrar la DLL interna de un paquete VST3 (`.vst3/Contents/x86_64-win/*.vst3`) a `.dll` viola la especificación del estándar Steinberg VST3, provocando fallos de carga o inestabilidad si el host no implementa un loader VST3 nativo.
2. **Requisitos de Administrador e Invasión del Sistema:** Modificar `config.txt` de Equalizer APO exige permisos UAC de Administrador para escribir en `C:\Program Files\`, exponiendo al usuario al riesgo de sobreescritura accidental de configuraciones globales de audio ajenas a Dash.
3. **Dependencia Crítica Externa:** Si Equalizer APO no está instalado, está deshabilitado en el registro o falla tras una actualización de Windows, la aplicación no procesa ninguna señal.

---

## Análisis Comparativo de las 3 Alternativas

### Opción A: Integración Válida con Equalizer APO (Pipeline de Terceros)
- **Mecanismo:** La aplicación escribe configuraciones delimitadas por bloques con identificadores propios (`# BEGIN DASHCOMPETITIVOAUDIO ... # END DASHCOMPETITIVOAUDIO`) o un archivo auxiliar `Include: DashCompetitivoAudio\dash_config.txt`, usando plugins compatibles o scripts de filtros estándar (GraphicEQ, Gain, Delay).
- **Ventajas:** Procesamiento a nivel de sistema antes de la salida física; no requiere gestionar el stream de audio WASAPI en la aplicación principal.
- **Desventajas:**
  - Requiere privilegios de Administrador para cualquier cambio global.
  - El usuario debe reiniciar Windows tras instalar Equalizer APO para registrar su LFX/GFX sAPO en el driver de audio.
  - La directiva `VSTPlugin:` en Equalizer APO es sensible a versiones de librerías y no admite VST3 nativo sin adaptadores.

### Opción B: Procesamiento Nativo mediante Windows Audio Processing Object (Custom sAPO / EFX)
- **Mecanismo:** Desarrollar un objeto COM nativo de Windows (`IAudioProcessingObject`, `IAudioProcessingObjectRT`) registrado como Endpoint Effect (EFX) o Global Effect (GFX) en el registro de Windows (`HKLM\System\CurrentControlSet\Control\Class\{4d36e96c-...}`).
- **Ventajas:** Es la arquitectura oficial y canónica de Microsoft para procesar audio en tiempo real a nivel de sistema con mínima sobrecarga.
- **Desventajas:**
  - **Distribución y Firma de Drivers:** Los sAPO modernos en Windows 10 y 11 exigen empaquetado en paquetes de controladores (`.inf`), firma digital con certificado de validación extendida (EV) y certificación WHQL de Microsoft. En modo normal de Windows, un sAPO no firmado es rechazado por el subsistema `AudioSrv`.
  - **Mantenimiento y Riesgo de Pantalla Azul / Bloqueo:** Un fallo o excepción en un sAPO en tiempo real detiene el subsistema de audio de Windows (`audiodg.exe`), dejando todo el sistema operativo sin sonido.
  - Inviable para un proyecto ágil, académico o de código abierto sin presupuesto de firma de drivers.

### Opción C: Audio Routing Propio mediante Ruta WASAPI Explícita (Recomendada)
- **Mecanismo:**
  1. Windows y el juego renderizan el audio en un dispositivo de cable virtual 7.1 (ej. *Hi-Fi Cable Input* de VB-Audio o loopback).
  2. La aplicación principal abre un stream WASAPI de captura en modo evento de baja latencia (*Event-Driven Shared* o *Exclusive Mode* a 48 kHz / 512 muestras).
  3. La aplicación entrega los buffers directamente a **`DashAudioCore::AudioGraph`** en C++ puro en tiempo real.
  4. La aplicación renderiza el flujo resultante procesado directamente en los auriculares físicos del usuario mediante un stream WASAPI de salida de baja latencia.
- **Ventajas:**
  - **Cero privilegios de Administrador:** No toca `C:\Program Files`, ni el registro de Windows de drivers, ni archivos ajenos.
  - **Independencia absoluta de VST:** No depende de loaders de plugins de terceros ni hacks de extensión `.dll`. El motor `DashAudioCore` corre en el mismo proceso.
  - **Control total de latencia:** Búfer determinista de 5.3 ms (256 muestras @ 48 kHz) o 10.6 ms (512 muestras @ 48 kHz).
  - **Seguridad y Resiliencia:** Si la aplicación se cierra, no afecta la integridad de los drivers del sistema.

---

## Matriz de Decisión

| Criterio | Opción A (Equalizer APO) | Opción B (Windows sAPO Nativo) | Opción C (WASAPI Router Propio) |
| :--- | :---: | :---: | :---: |
| **Latencia adicional** | 1 - 3 ms (sobre WASAPI) | 0 - 1 ms | 5 - 10 ms (buffer de routing) |
| **Estabilidad** | Media (depende de `audiodg.exe`) | Crítica (riesgo en `audiodg`) | **Excelente** (proceso aislado) |
| **Compatibilidad VST3** | Deficiente / Frágil | N/A (C++ nativo) | **Total** (ejecuta C++ directo) |
| **Privilegios Admin (UAC)** | Obligatorios | Obligatorios | **No requeridos** |
| **Instalación / Despliegue** | Compleja (Reinicio de PC) | Compleja (Firma WHQL/EV) | **Simple (Portable)** |
| **Facilidad de Mantenimiento** | Baja (dependencia externa) | Muy baja | **Muy alta** |

---

## Decisión y Plan de Transición

1. **Fase Actual (Transición Segura):**
   - Para compatibilidad con usuarios que ya utilicen Equalizer APO, se implementa **`ApoConfigManager`**, garantizando que **nunca se sobrescriba a ciegas el archivo `config.txt`**, utilizando marcadores delimitados (`# BEGIN DASHCOMPETITIVOAUDIO ... # END`) y backups previos.
   - Se elimina por completo el empaquetado engañoso de `.vst3` renombrados a `.dll` en el instalador.
2. **Evolución Estratégica (Hacia Opción C):**
   - Se establece la interfaz abstracta **`IAudioBackend`** en `App/Source/Audio/`.
   - La aplicación principal evolucionará hacia un motor de routing nativo WASAPI autónomo alimentado por **`DashAudioCore::AudioGraph`**, proporcionando una experiencia verdaderamente profesional, segura y portable sin depender de Equalizer APO.
