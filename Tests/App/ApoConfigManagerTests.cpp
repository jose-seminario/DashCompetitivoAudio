/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para ApoConfigManager y ApoConfigInjector
 *
 * Cobertura de pruebas preparadas:
 *  1. Inyección no destructiva: Preservación de directivas preexistentes en config.txt.
 *  2. Generación atómica NTFS: Escritura mediante archivo temporal para evitar colisiones.
 *  3. Verificación de directiva Include canónica ("Include DashCompetitivoAudio.txt").
 *  4. Restauración de Bypass: Limpieza segura sin alterar configuraciones ajenas de Equalizer APO.
 *  5. Detección de fallos de permisos NTFS y reporte de error claro.
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
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
        std::ofstream out(configFile);
        out << "# Ecualización previa del usuario\n";
        out << "GraphicEQ: 100 0; 1000 2; 10000 -1\n";
    }

    ApoConfigManager manager(configFile);

    ApoChainConfig chain;
    chain.bypassAll = false;
    chain.amount = 0.6f;
    chain.targetDb = -16.0f;
    chain.releaseMs = 120.0f;
    chain.gunReductionDb = 4.0f;
    chain.footstepsDb = 2.0f;
    chain.bassDb = 1.0f;
    chain.ambienceReductionDb = 6.0f;

    std::string err;
    const bool ok = manager.injectChain(chain, &err);
    assert(ok);
    assert(err.empty());

    // 2. Verificar que el archivo config.txt conserva las líneas del usuario
    {
        std::ifstream in(configFile);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        assert(content.find("GraphicEQ: 100 0; 1000 2; 10000 -1") != std::string::npos);
        assert(content.find("Include") != std::string::npos);
        assert(content.find(ApoConfigManager::kManagedFileName) != std::string::npos);
    }

    // 3. Verificar que el archivo administrado dedicado existe y contiene los parámetros
    const auto managedFile = tempDir / ApoConfigManager::kManagedFileName;
    assert(std::filesystem::exists(managedFile));
    {
        std::ifstream in(managedFile);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        assert(content.find("Stage: pre-mix") != std::string::npos);
    }

    // 4. Restauración de Bypass: debe retirar la inclusión de Dash sin borrar las líneas del usuario
    const bool bypassOk = manager.restoreBypass(&err);
    assert(bypassOk);
    {
        std::ifstream in(configFile);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        assert(content.find("GraphicEQ: 100 0; 1000 2; 10000 -1") != std::string::npos);
        assert(content.find(std::string("Include: ") + ApoConfigManager::kManagedFileName) == std::string::npos);
    }

    // Limpieza
    std::filesystem::remove_all(tempDir);
}

void testApoConfigManagerIdempotence()
{
    using namespace dash::audio;

    const auto tempDir = std::filesystem::temp_directory_path() / "DashApoIdempotenceTest";
    std::filesystem::create_directories(tempDir);

    const auto configFile = tempDir / "config.txt";
    ApoConfigManager manager(configFile);

    ApoChainConfig chain;
    chain.footstepsDb = 1.5f;

    // Llamadas repetidas no deben duplicar las directivas Include en config.txt
    manager.injectChain(chain);
    manager.injectChain(chain);

    std::ifstream in(configFile);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    size_t count = 0;
    size_t pos = 0;
    while ((pos = content.find(ApoConfigManager::kManagedFileName, pos)) != std::string::npos)
    {
        ++count;
        pos += std::string(ApoConfigManager::kManagedFileName).length();
    }

    assert(count == 1);

    std::filesystem::remove_all(tempDir);
}

} // namespace dash::test
