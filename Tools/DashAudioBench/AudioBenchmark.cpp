#include "AudioBenchmark.h"
#include <AudioGraph.h>
#include <SpatialProcessor.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace dash::bench
{

CpuBenchmarkResult AudioBenchmark::runCpuBenchmark(int numChannels, int blockSize, size_t iterations)
{
    CpuBenchmarkResult res;
    res.iterations = iterations;

    dash::core::AudioGraph graph;
    graph.prepare(48000.0, blockSize, numChannels);

    // Preparar buffers planares
    std::vector<std::vector<float>> channelBuffers(static_cast<size_t>(numChannels), std::vector<float>(static_cast<size_t>(blockSize), 0.1f));
    std::vector<float*> channelPointers(static_cast<size_t>(numChannels));
    for (size_t ch = 0; ch < static_cast<size_t>(numChannels); ++ch)
        channelPointers[ch] = channelBuffers[ch].data();

    std::vector<double> timingsUs;
    timingsUs.reserve(iterations);

    for (size_t i = 0; i < iterations; ++i)
    {
        const auto start = std::chrono::high_resolution_clock::now();
        graph.process(channelPointers.data(), numChannels, blockSize);
        const auto end = std::chrono::high_resolution_clock::now();

        const double elapsedUs = std::chrono::duration<double, std::micro>(end - start).count();
        timingsUs.push_back(elapsedUs);
    }

    std::sort(timingsUs.begin(), timingsUs.end());

    double sum = 0.0;
    for (double t : timingsUs)
        sum += t;

    res.meanExecutionUs = sum / static_cast<double>(iterations);
    res.p50ExecutionUs  = timingsUs[iterations / 2];
    res.p95ExecutionUs  = timingsUs[static_cast<size_t>(static_cast<double>(iterations) * 0.95)];
    res.p99ExecutionUs  = timingsUs[static_cast<size_t>(static_cast<double>(iterations) * 0.99)];
    res.maxExecutionUs  = timingsUs.back();

    // Búfer de 256 samples a 48000 Hz dura: 5333.33 µs
    const double bufferBudgetUs = (static_cast<double>(blockSize) / 48000.0) * 1000000.0;
    res.cpuLoadPercent = (res.meanExecutionUs / bufferBudgetUs) * 100.0;

    return res;
}

BenchmarkSuiteReport AudioBenchmark::runFullSuite(double sampleRate, int blockSize, int numChannels)
{
    BenchmarkSuiteReport rep;
    rep.sampleRate = sampleRate;
    rep.blockSize = blockSize;
    rep.numChannels = numChannels;

    rep.cpu = runCpuBenchmark(numChannels, blockSize, 500);
    rep.latency = LatencyAnalyzer::evaluateChainLatency(blockSize, sampleRate, 0);

    // Test de calidad: señal senoidal real a 1000 Hz procesada a través del AudioGraph
    dash::core::AudioGraph graph;
    graph.prepare(sampleRate, blockSize, numChannels);

    std::vector<std::vector<float>> channels(static_cast<size_t>(numChannels), std::vector<float>(static_cast<size_t>(blockSize)));
    for (size_t ch = 0; ch < static_cast<size_t>(numChannels); ++ch)
    {
        for (int i = 0; i < blockSize; ++i)
        {
            const double t = static_cast<double>(i) / sampleRate;
            channels[ch][static_cast<size_t>(i)] = static_cast<float>(0.5 * std::sin(2.0 * 3.14159265358979323846 * 1000.0 * t));
        }
    }

    std::vector<float*> channelPtrs(static_cast<size_t>(numChannels));
    for (size_t ch = 0; ch < static_cast<size_t>(numChannels); ++ch)
        channelPtrs[ch] = channels[ch].data();

    graph.process(channelPtrs.data(), numChannels, blockSize);

    rep.quality = AudioMetrics::analyzeBuffer(channelPtrs[0], static_cast<size_t>(blockSize), sampleRate);

    rep.passedAllIntegrityChecks = (!rep.quality.isClipped && rep.cpu.cpuLoadPercent < 50.0);
    return rep;
}

std::string AudioBenchmark::formatReportText(const BenchmarkSuiteReport& r)
{
    std::ostringstream ss;
    ss << "=================================================================\n";
    ss << "   DashCompetitivoAudio - Reporte de Benchmark Acústico y QA     \n";
    ss << "=================================================================\n";
    ss << "Plataforma:       " << r.platform << "\n";
    ss << "Formato:          " << r.sampleRate << " Hz | " << r.numChannels << " Canales | Búfer: " << r.blockSize << " samples\n";
    ss << "-----------------------------------------------------------------\n";
    ss << "Rendimiento CPU (por bloque):\n";
    ss << "  - Media:        " << std::fixed << std::setprecision(2) << r.cpu.meanExecutionUs << " µs\n";
    ss << "  - Percentil 50: " << r.cpu.p50ExecutionUs << " µs\n";
    ss << "  - Percentil 95: " << r.cpu.p95ExecutionUs << " µs\n";
    ss << "  - Percentil 99: " << r.cpu.p99ExecutionUs << " µs\n";
    ss << "  - Máximo:       " << r.cpu.maxExecutionUs << " µs\n";
    ss << "  - Carga Hilo:   " << r.cpu.cpuLoadPercent << " %\n";
    ss << "-----------------------------------------------------------------\n";
    ss << "Latencia Acústica:\n";
    ss << "  - Búfer:        " << r.latency.bufferDurationMs << " ms\n";
    ss << "  - Algorítmica:  " << r.latency.algorithmicDelayMs << " ms (" << (r.latency.isZeroLatencyStage ? "Cero Latencia pura" : "Con retardo") << ")\n";
    ss << "  - Round-Trip:   ~" << r.latency.roundTripEstimateMs << " ms\n";
    ss << "-----------------------------------------------------------------\n";
    ss << "Calidad de Audio e Integridad:\n";
    ss << "  - Peak:         " << r.quality.peakDbFs << " dBFS\n";
    ss << "  - RMS:          " << r.quality.rmsDbFs << " dBFS\n";
    ss << "  - Clipping:     " << (r.quality.isClipped ? "DETECTADO [FALLO]" : "0 muestras (Limpio)") << "\n";
    ss << "  - Estado QA:    " << (r.passedAllIntegrityChecks ? "SUPERADO [LISTO]" : "FALLIDO") << "\n";
    ss << "=================================================================\n";
    return ss.str();
}

std::string AudioBenchmark::formatReportJson(const BenchmarkSuiteReport& r)
{
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"platform\": \"" << r.platform << "\",\n";
    ss << "  \"sampleRate\": " << r.sampleRate << ",\n";
    ss << "  \"blockSize\": " << r.blockSize << ",\n";
    ss << "  \"numChannels\": " << r.numChannels << ",\n";
    ss << "  \"cpu\": {\n";
    ss << "    \"meanExecutionUs\": " << r.cpu.meanExecutionUs << ",\n";
    ss << "    \"p50ExecutionUs\": " << r.cpu.p50ExecutionUs << ",\n";
    ss << "    \"p95ExecutionUs\": " << r.cpu.p95ExecutionUs << ",\n";
    ss << "    \"p99ExecutionUs\": " << r.cpu.p99ExecutionUs << ",\n";
    ss << "    \"maxExecutionUs\": " << r.cpu.maxExecutionUs << ",\n";
    ss << "    \"cpuLoadPercent\": " << r.cpu.cpuLoadPercent << "\n";
    ss << "  },\n";
    ss << "  \"latency\": {\n";
    ss << "    \"bufferDurationMs\": " << r.latency.bufferDurationMs << ",\n";
    ss << "    \"algorithmicDelayMs\": " << r.latency.algorithmicDelayMs << ",\n";
    ss << "    \"roundTripEstimateMs\": " << r.latency.roundTripEstimateMs << "\n";
    ss << "  },\n";
    ss << "  \"passed\": " << (r.passedAllIntegrityChecks ? "true" : "false") << "\n";
    ss << "}\n";
    return ss.str();
}

} // namespace dash::bench

int main()
{
    const auto report = dash::bench::AudioBenchmark::runFullSuite(48000.0, 256, 8);
    std::cout << dash::bench::AudioBenchmark::formatReportText(report) << std::endl;
    return report.passedAllIntegrityChecks ? 0 : 1;
}
