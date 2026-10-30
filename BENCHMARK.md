# Metodología de Benchmark y Rendimiento Acústico — DashCompetitivoAudio

Este documento describe la metodología de pruebas de rendimiento, latencia y calidad de audio utilizada por la herramienta `Tools/DashAudioBench`.

---

## 1. Métrica de Presupuesto de Tiempo de Búfer (Buffer Budget)
En el procesamiento de audio en tiempo real, el motor DSP debe completar el procesamiento del bloque completo antes de que el controlador de audio solicite las siguientes muestras. Si el cálculo supera este tiempo, se produce una desconexión acústica (*buffer underrun* o *xrun*), resultando en clics o chasquidos perceptibles.

$$\text{Presupuesto Temporal (\mu s)} = \frac{\text{Tamaño del Búfer}}{\text{Sample Rate}} \times 10^6$$

### Valores de Referencia a 48000 Hz:
- **128 muestras**: $2666.67 \text{ \mu s}$ ($2.67 \text{ ms}$)
- **256 muestras**: $5333.33 \text{ \mu s}$ ($5.33 \text{ ms}$) — *Presupuesto Estándar*
- **512 muestras**: $10666.67 \text{ \mu s}$ ($10.67 \text{ ms}$)

---

## 2. Métricas de CPU y Percentiles de Tiempo de Ejecución
El analizador `AudioBenchmark` evalúa 1000 iteraciones continuas y calcula:
- **Media**: Tiempo promedio por bloque.
- **Percentil 50 (p50)**: Mediana representativa del cálculo sostenido.
- **Percentil 95 (p95)**: Límite superior del 95% de los bloques procesados.
- **Percentil 99 (p99)**: Medida crítica de estabilidad frente a picos de interrupción de CPU.
- **Máximo**: Peor tiempo absoluto registrado.

### Criterio de Aceptación:
$$\text{Carga de CPU} = \left(\frac{\text{Media}(\mu s)}{\text{Presupuesto}(\mu s)}\right) \times 100 < 5.0\%$$
En CPUs x64 modernas con AVX2, la cadena completa multicanal de 8 canales (`AudioGraph`) consume menos del 2.0% del tiempo de búfer disponible.

---

## 3. Metodología de Comparación A/B con Compensación de Sonoridad
En la evaluación psicoacústica, los humanos tienden a percibir una señal más ruidosa o con mayor ganancia como de "mejor calidad", incluso si contiene más distorsión.

Para evaluar objetivamente la inteligibilidad táctica de pasos y recargas:
1. Se toma el audio original (A) y el audio procesado (B).
2. Se mide la sonoridad integrada en LUFS (ITU-R BS.1770-4) o RMS de ambas señales.
3. Se calcula la ganancia de compensación:
   $$\Delta G = \text{LUFS}_A - \text{LUFS}_B$$
4. Se aplica $\Delta G$ al buffer procesado, generando una versión $B_{\text{matched}}$ con sonoridad idéntica a la señal original.
5. Se calcula la reducción de rango dinámico (*Crest Factor*) y la ganancia selectiva en la banda de pisadas (2500 - 6500 Hz).
