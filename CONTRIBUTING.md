# Guía de Contribución al Proyecto — DashCompetitivoAudio

¡Gracias por tu interés en contribuir a **DashCompetitivoAudio**! Esta plataforma persigue la excelencia en procesamiento de audio competitivo, honestidad técnica y máxima seguridad en Windows.

---

## 1. Principios Fundamentales
1. **Seguridad y Confianza**: Los perfiles y presets son datos puros en formato JSON. Queda terminantemente prohibido incorporar código ejecutable, scripts de automatización (PowerShell, BAT, VBS) o dependencias opacas.
2. **Honestidad en Mediciones Acústicas**: Si un auricular no dispone de una medición contrastable en simulador estandarizado (Gras 43AG / BK 5128), el perfil debe catalogarse como `uncalibrated_pending_verified_measurement`. No se aceptan curvas acústicas inventadas.
3. **Tiempo Real Estricto en Audio**: Todo código que opere dentro del método `process()` en `DashAudioCore` o `DashAudioSpatial` debe respetar cero asignaciones dinámicas (`no malloc/new`), cero locks (`no std::mutex`) y cero operaciones de entrada/salida (`no I/O`).

---

## 2. Flujo de Trabajo para Pull Requests
1. Realiza un Fork del repositorio y crea una rama descriptiva (`feature/nueva-funcionalidad` o `fix/correccion-dsp`).
2. Mantén los estándares de código C++20 (compilador MSVC v143 / Clang 16+).
3. Asegúrate de añadir las pruebas unitarias correspondientes en el directorio `Tests/`.
4. Todos los commits deben seguir la convención de [Conventional Commits](https://www.conventionalcommits.org/) (ej: `feat:`, `fix:`, `docs:`, `test:`).
5. Abre un Pull Request describiendo claramente la motivación acústica o técnica del cambio.

---

## 3. Licenciamiento de Contribuciones
Al contribuir código o perfiles a este repositorio, aceptas que tus contribuciones se licencien bajo los términos de la licencia general del proyecto (GPLv3 para el código fuente y CC-BY-4.0 para los perfiles y datos de DashAudioDB).
