/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para ApoConfigManager y ApoConfigInjector
 *
 * Cobertura de pruebas preparadas:
 *  1. Inyección no destructiva: Preservación de directivas preexistentes en config.txt.
 *  2. Generación atómica y respaldo (.dash_backup): Escritura segura sin destruir filtros ajenos.
 *  3. Verificación de delimitadores canónicos (kBeginMarker / kEndMarker).
 *  4. Remoción de sección administrada: Limpieza segura sin alterar configuraciones del usuario.
 *  5. Idempotencia ante inyecciones repetidas.
 */

#include <Core/ApoConfigManager.h>
#include <Core/ApoConfigInjector.h>

#include <cassert>
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

    // 1. Simular un config.txt preexistente del usuario con ecualizaciones previas
    {
        std::ofstream out(configFile, std::ios::binary | std::ios::trunc);
        out << "# Ecualización previa del usuario\r\n";
        out << "GraphicEQ: 100 0; 1000 2; 10000 -1\r\n";
    }

    ApoChainConfig chain;
    chain.profileName               = "Warzone Competitive";
    chain.preampDb                  = -6.0;
    chain.enableTransientSuppressor = true;
    chain.gunVolume                 = 0.45;
    chain.enableLoudnessCompressor  = true;
    chain.compAmount                = 0.60;
    chain.compTarget                = ( -16.0 + 28.0 ) / 36.0;
    chain.enableAmbienceGuard       = true;
    chain.ambGuard                  = 0.85;
    chain.enableGraphicEq           = true;
    chain.footstepsEqDb             = 2.0;
    chain.bassEqDb                  = 1.0;

    const std::string directives = ApoConfigInjector::buildConfigContent(chain, configFile);

    std::string err;
    const ApoConfigResult res = ApoConfigManager::injectManagedSection(configFile, directives, &err);
    assert(res == ApoConfigResult::Success);
    assert(err.empty());
    assert(ApoConfigManager::hasManagedSection(configFile));
    assert(ApoConfigManager::hasBackup(configFile));

    // 2. Verificar que el archivo config.txt conserva las líneas del usuario y el bloque gestionado
    {
        std::ifstream in(configFile, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        assert(content.find("GraphicEQ: 100 0; 1000 2; 10000 -1") != std::string::npos);
        assert(content.find(ApoConfigManager::kBeginMarker) != std::string::npos);
        assert(content.find(ApoConfigManager::kEndMarker) != std::string::npos);
        assert(content.find("Preamp: -6 dB") != std::string::npos);
    }

    // 3. Remoción de sección gestionada: debe retirar el bloque de Dash sin borrar las líneas del usuario
    const ApoConfigResult removeRes = ApoConfigManager::removeManagedSection(configFile, &err);
    assert(removeRes == ApoConfigResult::Success);
    assert(!ApoConfigManager::hasManagedSection(configFile));
    {
        std::ifstream in(configFile, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        assert(content.find("GraphicEQ: 100 0; 1000 2; 10000 -1") != std::string::npos);
        assert(content.find(ApoConfigManager::kBeginMarker) == std::string::npos);
    }

    // Limpieza
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
    chain.footstepsEqDb = 1.5;
    const std::string directives = ApoConfigInjector::buildConfigContent(chain, configFile);

    // Llamadas repetidas no deben duplicar el marcador administrado en config.txt
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

} // namespace dash::test
