/**
 * DashCompetitivoAudio - Ejecutor Principal de Pruebas Automatizadas (CTest Runner)
 *
 * Suites activas del Ecualizador Táctico Estéreo de 6 Bandas:
 *  1. EQProcessorTests
 *  2. LimiterProcessorTests
 *  3. AudioGraphTests
 *  4. ApoConfigManagerTests
 */

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace dash::test
{
    // 1. EQProcessorTests
    void testSixBandDefaultConfiguration();
    void testLowShelfAndPeakingBoost();
    void testHighShelfAttenuation();
    void testStereoChannelIsolation();

    // 2. LimiterProcessorTests
    void testLimiterCeilingClamping();
    void testLimiterMultichannelLink();
    void testLimiterZeroLatency();

    // 3. AudioGraphTests
    void testAudioGraphStereoSixBandProcessing();
    void testAudioGraphStereoChannelIsolation();
    void testAudioGraphLimiterProtection();
    void testAudioGraphBypassAll();
    void testAudioGraphSampleRateAndBlockSizeVariations();

    // 4. ApoConfigManagerTests
    void testApoConfigManagerNonDestructiveInjection();
    void testApoConfigManagerIdempotence();
    void testEndpointGuidExtraction();
    void testApoInstallationCheck();
} // namespace dash::test

int main(int argc, char* argv[])
{
    using namespace dash::test;

    struct SuiteEntry
    {
        const char* name;
        std::function<void()> run;
    };

    const std::vector<SuiteEntry> suites = {
        { "EQProcessorTests", [] {
            testSixBandDefaultConfiguration();
            testLowShelfAndPeakingBoost();
            testHighShelfAttenuation();
            testStereoChannelIsolation();
        }},
        { "LimiterProcessorTests", [] {
            testLimiterCeilingClamping();
            testLimiterMultichannelLink();
            testLimiterZeroLatency();
        }},
        { "AudioGraphTests", [] {
            testAudioGraphStereoSixBandProcessing();
            testAudioGraphStereoChannelIsolation();
            testAudioGraphLimiterProtection();
            testAudioGraphBypassAll();
            testAudioGraphSampleRateAndBlockSizeVariations();
        }},
        { "ApoConfigManagerTests", [] {
            testApoConfigManagerNonDestructiveInjection();
            testApoConfigManagerIdempotence();
            testEndpointGuidExtraction();
            testApoInstallationCheck();
        }}
    };

    const std::string filter = (argc > 1 && argv[1] != nullptr) ? std::string(argv[1]) : std::string();
    int executedCount = 0;

    try
    {
        for (const auto& suite : suites)
        {
            if (!filter.empty() && filter != suite.name)
                continue;

            std::cout << "[RUN ] " << suite.name << std::endl;
            suite.run();
            std::cout << "[PASS] " << suite.name << std::endl;
            ++executedCount;
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "[FAIL] Excepción no controlada: " << ex.what() << std::endl;
        return 1;
    }

    if (!filter.empty() && executedCount == 0)
    {
        std::cerr << "[ERROR] Suite desconocida: " << filter << std::endl;
        return 2;
    }

    std::cout << "[OK  ] Suites completadas con éxito: " << executedCount << std::endl;
    return 0;
}
