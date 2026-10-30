# Guía de Contribución para DashAudioDB

Gracias por contribuir a la biblioteca acústica de **DashAudioDB**. Para mantener el más alto estándar de seguridad, honestidad y rendimiento competitivo, cada propuesta de perfil debe acatar las siguientes reglas:

---

## 1. Reglas de Seguridad (Innegociables)
1. **Perfiles como Datos Puros**: Los archivos deben ser exclusivamente JSON válidos que cumplan con `Schema/community-profile.schema.json`.
2. **Cero Binarios y Scripts**: Queda terminantemente prohibido incluir ejecutables (`.exe`, `.dll`), scripts (`.bat`, `.cmd`, `.ps1`, `.vbs`, PowerShell) o llamadas al sistema. El motor C++ `ProfileValidator` audita cada archivo y rechaza instantáneamente cualquier token sospechoso.
3. **Límite de Tamaño**: Tamaño máximo por archivo: **1 MB**.
4. **Nomenclatura**: El `profileId` debe ser un slug alfanumérico en minúsculas con guiones o guiones bajos (ej: `warzone-audio-clarity`).

---

## 2. Honestidad en Mediciones de Auriculares
1. **Nunca inventar datos**: Si un auricular (ej: *Razer BlackShark V2 X*) no dispone de una medición pública de laboratorio en simuladores calibrados (Gras 43AG o BK 5128), el perfil debe marcarse con:
   ```json
   "calibrationStatus": "uncalibrated_pending_verified_measurement"
   ```
   y la sección de bandas paramétricas debe permanecer vacía hasta que se obtenga una medición contrastable.
2. **Atribución Clara**: Indicar siempre la procedencia de la curva (ej: "AutoEq por Jaakkopasanen / Archivo de oratory1990").
3. **Clasificación Explícita**:
   - `verified`: Curva respaldada por mediciones de laboratorio acreditadas.
   - `community`: Curva diseñada o ajustada por oído por un miembro de la comunidad.
   - `experimental`: Curva en fase de pruebas.

---

## 3. Licenciamiento Requerido
- Para perfiles propios, se requiere licenciar bajo **CC-BY-4.0**.
- Para adaptaciones de AutoEq, se preserva la licencia **MIT** correspondiente.
