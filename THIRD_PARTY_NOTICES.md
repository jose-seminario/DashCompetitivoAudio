# Avisos de Software de Terceros (Third-Party Notices)

Este documento detalla las bibliotecas, herramientas de compilación y dependencias externas del entorno de ejecución relacionadas con **Dash Competitivo Audio**, así como el cumplimiento de sus respectivas licencias.

El código fuente propio de **Dash Competitivo Audio** se distribuye bajo la licencia **GNU Affero General Public License v3.0 (AGPLv3)** (véase el archivo [`LICENSE`](./LICENSE)).

---

## 1. Bibliotecas Enlazadas en Compilación

Estas dependencias se descargan automáticamente durante la configuración de CMake (`FetchContent`) y se enlazan en los binarios generados (`Dash Competitivo Audio.exe` y los plugins `.vst3`):

### JUCE Framework (v8.0.4)
- **Autor / Titular:** Raw Material Software Limited
- **Repositorio:** https://github.com/juce-framework/JUCE
- **Licencia aplicable:** **GNU Affero General Public License v3 (AGPLv3)** (licencia dual AGPLv3 / Comercial; este proyecto se acoge íntegramente a la modalidad de código abierto AGPLv3).
- **Uso en el proyecto:** Interfaz gráfica de escritorio (`juce_gui_basics`, `juce_gui_extra`), envoltorios de plugins de audio (`juce_audio_utils`) y bloques matemáticos de procesamiento digital de señales (`juce_dsp`).

### Steinberg VST3 SDK (integrado a través de JUCE)
- **Autor / Titular:** Steinberg Media Technologies GmbH
- **Sitio oficial:** https://www.steinberg.net/vst3sdk
- **Licencia aplicable:** **GNU General Public License v3 (GPLv3)** (licencia dual Proprietary Steinberg VST3 License / GPLv3; compatible con AGPLv3 según la Sección 13 de ambas licencias).
- **Uso en el proyecto:** Interfaces de comunicación host-plugin para compilar los motores DSP (`LoudnessCompressor`, `TransientSuppressor` y `AmbienceGuard`) en formato `.vst3`.
- *Aviso de marca:* *VST is a trademark of Steinberg Media Technologies GmbH, registered in Europe and other countries.*

---

## 2. Herramientas de Construcción y Empaquetado (No Enlazadas en el Código)

Estas herramientas se emplean únicamente durante el proceso de compilación e integración continua (CI/CD) y no imponen restricciones copyleft adicionales sobre el código fuente del proyecto:

- **CMake (3.22+):** Kitware, Inc. — Licencia **BSD 3-Clause**.
- **NSIS — Nullsoft Scriptable Install System (3.x):** Equipos de desarrollo de NSIS — Licencia **zlib/libpng** (con excepciones aplicables al stub descompresor).

---

## 3. Dependencias Externas en Tiempo de Ejecución (Instaladas por el Usuario)

**Dash Competitivo Audio NO empaqueta, redistribuye ni modifica** ninguno de los siguientes programas o conjuntos de datos en su repositorio ni en su instalador. Son herramientas independientes que el usuario final instala en su propio sistema Windows:

### Equalizer APO
- **Autor:** Jonas Thedering
- **Sitio oficial:** https://sourceforge.net/projects/equalizerapo/
- **Licencia:** **GNU General Public License v2.0 (GPLv2)**
- **Relación con el proyecto:** Dash Competitivo Audio actúa como generador externo del archivo de configuración de texto plano (`config.txt`), sin enlazar estática ni dinámicamente con binarios de Equalizer APO.

### HeSuVi (Headphone Surround Virtualization)
- **Autor:** MattGore / Colaboradores de HeSuVi
- **Sitio oficial:** https://sourceforge.net/projects/hesuvi/
- **Licencia:** Aplicación bajo **GPLv3**; las respuestas al impulso HRIR incluidas en la distribución oficial de HeSuVi pertenecen a sus respectivos autores bajo sus propias licencias.
- **Relación con el proyecto:** Dash Competitivo Audio únicamente referencia la ruta local `Include: HeSuVi\hesuvi.txt` si está presente en el equipo del usuario o facilita el enlace/descarga directa desde su repositorio oficial en SourceForge sin redistribuir sus archivos HRIR.

### VB-Audio Virtual Cable / Hi-Fi Cable / VoiceMeeter
- **Autor:** VB-Audio Software (Vincent Burel)
- **Sitio oficial:** https://vb-audio.com/
- **Licencia:** Propietaria (*Freeware / Donationware*)
- **Relación con el proyecto:** Dash Competitivo Audio consulta los dispositivos de audio activos de Windows mediante las APIs estándar de Microsoft (`MMDevice` / WASAPI) y abre la página web oficial de VB-Audio para que el usuario descargue e instale el controlador por su cuenta.

---

## 4. Declaración de Independencia de Motores Propietarios y Bases de Datos Restrictivas

Tras la revisión integral del código fuente y activos del repositorio, se certifica expresamente que:

1. **Sin motores propietarios de terceros:** Este proyecto **no utiliza, enlaza ni incluye** el motor propietario *"ATK Spatial Engine Bravo"* ni ningún binario cerrado equivalente. Todo el procesamiento DSP (`CenterTransientSuppressor`, `HybridLoudnessCompressor`, `KWeightingFilter` y `MultibandAmbienceGuard`) está implementado desde cero en C++20 sobre primitivas abiertas.
2. **Sin perfiles con licencias restrictivas (CC BY-NC-SA):** Este proyecto **no contiene, importa ni distribuye** curvas ni perfiles pertenecientes a *"ArtTuneDB"* ni activos sujetos a licencias no comerciales (*Creative Commons BY-NC-SA*). Todos los perfiles tácticos (`Campaign`, `Eclipse`) y la curva paramétrica/gráfica de 31 bandas se generan de manera procedural mediante código propio en `ApoConfigInjector`.
