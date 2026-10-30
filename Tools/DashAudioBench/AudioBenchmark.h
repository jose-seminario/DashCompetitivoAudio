#pragma once

#include "AudioMetrics.h"
#include "LatencyAnalyzer.h"
#include "ABComparison.h"

#include <chrono>
#include <string>
#include <vector>

namespace dash::bench
{

struct CpuBenchmarkResult
{
    double meanExecutionUs = 0.0;
    double p50ExecutionUs  = 0.0;
    double p95ExecutionUs  = 0.0;
    double p99ExecutionUs  = 0.0;
    double maxExecutionUs  = 0.0;
    double cpuLoadPercent  = 0.0;
    size_t iterations      = 0;
};

struct BenchmarkSuiteReport
{
    std::string platform = "Windows x64 (AVX2)";
    double sampleRate = 48000.0;
    int blockSize = 256;
    int numChannels = 8; // 7.1 Surround

    CpuBenchmarkResult cpu;
    LatencyReport latency;
    QualityMetrics quality;
    bool passedAllIntegrityChecks = true;
};

/**
 * Suite Central de Benchmarks y QA de DashCompetitivoAudio (AudioBenchmark).
 */
class AudioBenchmark
{
public:
    static CpuBenchmarkResult runCpuBenchmark(int numChannels = 8, int blockSize = 256, size_t iterations = 1000);
    static BenchmarkSuiteReport runFullSuite(double sampleRate = 48000.0, int blockSize = 256, int numChannels = 8);
    static std::string formatReportText(const BenchmarkSuiteReport& report);
    static std::string formatReportJson(const BenchmarkSuiteReport& report);
};

} // namespace dash::bench
