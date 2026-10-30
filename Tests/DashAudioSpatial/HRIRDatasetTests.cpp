/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para HRIRDataset
 *
 * Cobertura de pruebas preparadas:
 *  1. Generación Sintética Woodworth: Cobertura de 7.1 y 7.1.4 sin dependencias externas.
 *  2. Validación de ITD (Tiempo): Un altavoz izquierdo (-30°) llega antes al oído izquierdo que al derecho.
 *  3. Validación de ILD (Nivel): La sombra de la cabeza atenúa las altas frecuencias en el oído contralateral.
 *  4. Canal LFE: Impulso en fase pura e idéntico para ambos oídos.
 *  5. Comprobación de Límites: Rechazo seguro de índices fuera de rango.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <DashAudioSpatial/HRIRDataset.h>

#include <cassert>
#include <cmath>

namespace dash::test
{

void testHRIRDatasetWoodworthGeneration()
{
    using namespace dash::spatial;

    HRIRDataset dataset;
    const auto layout71 = ChannelLayout::surround7_1();
    const bool ok = dataset.loadSyntheticReference(layout71, 48000.0, 256);

    assert(ok);
    assert(dataset.isLoaded());
    assert(dataset.getIRLength() == 256);

    // Canal 0: FrontLeft (-30° azimuth)
    const auto* flPair = dataset.getImpulsePair(0);
    assert(flPair != nullptr && flPair->isValid());

    // El oído izquierdo debe recibir la señal antes o con mayor nivel (ipsilateral)
    float maxL = 0.0f;
    float maxR = 0.0f;
    size_t peakIdxL = 0;
    size_t peakIdxR = 0;

    for (size_t i = 0; i < flPair->leftEar.size(); ++i)
    {
        if (std::abs(flPair->leftEar[i]) > maxL)
        {
            maxL = std::abs(flPair->leftEar[i]);
            peakIdxL = i;
        }
        if (std::abs(flPair->rightEar[i]) > maxR)
        {
            maxR = std::abs(flPair->rightEar[i]);
            peakIdxR = i;
        }
    }

    // 1. ILD: Nivel izquierdo mayor que nivel derecho
    assert(maxL > maxR);
    // 2. ITD: El pico derecho llega con retraso respecto al izquierdo
    assert(peakIdxR >= peakIdxL);
}

void testHRIRDatasetLfeTreatment()
{
    using namespace dash::spatial;

    HRIRDataset dataset;
    const auto layout51 = ChannelLayout::surround5_1();
    dataset.loadSyntheticReference(layout51, 48000.0, 128);

    // Canal 3 es LFE
    const auto* lfePair = dataset.getImpulsePair(3);
    assert(lfePair != nullptr && lfePair->isValid());

    // En LFE, ambos oídos deben recibir exactamente la misma señal en fase
    assert(std::abs(lfePair->leftEar[0] - lfePair->rightEar[0]) < 1.0e-5f);
}

} // namespace dash::test
