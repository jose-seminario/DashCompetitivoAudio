/**
 * DashCompetitivoAudio - Ejecutor Principal de Pruebas Automatizadas (CTest Runner)
 *
 * Permite ejecutar la totalidad de las 13 suites o filtrar una suite individual
 * pasando su nombre como primer argumento de línea de comandos.
 */

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace dash::test
{
    // 1. DynamicsProcessorTests
    void testDynamicsSilenceAndNaN();
    void testDynamicsDownwardCompression();
    void testDynamicsUpwardCompression();
    void testDynamicsLfeExclusion();
    void testDynamicsBypass();

    // 2. TransientProcessorTests
    void testTransientGunCrackSuppression();
    void testTransientFootstepsPreservation();
    void testTransientMidSidePreservation();

    // 3. AmbienceProcessorTests
    void testAmbienceStationaryDucking();
    void testAmbienceDynamicSignalProtection();
    void testAmbienceCrossoverReconstruction();

    // 4. EQProcessorTests
    void testGraphicEQ31Bands();
    void testParametricPeakingFilter();
    void testParametricHighPassFilter();
    void testParametricNotchFilter();

    // 5. LimiterProcessorTests
    void testLimiterCeilingClamping();
    void testLimiterMultichannelLink();
    void testLimiterLfeIndependence();
    void testLimiterZeroLatency();

    // 6. AudioGraphTests
    void testAudioGraphFullProcessing();
    void testAudioGraphBypassAll();
    void testAudioGraphSampleRateAndBlockSizeVariations();
    void testAudioGraphMultichannelSurround();
    void testAudioGraph16ChannelsImmersive();

    // 7. ChannelLayoutTests
    void testStandardChannelLayouts();
    void testWindowsSpeakerMaskConversion();
    void testDiscreteCustomLayout();

    // 8. BinauralRendererTests
    void testBinauralRenderer71ToStereo();
    void testBinauralRendererLfeBypass();

    // 9. ConvolutionTests
    void testConvolutionUnitImpulse();
    void testConvolutionDelayImpulse();
    void testConvolutionBlockBoundaryContinuity();

    // 10. ProfileValidatorTests
    void testProfileValidatorMaliciousJsonRejection();
    void testProfileValidatorPathTraversalPrevention();
    void testProfileValidatorNumericRanges();

    // 11. ProfileSerializerTests
    void testProfileSerializerRoundTrip();
    void testProfileSerializerMalformedJsonRejection();

    // 12. ProfileMigratorTests
    void testProfileMigratorV0ToV1();
    void testProfileMigratorUnsupportedVersion();

    // 13. ApoConfigManagerTests
    void testApoConfigManagerNonDestructiveInjection();
    void testApoConfigManagerIdempotence();
    void testEndpointGuidExtraction();
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
        { "DynamicsProcessorTests", [] {
            testDynamicsSilenceAndNaN();
            testDynamicsDownwardCompression();
            testDynamicsUpwardCompression();
            testDynamicsLfeExclusion();
            testDynamicsBypass();
        }},
        { "TransientProcessorTests", [] {
            testTransientGunCrackSuppression();
            testTransientFootstepsPreservation();
            testTransientMidSidePreservation();
        }},
        { "AmbienceProcessorTests", [] {
            testAmbienceStationaryDucking();
            testAmbienceDynamicSignalProtection();
            testAmbienceCrossoverReconstruction();
        }},
        { "EQProcessorTests", [] {
            testGraphicEQ31Bands();
            testParametricPeakingFilter();
            testParametricHighPassFilter();
            testParametricNotchFilter();
        }},
        { "LimiterProcessorTests", [] {
            testLimiterCeilingClamping();
            testLimiterMultichannelLink();
            testLimiterLfeIndependence();
            testLimiterZeroLatency();
        }},
        { "AudioGraphTests", [] {
            testAudioGraphFullProcessing();
            testAudioGraphBypassAll();
            testAudioGraphSampleRateAndBlockSizeVariations();
            testAudioGraphMultichannelSurround();
            testAudioGraph16ChannelsImmersive();
        }},
        { "ChannelLayoutTests", [] {
            testStandardChannelLayouts();
            testWindowsSpeakerMaskConversion();
            testDiscreteCustomLayout();
        }},
        { "BinauralRendererTests", [] {
            testBinauralRenderer71ToStereo();
            testBinauralRendererLfeBypass();
        }},
        { "ConvolutionTests", [] {
            testConvolutionUnitImpulse();
            testConvolutionDelayImpulse();
            testConvolutionBlockBoundaryContinuity();
        }},
        { "ProfileValidatorTests", [] {
            testProfileValidatorMaliciousJsonRejection();
            testProfileValidatorPathTraversalPrevention();
            testProfileValidatorNumericRanges();
        }},
        { "ProfileSerializerTests", [] {
            testProfileSerializerRoundTrip();
            testProfileSerializerMalformedJsonRejection();
        }},
        { "ProfileMigratorTests", [] {
            testProfileMigratorV0ToV1();
            testProfileMigratorUnsupportedVersion();
        }},
        { "ApoConfigManagerTests", [] {
            testApoConfigManagerNonDestructiveInjection();
            testApoConfigManagerIdempotence();
            testEndpointGuidExtraction();
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
