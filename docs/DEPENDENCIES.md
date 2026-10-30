# Inventario Exhaustivo de Dependencias y Exclusiones

Este documento proporciona una auditoría detallada de todas las dependencias directas, indirectas, de construcción y del entorno de ejecución de **Dash Competitivo Audio**.

---

## 1. Dependencias de Compilación y Enlace (Incrustadas en Binarios)

| Dependencia | Versión | Origen | Licencia | Propósito |
| :--- | :--- | :--- | :--- | :--- |
| **JUCE Framework** | 8.0.4 | GitHub (`juce-framework/JUCE`) vía CMake `FetchContent` | **GNU AGPLv3** (esquema dual) | Interfaz gráfica nativa de escritorio (`juce_gui_basics`, `juce_gui_extra`), infraestructura DSP (`juce_dsp`) y envoltorios de plugins (`juce_audio_utils`). |
| **Steinberg VST3 SDK** | Integrada en JUCE 8.0.4 | Incluido internamente en el árbol de módulos de JUCE | **GNU GPLv3** (esquema dual con licencia propietaria) | Interfaz de comunicación host-plugin para exportar los procesadores de audio en formato VST3. |
| **Windows SDK** | 10.0.19041+ / 11 | Herramientas de Visual Studio 2022 | Licencia de Microsoft Windows SDK | Interfaces de Core Audio: `MMDeviceAPI`, `FunctionDiscoveryKeys_devpkey`, `IPolicyConfig` COM, `URLMon` y `ShellAPI`. |

---

## 2. Herramientas de Construcción y Empaquetado

| Herramienta | Versión Mínima | Licencia | Función |
| :--- | :--- | :--- | :--- |
| **CMake** | 3.22+ | BSD 3-Clause | Generación determinista de soluciones MSVC para x64. |
| **MSVC Toolset (v143)** | VS 2022 v17.x | Comercial / Community de Microsoft | Compilación con optimizaciones estándar C++20 (`/O2 /utf-8 /permissive-`). |
| **NSIS** | 3.x | zlib/libpng | Creación del instalador comercial/redistribuible `DashCompetitivoAudio-setup.exe`. |

---

## 3. Dependencias Externas del Entorno del Usuario (No Distribuidas)

Dash Competitivo Audio **no distribuye, no incrusta ni empaqueta** estos componentes en su repositorio; el software orquesta su configuración en el sistema del usuario final:

1. **Equalizer APO:**
   - **Licencia:** GNU GPLv2.
   - **Interacción:** `App` escribe atómicamente un archivo de directivas de texto plano (`C:\Program Files\EqualizerAPO\config\config.txt`). No existe enlace dinámico ni estático con binarios de Equalizer APO.
2. **HeSuVi (Headphone Surround Virtualization):**
   - **Licencia:** Aplicación bajo GPLv3; archivos HRIR individuales sujetos a las licencias de sus respectivos autores.
   - **Interacción:** Se referencia mediante directiva `Include: HeSuVi\hesuvi.txt`. La aplicación provee un enlace o descarga directa opcional desde SourceForge sin incorporar los archivos de audio en el código fuente.
3. **VB-Audio Virtual Hi-Fi Cable / VoiceMeeter:**
   - **Licencia:** Propietaria (*Freeware / Donationware*).
   - **Interacción:** El sistema interactúa mediante llamadas estándar WASAPI de Windows para consultar endpoints y establecer el dispositivo predeterminado mediante COM.

---

## 4. Registro Formal de Exclusiones (Tecnologías Prohibidas)

Se certifica formalmente que el proyecto **no contiene ni utiliza**:

- **ATK Spatial Engine Bravo:** Ningún binario, algoritmo ni referencia técnica a este motor cerrado.
- **ArtTuneKit / ArtTuneDB:** Ninguna curva ecualizadora, perfil de sonido ni activo con licencia *Creative Commons Atribución-NoComercial-CompartirIgual (CC BY-NC-SA)*. Todos los perfiles son generados de manera procedural en C++20 propio.
