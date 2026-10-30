#pragma once

#include <filesystem>
#include <string>

namespace dash::audio
{

enum class ApoConfigResult
{
    Success,
    PermissionDenied,
    ExternalConflictDetected,
    BackupFailed,
    WriteFailed,
    FileNotFound
};

/**
 * Gestor No Destructivo del archivo config.txt de Equalizer APO (ApoConfigManager).
 *
 * Principios de seguridad:
 *  1. Conserva 100% de las directivas y filtros preexistentes del usuario.
 *  2. Realiza un backup previo (.dash_backup) antes de modificar el archivo original por primera vez.
 *  3. Inyecta y actualiza exclusivamente la sección delimitada entre marcadores:
 *       # BEGIN DASHCOMPETITIVOAUDIO MANAGED BLOCK
 *       ...
 *       # END DASHCOMPETITIVOAUDIO MANAGED BLOCK
 *  4. Al desactivar el perfil, remueve únicamente la sección administrada, restaurando
 *     las directivas originales del usuario sin vaciar el archivo.
 *  5. Detecta modificaciones externas concurrentes para evitar sobrescribir cambios ajenos.
 *  6. Utiliza escritura atómica en el sistema de archivos NTFS.
 */
class ApoConfigManager
{
public:
    static constexpr const char* kBeginMarker = "# BEGIN DASHCOMPETITIVOAUDIO MANAGED BLOCK";
    static constexpr const char* kEndMarker   = "# END DASHCOMPETITIVOAUDIO MANAGED BLOCK";
    static constexpr const char* kBackupExt   = ".dash_backup";

    /**
     * Inyecta o actualiza el bloque de directivas de Dash respetando todo contenido externo.
     */
    static ApoConfigResult injectManagedSection(
        const std::filesystem::path& configPath,
        const std::string& managedDirectives,
        std::string* outError = nullptr);

    /**
     * Remueve la sección administrada por Dash y restituye intacto el resto del archivo.
     */
    static ApoConfigResult removeManagedSection(
        const std::filesystem::path& configPath,
        std::string* outError = nullptr);

    /**
     * Restaura el archivo original a partir del backup previo si existiera.
     */
    static ApoConfigResult restoreOriginalBackup(
        const std::filesystem::path& configPath,
        std::string* outError = nullptr);

    /**
     * Comprueba si el archivo contiene actualmente una sección administrada de Dash.
     */
    static bool hasManagedSection(const std::filesystem::path& configPath);

    /**
     * Comprueba si existe un archivo de respaldo (.dash_backup).
     */
    static bool hasBackup(const std::filesystem::path& configPath);

    /**
     * Calcula un hash FNV-1a de 64 bits del contenido de un archivo en disco.
     */
    static uint64_t computeContentHash(const std::string& content) noexcept;

private:
    static bool readFullFile(
        const std::filesystem::path& path,
        std::string* outContent,
        std::string* outError);

    static bool createBackupIfNeeded(
        const std::filesystem::path& originalPath,
        const std::string& originalContent,
        std::string* outError);
};

} // namespace dash::audio
