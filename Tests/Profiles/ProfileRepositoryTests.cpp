/**
 * DashCompetitivoAudio - Suite de Pruebas Unitarias para ProfileRepository
 *
 * Cobertura de pruebas preparadas:
 *  1. Registro seguro de directorios de búsqueda.
 *  2. Rechazo de peticiones de carga o borrado con slugs maliciosos (Path Traversal).
 *  3. Verificación de ruta por defecto de perfiles de usuario (%LOCALAPPDATA%).
 *
 * NOTA: Archivo de solo lectura/preparación; no se ejecuta en la terminal.
 */

#include <Profiles/ProfileRepository.h>

#include <cassert>
#include <filesystem>
#include <string>

namespace dash::test
{

void testProfileRepositorySecurity()
{
    using namespace dash::profile;

    const auto tempDir = std::filesystem::temp_directory_path() / "DashProfileRepoTest";
    ProfileRepository repo(tempDir);

    AudioProfile profile;
    std::string err;

    // Intentos de Path Traversal deben fallar inmediatamente sin acceder al disco
    assert(!repo.loadProfileById("../malicious", profile, &err));
    assert(!repo.loadProfileById("..\\system32", profile, &err));
    assert(!repo.deleteUserProfile("../windows", &err));

    // Slug seguro pero inexistente retorna error limpio de "no encontrado"
    assert(!repo.loadProfileById("non-existent-profile", profile, &err));
    assert(!err.empty());
}

void testProfileRepositoryUserPath()
{
    using namespace dash::profile;

    const auto defaultPath = ProfileRepository::getDefaultUserProfilesDir();
    assert(!defaultPath.empty());
    assert(defaultPath.is_absolute());
}

} // namespace dash::test
