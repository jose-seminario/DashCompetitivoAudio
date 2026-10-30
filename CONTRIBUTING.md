# Guía de Contribución - Dash Competitivo Audio

Agradecemos el interés en colaborar con **Dash Competitivo Audio**. Para mantener la robustez, el rendimiento en tiempo real y el cumplimiento legal estricto, todas las contribuciones deben seguir estas directrices.

---

## 1. Principios de Diseño y Estándares de Código

1. **Estándar C++20:** Se utiliza C++20 estricto (`/permissive- /utf-8 /W4` en MSVC). El código debe ser limpio, portable y auto-documentado.
2. **Separación de Responsabilidades:**
   - La lógica matemática y de procesamiento de señales pertenece exclusivamente a `DashAudioCore`.
   - Ninguna clase pública en `DashAudioCore` debe incluir cabeceras de JUCE ni exponer tipos del framework (`juce::AudioBuffer`, `juce::String`, etc.).
   - Utilizar el patrón **PIMPL** (`std::unique_ptr<Impl>`) para encapsular detalles de implementación interna.
3. **Reglas de Oro de Audio en Tiempo Real (Real-Time Safety):**
   - **Prohibido alojar memoria en el heap** dentro de funciones de audio (`process()`, `processBlock()`, `computeSampleGainLinear()`). Cero `new`, `malloc`, `std::vector::push_back`, `std::string` o `std::make_unique`.
   - **Prohibido realizar llamadas bloqueantes:** No usar `std::mutex`, `std::condition_variable`, `Sleep`, ni operaciones de entrada/salida a disco o red en el hilo de audio.
   - Toda asignación y configuración pesada de buffers o filtros debe ejecutarse durante la llamada a `prepare()`.

---

## 2. Conformidad Legal y de Licenciamiento

- Este proyecto está licenciado bajo la **GNU Affero General Public License v3.0 (AGPLv3)**.
- **Prohibido incorporar código propietario o no autorizado:**
  - No se aceptará bajo ninguna circunstancia código perteneciente a *"ArtTuneKit"* o al motor *"Bravo"*.
  - No se admiten perfiles o ecualizaciones extraídas de repositorios con licencias restrictivas o no comerciales (*CC BY-NC-SA*, como *ArtTuneDB*).
  - Cualquier algoritmo agregado debe ser de autoría propia o basarse en estándares públicos abiertos (p. ej. normas ITU-R, AES, IEEE) con cita de procedencia.

---

## 3. Convenciones de Mensajes de Commit

Adoptamos la especificación de **Conventional Commits**:

- `feat:` Nueva funcionalidad o algoritmo DSP.
- `fix:` Corrección de errores en DSP, UI o inyección APO.
- `refactor:` Reorganización de código sin cambios en el comportamiento.
- `docs:` Cambios o mejoras exclusivas en la documentación.
- `perf:` Optimizaciones de rendimiento (p. ej. vectorización SIMD).
- `test:` Adición o mejora de pruebas unitarias o de integración.

Ejemplo:
```text
feat(core): implement SSE4.1 vectorized loop in TransientProcessor
```

---

## 4. Flujo de Trabajo para Pull Requests

1. Crea una rama descriptiva a partir de `main`:
   ```bash
   git checkout -b feat/nombre-de-la-mejora
   ```
2. Realiza tus cambios asegurándote de no romper la compatibilidad con procesadores x64 estándar.
3. Verifica que la compilación en Visual Studio 2022 x64 Release sea exitosa y sin advertencias (`/W4`).
4. Abre un Pull Request describiendo detalladamente los cambios realizados, las pruebas efectuadas y el impacto en rendimiento o latencia.
