/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para ApoConfigManager y ApoConfigInjector (6 Bandas Biquad ±18 dB)
 */

#include <Core/AppSettingsStore.h>
#include <Core/ApoConfigManager.h>
#include <Core/ApoConfigInjector.h>
#include <Core/AudioPolicyManager.h>

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

namespace dash::test
{

void testApoConfigManagerNonDestructiveInjection()
{
    using namespace dash::audio;

    const auto tempDir = std::filesystem::temp_directory_path() / "DashApoTest";
    std::filesystem::create_directories(tempDir);

    const auto configFile = tempDir / "config.txt";

    {
        std::ofstream out(configFile, std::ios::binary | std::ios::trunc);
        out << "# Ecualización previa del usuario\r\n";
        out << "GraphicEQ: 100 0; 1000 2; 10000 -1\r\n";
    }

    ApoChainConfig chain;
    chain.preampDb    = -18.0;
    chain.bandGainsDb = { -18.0, -9.5, 0.5, 14.5, 18.0, -15.25 };
    chain.bypass      = false;

    const std::string directives = ApoConfigInjector::buildConfigContent(chain, configFile);

    // Verificar las 6 directivas Biquad LS / PK / HS con rango extremo [-18 .. +18 dB]
    assert(directives.find("Copy:") == std::string::npos);
    assert(directives.find("Preamp: -18 dB") != std::string::npos);
    assert(directives.find("Filter 1: ON LS Fc 60 Hz Gain -18 dB") != std::string::npos);
    assert(directives.find("Filter 2: ON LS Fc 150 Hz Gain -9.5 dB") != std::string::npos);
    assert(directives.find("Filter 3: ON PK Fc 400 Hz Gain 0.5 dB Q 1") != std::string::npos);
    assert(directives.find("Filter 4: ON PK Fc 1500 Hz Gain 14.5 dB Q 1.2") != std::string::npos);
    assert(directives.find("Filter 5: ON PK Fc 3500 Hz Gain 18 dB Q 1.2") != std::string::npos);
    assert(directives.find("Filter 6: ON HS Fc 8000 Hz Gain -15.25 dB") != std::string::npos);

    std::string err;
    const ApoConfigResult res = ApoConfigManager::injectManagedSection(configFile, directives, &err);
    assert(res == ApoConfigResult::Success);
    assert(err.empty());
    assert(ApoConfigManager::hasManagedSection(configFile));
    assert(ApoConfigManager::hasBackup(configFile));

    {
        std::ifstream in(configFile, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        assert(content.find("GraphicEQ: 100 0; 1000 2; 10000 -1") != std::string::npos);
        assert(content.find(ApoConfigManager::kBeginMarker) != std::string::npos);
        assert(content.find(ApoConfigManager::kEndMarker) != std::string::npos);
        assert(content.find("Preamp: -18 dB") != std::string::npos);
        assert(content.find("Filter 5: ON PK Fc 3500 Hz Gain 18 dB Q 1.2") != std::string::npos);
    }

    const ApoConfigResult removeRes = ApoConfigManager::removeManagedSection(configFile, &err);
    assert(removeRes == ApoConfigResult::Success);
    assert(!ApoConfigManager::hasManagedSection(configFile));
    {
        std::ifstream in(configFile, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        assert(content.find("GraphicEQ: 100 0; 1000 2; 10000 -1") != std::string::npos);
        assert(content.find(ApoConfigManager::kBeginMarker) == std::string::npos);
    }

    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);
}

void testApoConfigManagerIdempotence()
{
    using namespace dash::audio;

    const auto tempDir = std::filesystem::temp_directory_path() / "DashApoIdempotenceTest";
    std::filesystem::create_directories(tempDir);

    const auto configFile = tempDir / "config.txt";

    ApoChainConfig chain;
    chain.bandGainsDb[3] = 16.5;
    const std::string directives = ApoConfigInjector::buildConfigContent(chain, configFile);

    assert(ApoConfigManager::injectManagedSection(configFile, directives) == ApoConfigResult::Success);
    assert(ApoConfigManager::injectManagedSection(configFile, directives) == ApoConfigResult::Success);

    {
        std::ifstream in(configFile, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        size_t count = 0;
        size_t pos = 0;
        const std::string marker(ApoConfigManager::kBeginMarker);
        while ((pos = content.find(marker, pos)) != std::string::npos)
        {
            ++count;
            pos += marker.length();
        }

        assert(count == 1);
    }

    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);
}

void testEndpointGuidExtraction()
{
    using namespace dash::audio;

    const std::wstring mmDeviceId = L"{0.0.0.00000000}.{c48a7312-32b4-4b51-9efb-3375cb474c10}";
    const std::wstring extracted = AudioPolicyManager::extractEndpointGuid(mmDeviceId);
    assert(extracted == L"{c48a7312-32b4-4b51-9efb-3375cb474c10}");

    const std::wstring bareGuid = L"{c48a7312-32b4-4b51-9efb-3375cb474c10}";
    assert(AudioPolicyManager::extractEndpointGuid(bareGuid) == L"{c48a7312-32b4-4b51-9efb-3375cb474c10}");

    const std::wstring friendlyName = L"Speakers (Realtek High Definition Audio)";
    assert(AudioPolicyManager::extractEndpointGuid(friendlyName) == friendlyName);

    assert(AudioPolicyManager::extractEndpointGuid(L"").empty());

    const auto tempDir = std::filesystem::temp_directory_path() / "DashApoGuidTest";
    std::filesystem::create_directories(tempDir);
    const auto configFile = tempDir / "config.txt";

    ApoChainConfig chain;
    chain.targetEndpointGuid = "{c48a7312-32b4-4b51-9efb-3375cb474c10}";
    const std::string directives = ApoConfigInjector::buildConfigContent(chain, configFile);
    assert(directives.find("Device: {c48a7312-32b4-4b51-9efb-3375cb474c10}") != std::string::npos);

    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);
}

void testApoInstallationCheck()
{
    using namespace dash::audio;
    // Comprueba que la detección se ejecute de forma segura sin excepciones
    const bool installed = ApoConfigManager::isApoInstalled();
    (void)installed;
}

void testApoConfigRestoreBackup()
{
    using namespace dash::audio;

    const auto tempDir = std::filesystem::temp_directory_path() / "DashApoRestoreBackupTest";
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);
    std::filesystem::create_directories(tempDir, ec);

