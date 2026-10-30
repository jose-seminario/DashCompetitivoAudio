# Auditoría de Licenciamiento y Compatibilidad Legal

Este documento contiene el análisis jurídico-técnico sobre las licencias aplicables al proyecto **Dash Competitivo Audio**, sus efectos en la distribución y el análisis comparativo de alternativas.

---

## 1. Situación Actual del Proyecto

- **Licencia declarada:** **GNU Affero General Public License v3.0 (AGPLv3)** (véase el archivo [`LICENSE`](../LICENSE)).
- **Dependencia condicionante principal:** **JUCE 8.0.4** (adquirido mediante descarga automática vía CMake `FetchContent`).
  - Raw Material Software Limited ofrece JUCE bajo un modelo dual: licencia comercial de pago o licencia libre **AGPLv3**.
  - Al no contar con una licencia comercial de pago, el uso legítimo de JUCE 8 impone la adopción del esquema copyleft fuerte de la **AGPLv3**.

---

## 2. Análisis de Compatibilidad de la Cadena de Dependencias

### 2.1. Steinberg VST3 SDK
- El SDK de VST3 incrustado en JUCE se ofrece bajo licencia dual: Licencia Propietaria de Steinberg o **GNU General Public License v3.0 (GPLv3)**.
- **Compatibilidad:** Conforme a la Sección 13 de la AGPLv3 y la Sección 13 de la GPLv3, las obras licenciadas bajo GPLv3 pueden combinarse y enlazarse válidamente con software bajo AGPLv3.

### 2.2. Equalizer APO (GPLv2)
- Equalizer APO se distribuye bajo GPLv2.
- **Relación con Dash Competitivo Audio:** No existe enlace estático ni dinámico con el código o las DLLs de Equalizer APO. La aplicación simplemente genera un archivo de texto de comandos (`config.txt`) y lo deposita en el sistema de archivos de Windows. Según la doctrina estándar de la Free Software Foundation, la comunicación mediante archivos de configuración en disco no genera una obra derivada que comprometa el licenciamiento entre ambos proyectos.

### 2.3. HeSuVi y Archivos HRIR
- HeSuVi es una herramienta bajo GPLv3. Sus archivos de respuesta al impulso HRIR pertenecen a diversos investigadores y autores.
- Dash Competitivo Audio no distribuye perfiles HRIR en su repositorio ni binarios empaquetados; simplemente emite la directiva `Include: HeSuVi\hesuvi.txt` si el usuario ha instalado dicha suite en su máquina.

---

## 3. Análisis de Alternativas de Licenciamiento Futuras

Si en el futuro se requiriese cambiar la licencia del proyecto hacia un esquema permisivo (MIT/Apache 2.0) o propietario comercial, deberán evaluarse las siguientes alternativas:

| Alternativa | Coste Económico | Esfuerzo de Ingeniería | Viabilidad / Riesgos |
| :--- | :--- | :--- | :--- |
| **Opción A: Adquisición de Licencia Comercial JUCE** | Suscripción mensual / perpetua por desarrollador (~$800 - $2,600 USD) | Nulo (el código actual se mantiene 100% idéntico). | Inviable para proyectos académicos o comunitarios de presupuesto cero. |
| **Opción B: Migración de la UI a Frameworks Permisivos (Slint / Dear ImGui / Qt LGPL)** | $0 USD | **Muy alto:** Reescritura total de `MainComponent` y de los editores gráficos de los 3 plugins VST3. | Alto riesgo de introducir regresiones en la interfaz y pérdida de las facilidades nativas de hosting de JUCE. |
| **Opción C: Permanencia en AGPLv3 (Recomendada)** | $0 USD | **Nulo:** Cumplimiento total e inmediato. | El código fuente completo debe permanecer público y accesible bajo AGPLv3 si se distribuye el binario. |

---

## 4. Conclusión

La adopción de **AGPLv3** es la vía más sólida, legalmente intachable y técnicamente viable para **Dash Competitivo Audio**, permitiendo un desarrollo ágil y abierto sin costes financieros ni riesgos de infracción de propiedad intelectual.
