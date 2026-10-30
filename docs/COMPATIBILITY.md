# Matriz de Compatibilidad — DashCompetitivoAudio

Este documento detalla la compatibilidad técnica del software respecto a versiones de Windows, frecuencias de muestreo, tamaños de búfer de audio y configuraciones multicanal.

---

## 1. Definición de Estados de Compatibilidad

- **Implementado**: El código fuente, algoritmos matemáticos y estructuras de datos están completamente escritos e integrados en la arquitectura.
- **Preparado**: Pruebas unitarias o de integración escritas y listas para su validación cuando se compile el ejecutable.
- **Probado**: Validado mediante ejecución de suites de pruebas o análisis en entorno de pruebas.
- **No probado**: Funcionalidad implementada pero pendiente de pruebas en hardware físico real específico.
- **No soportado**: Configuraciones explícitamente excluidas por limitaciones de la plataforma o estándares acústicos.

---

## 2. Compatibilidad de Sistema Operativo (Windows x64)

| Sistema Operativo | Arquitectura | Estado | Notas Técnicas |
| :--- | :--- | :--- | :--- |
| **Windows 11 (23H2 / 24H2)** | x64 | **Implementado / Preparado** | Soporte completo COM `IPolicyConfig`, WASAPI exclusivo y modo Equalizer APO. |
| **Windows 11 (21H2 / 22H2)** | x64 | **Implementado / Preparado** | Totalmente compatible. |
| **Windows 10 (21H2 / 22H2)** | x64 | **Implementado / Preparado** | Compatible. Requiere UAC para modificar `config.txt` de Equalizer APO. |
| **Windows 10 (< 20H1)** | x64 | **No probado** | Sin pruebas en versiones antiguas de Windows 10. |
| **Windows 7 / 8 / 8.1** | x64 | **No soportado** | Requiere C++20 y APIs modernas de gestión de audio de Windows 10/11. |
| **Windows ARM64** | ARM64 | **No soportado** | Optimizado exclusivamente para conjuntos de instrucciones x64 (AVX2/SSE2). |

---

## 3. Frecuencias de Muestreo (Sample Rates)

| Frecuencia | Estado | Comportamiento DSP |
| :--- | :--- | :--- |
| **44100 Hz (44.1 kHz)** | **Implementado / Preparado** | Coeficientes de filtros IIR biquad y Linkwitz-Riley calculados dinámicamente. |
| **48000 Hz (48.0 kHz)** | **Implementado / Preparado** | **Frecuencia nominal recomendada** para Call of Duty: Warzone y shooters modernos. |
| **88200 Hz (88.2 kHz)** | **Implementado / No probado** | Soporte algorítmico completo. |
| **96000 Hz (96.0 kHz)** | **Implementado / Preparado** | Totalmente soportado en `AudioGraph` y `SpatialProcessor`. |
| **192000 Hz (192.0 kHz)** | **Implementado / No probado** | Soportado; incrementa el uso de CPU en convolución directa FIR. |
| **< 44100 Hz** | **No soportado** | Rechazado por `ProfileValidator` para evitar aliasing severo. |

---

## 4. Tamaños de Búfer y Latencia Estimada (a 48 kHz)

| Tamaño de Búfer | Latencia de Búfer | Estado | Recomendación de Uso |
| :--- | :--- | :--- | :--- |
| **64 muestras** | 1.33 ms | **Implementado / No probado** | Requiere hardware de audio dedicado o DAC ASIO de gama alta. |
| **128 muestras** | 2.67 ms | **Implementado / Preparado** | Modo competitivo de ultra baja latencia. |
| **256 muestras** | **5.33 ms** | **Implementado / Preparado** | **Recomendación estándar**: Balance perfecto estabilidad/latencia. |
| **512 muestras** | 10.67 ms | **Implementado / Preparado** | Modo balanceado de alta estabilidad. |
| **1024 muestras** | 21.33 ms | **Implementado / Preparado** | Modo seguro para CPUs de bajo consumo. |

---

## 5. Canales de Audio y Layouts Espaciales

| Formato | Canales | Estado | Enrutamiento Acústico |
| :--- | :--- | :--- | :--- |
| **Mono (1.0)** | 1 | **Implementado / Preparado** | Duplicación limpia a ambos oídos sin convolución espacial. |
| **Estéreo (2.0)** | 2 | **Implementado / Preparado** | Traspaso estéreo con procesamiento dinámico e ISO 31. |
| **Quad (4.0)** | 4 | **Implementado / Preparado** | Mapeo espacial FL, FR, BL, BR. |
| **5.1 Surround** | 6 | **Implementado / Preparado** | FL, FR, C, LFE (bypass HRTF), SL, SR. |
| **7.1 Surround** | 8 | **Implementado / Preparado** | **Formato nativo para Warzone**: FL, FR, C, LFE, SL, SR, BL, BR. |
| **7.1.4 Inmersivo** | 12 | **Implementado / Preparado** | 7.1 base + 4 canales de techo (TFL, TFR, TBL, TBR) con HRTF 3D. |
| **9.1.6 Espacial** | 16 | **Implementado / Preparado** | Soporte máximo nativo de `kMaxSupportedChannels = 16`. |
| **> 16 canales** | > 16 | **No soportado** | Limitado por seguridad de memoria y rendimiento en tiempo real. |

---

## 6. Auriculares

| Modelo | Estado | Notas |
| :--- | :--- | :--- |
| **Razer BlackShark V2 X** | **Implementado / Pendiente Medición** | Perfil creado sin curva compensatoria falsa (pendiente medición Gras 43AG). |
| **HyperX Cloud II** | **Implementado / Preparado** | Curva AutoEq verificada hacia target Harman Over-Ear 2018 (licencia MIT). |
| **Logitech G Pro X** | **Implementado / Preparado** | Curva AutoEq verificada hacia target Harman Over-Ear 2018 (licencia MIT). |
