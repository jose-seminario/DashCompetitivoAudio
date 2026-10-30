#include "ApoConfigManager.h"
#include "ApoConfigInjector.h"

#include <fstream>
#include <sstream>

namespace dash::audio
{

uint64_t ApoConfigManager::computeContentHash(const std::string& content) noexcept
{
    // Algoritmo FNV-1a de 64 bits para detección rápida y determinista de cambios
    constexpr uint64_t kFnvPrime  = 0x100000001b3ULL;
    constexpr uint64_t kFnvOffset = 0xcbf29ce484222325ULL;

    uint64_t hash = kFnvOffset;
    for (unsigned char c : content)
    {
        hash ^= static_cast<uint64_t>(c);
        hash *= kFnvPrime;
    }
    return hash;
}

bool ApoConfigManager::readFullFile(
    const std::filesystem::path& path,
    std::string* outContent,
    std::string* outError)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
    {
        if (outError) *outError = "Archivo no encontrado: " + path.string();
        return false;
    }

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
    {
        if (outError) *outError = "No se pudo abrir el archivo para lectura: " + path.string();
        return false;
    }

    if (outContent)
    {
        outContent->assign((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
    }
    return true;
}

bool ApoConfigManager::createBackupIfNeeded(
    const std::filesystem::path& originalPath,
    const std::string& originalContent,
    std::string* outError)
{
    const std::filesystem::path backupPath = originalPath.string() + kBackupExt;
    std::error_code ec;

    // Solo creamos el backup original si no existe ya uno previo
    if (!std::filesystem::exists(backupPath, ec))
    {
        std::ofstream ofs(backupPath, std::ios::binary | std::ios::trunc);
        if (!ofs)
        {
            if (outError) *outError = "Fallo al crear backup previo en: " + backupPath.string();
            return false;
        }
        ofs.write(originalContent.data(), static_cast<std::streamsize>(originalContent.size()));
        ofs.flush();
    }
    return true;
}

bool ApoConfigManager::hasManagedSection(const std::filesystem::path& configPath)
{
    std::string content;
    if (!readFullFile(configPath, &content, nullptr))
        return false;

    return content.find(kBeginMarker) != std::string::npos
        && content.find(kEndMarker) != std::string::npos;
}

bool ApoConfigManager::hasBackup(const std::filesystem::path& configPath)
{
    std::error_code ec;
    return std::filesystem::exists(configPath.string() + kBackupExt, ec);
}

ApoConfigResult ApoConfigManager::injectManagedSection(
    const std::filesystem::path& configPath,
    const std::string& managedDirectives,
    std::string* outError)
{
    std::string existingContent;
    std::error_code ec;

    if (std::filesystem::exists(configPath, ec))
    {
        if (!readFullFile(configPath, &existingContent, outError))
            return ApoConfigResult::PermissionDenied;

        // Crear backup del estado previo no administrado
        if (existingContent.find(kBeginMarker) == std::string::npos)
        {
            if (!createBackupIfNeeded(configPath, existingContent, outError))
                return ApoConfigResult::BackupFailed;
        }
    }

    // Construcción del bloque gestionado con delimitadores explícitos
    std::ostringstream blockStream;
    blockStream << "\r\n" << kBeginMarker << "\r\n"
                << managedDirectives;
    if (!managedDirectives.empty() && managedDirectives.back() != '\n')
        blockStream << "\r\n";
    blockStream << kEndMarker << "\r\n";

    const std::string managedBlock = blockStream.str();
    std::string newContent;

    const size_t beginPos = existingContent.find(kBeginMarker);
    const size_t endPos   = existingContent.find(kEndMarker);

    if (beginPos != std::string::npos && endPos != std::string::npos && endPos > beginPos)
    {
        // Reemplazar de forma quirúrgica únicamente la sección entre los marcadores
        const size_t afterEnd = endPos + std::string(kEndMarker).length();
        // Consumir CRLF tras el marcador final si existiera
        size_t nextValid = afterEnd;
        if (nextValid < existingContent.size() && existingContent[nextValid] == '\r') ++nextValid;
        if (nextValid < existingContent.size() && existingContent[nextValid] == '\n') ++nextValid;

        newContent = existingContent.substr(0, beginPos)
                   + kBeginMarker + "\r\n"
                   + managedDirectives;
        if (!managedDirectives.empty() && managedDirectives.back() != '\n')
            newContent += "\r\n";
        newContent += std::string(kEndMarker) + "\r\n"
                   + existingContent.substr(nextValid);
    }
    else
    {
        // Anexar al final del archivo conservando intactos todos los filtros del usuario
        newContent = existingContent;
        if (!newContent.empty() && newContent.back() != '\n')
            newContent += "\r\n";
        newContent += managedBlock;
    }

    bool permDenied = false;
    std::string writeErr;
    if (!ApoConfigInjector::writeConfigAtomically(configPath, ApoChainConfig{}, &writeErr, &permDenied))
    {
        // En caso de fallo con ApoConfigInjector, reintentamos escribir el payload procesado con su método directo
    }

    // Usamos el escritor atómico con el payload final preservado
    std::ofstream ofs(configPath.string() + ".dash.tmp", std::ios::binary | std::ios::trunc);
    if (!ofs)
    {
        if (outError) *outError = "Permiso denegado al escribir en carpeta de Equalizer APO.";
        return ApoConfigResult::PermissionDenied;
    }
    ofs.write(newContent.data(), static_cast<std::streamsize>(newContent.size()));
    ofs.flush();
    ofs.close();

    std::filesystem::rename(configPath.string() + ".dash.tmp", configPath, ec);
    if (ec)
    {
        if (outError) *outError = "Fallo al renombrar atómicamente: " + ec.message();
        return ApoConfigResult::WriteFailed;
    }

    return ApoConfigResult::Success;
}

ApoConfigResult ApoConfigManager::removeManagedSection(
    const std::filesystem::path& configPath,
    std::string* outError)
{
    std::string existingContent;
    if (!readFullFile(configPath, &existingContent, outError))
        return ApoConfigResult::FileNotFound;

    const size_t beginPos = existingContent.find(kBeginMarker);
    const size_t endPos   = existingContent.find(kEndMarker);

    if (beginPos == std::string::npos || endPos == std::string::npos || endPos <= beginPos)
    {
        // No hay sección administrada; no se requiere acción
        return ApoConfigResult::Success;
    }

    const size_t afterEnd = endPos + std::string(kEndMarker).length();
    size_t nextValid = afterEnd;
    if (nextValid < existingContent.size() && existingContent[nextValid] == '\r') ++nextValid;
    if (nextValid < existingContent.size() && existingContent[nextValid] == '\n') ++nextValid;

    // Restaurar el contenido antes y después del bloque gestionado
    std::string restoredContent = existingContent.substr(0, beginPos)
                                + existingContent.substr(nextValid);

    std::error_code ec;
    const std::filesystem::path tempPath = configPath.string() + ".dash.tmp";
    std::ofstream ofs(tempPath, std::ios::binary | std::ios::trunc);
    if (!ofs)
    {
        if (outError) *outError = "Permiso denegado al remover sección de config.txt.";
        return ApoConfigResult::PermissionDenied;
    }
    ofs.write(restoredContent.data(), static_cast<std::streamsize>(restoredContent.size()));
    ofs.flush();
    ofs.close();

    std::filesystem::rename(tempPath, configPath, ec);
    if (ec)
    {
        if (outError) *outError = "Fallo al renombrar atómicamente: " + ec.message();
        return ApoConfigResult::WriteFailed;
    }

    return ApoConfigResult::Success;
}

ApoConfigResult ApoConfigManager::restoreOriginalBackup(
    const std::filesystem::path& configPath,
    std::string* outError)
{
    const std::filesystem::path backupPath = configPath.string() + kBackupExt;
    std::error_code ec;

    if (!std::filesystem::exists(backupPath, ec))
    {
        if (outError) *outError = "No existe archivo de respaldo previo en: " + backupPath.string();
        return ApoConfigResult::FileNotFound;
    }

    std::filesystem::copy_file(
        backupPath,
        configPath,
        std::filesystem::copy_options::overwrite_existing,
        ec);

    if (ec)
    {
        if (outError) *outError = "Fallo al restaurar backup: " + ec.message();
        return ApoConfigResult::WriteFailed;
    }

    return ApoConfigResult::Success;
}

} // namespace dash::audio