    const auto configFile = tempDir / "config.txt";
    const auto backupFile = tempDir / "config.txt.bak";

    const std::string originalContent =
        "# Configuracion original intacta\r\n"
        "Preamp: -2.5 dB\r\n"
        "Filter 1: ON PK Fc 1000 Hz Gain 1.5 dB Q 1.41\r\n";

    {
        std::ofstream out(configFile, std::ios::binary | std::ios::trunc);
        out.write(originalContent.data(), static_cast<std::streamsize>(originalContent.size()));
    }

    ApoChainConfig firstInjection;
    firstInjection.preampDb    = 6.0;
    firstInjection.bandGainsDb = { 4.0, -3.0, 2.0, 9.5, 11.0, -5.0 };

    std::string err;
    assert(ApoConfigManager::injectManagedSection(
        configFile,
        ApoConfigInjector::buildConfigContent(firstInjection, configFile),
        &err) == ApoConfigResult::Success);

    assert(std::filesystem::exists(backupFile, ec));
    assert(ApoConfigManager::hasBackup(configFile));

    // Una segunda inyección NO debe sobrescribir config.txt.bak
    ApoChainConfig secondInjection;
    secondInjection.preampDb    = -10.0;
    secondInjection.bandGainsDb = { -12.0, 0.0, 0.0, 15.0, 18.0, -8.0 };
    assert(ApoConfigManager::injectManagedSection(
        configFile,
        ApoConfigInjector::buildConfigContent(secondInjection, configFile),
        &err) == ApoConfigResult::Success);

    // Restaurar config.txt original desde config.txt.bak
    err.clear();
    const ApoConfigResult restoreRes = ApoConfigManager::restoreOriginalConfig(configFile, &err);
    assert(restoreRes == ApoConfigResult::Success);
    assert(err.empty());
    assert(!ApoConfigManager::hasManagedSection(configFile));

    {
        std::ifstream in(configFile, std::ios::binary);
        const std::string restoredBytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        assert(restoredBytes == originalContent);
    }

    std::filesystem::remove_all(tempDir, ec);
}

void testAppSettingsPersistence()
{
    using namespace dash::audio;

    const auto tempDir = std::filesystem::temp_directory_path() / "DashSettingsPersistenceTest";
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);
    std::filesystem::create_directories(tempDir, ec);

    const juce::File jsonFile(juce::String((tempDir / "DashAudioSettings.json").string()));

    // Si no existe archivo, load devuelve false y valores neutros
    AppPersistedSettings missingLoad;
    assert(!AppSettingsStore::load(missingLoad, jsonFile));
    assert(std::abs(missingLoad.preampDb) < 1e-5f);
    assert(!missingLoad.bypass);

    // Guardado y lectura completa + verificación de clamp a [-18, +18 dB]
    AppPersistedSettings toSave;
    toSave.preampDb    = -6.5f;
    toSave.bandGainsDb = { -25.0f, -9.5f, 0.0f, 7.25f, 18.0f, 30.0f };
    toSave.bypass      = true;

    assert(AppSettingsStore::save(toSave, jsonFile));
    assert(jsonFile.existsAsFile());

    AppPersistedSettings loaded;
    assert(AppSettingsStore::load(loaded, jsonFile));
    assert(std::abs(loaded.preampDb - (-6.5f)) < 1e-4f);
    assert(std::abs(loaded.bandGainsDb[0] - (-18.0f)) < 1e-4f);
    assert(std::abs(loaded.bandGainsDb[1] - (-9.5f)) < 1e-4f);
    assert(std::abs(loaded.bandGainsDb[2] - 0.0f) < 1e-4f);
    assert(std::abs(loaded.bandGainsDb[3] - 7.25f) < 1e-4f);
    assert(std::abs(loaded.bandGainsDb[4] - 18.0f) < 1e-4f);
    assert(std::abs(loaded.bandGainsDb[5] - 18.0f) < 1e-4f);
    assert(loaded.bypass == true);

    std::filesystem::remove_all(tempDir, ec);
}

} // namespace dash::test
