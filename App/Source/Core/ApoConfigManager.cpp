#include "ApoConfigManager.h"
#include "ApoConfigInjector.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <fstream>
#include <sstream>

namespace dash::audio
{

namespace
{
    bool hasApoBinariesInDir(const std::filesystem::path& installDir) noexcept
    {
        if (installDir.empty())
            return false;

        std::error_code ec;
        if (!std::filesystem::exists(installDir, ec))
            return false;

        return std::filesystem::exists(installDir / "EqualizerAPO.dll", ec)
            || std::filesystem::exists(installDir / "Configurator.exe", ec)
            || std::filesystem::exists(installDir / "DeviceSelector.exe", ec)
            || std::filesystem::exists(installDir / "Editor.exe", ec);
    }

    std::string stripManagedBlockFromContent(const std::string& content)
    {
        const size_t beginPos = content.find(ApoConfigManager::kBeginMarker);
        const size_t endPos   = content.find(ApoConfigManager::kEndMarker);

        if (beginPos == std::string::npos || endPos == std::string::npos || endPos <= beginPos)
            return content;

        size_t cutStart = beginPos;
        if (cutStart >= 2 && content[cutStart - 2] == '\r' && content[cutStart - 1] == '\n')
            cutStart -= 2;
        else if (cutStart >= 1 && content[cutStart - 1] == '\n')
            cutStart -= 1;

        const size_t afterEnd = endPos + std::string(ApoConfigManager::kEndMarker).length();
        size_t nextValid = afterEnd;
        if (nextValid < content.size() && content[nextValid] == '\r') ++nextValid;
        if (nextValid < content.size() && content[nextValid] == '\n') ++nextValid;

        return content.substr(0, cutStart) + content.substr(nextValid);
    }
} // namespace

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

    const auto parentDir = originalPath.parent_path();
    if (!parentDir.empty() && !std::filesystem::exists(parentDir, ec))
    {
        std::filesystem::create_directories(parentDir, ec);
    }

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

bool ApoConfigManager::isApoInstalled()
{
    const auto apoConfigPath = ApoConfigInjector::getDefaultApoConfigPath();
    const auto installDir = apoConfigPath.parent_path().parent_path();
    if (hasApoBinariesInDir(installDir))
        return true;

    if (hasApoBinariesInDir("C:\\Program Files\\EqualizerAPO"))
        return true;

    return false;
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

        if (!hasBackup(configPath))
        {
            const std::string cleanOriginal = stripManagedBlockFromContent(existingContent);
            if (!createBackupIfNeeded(configPath, cleanOriginal, outError))
                return ApoConfigResult::BackupFailed;
        }
    }
    else
    {
        if (!hasBackup(configPath))
        {
            if (!createBackupIfNeeded(configPath, "", outError))
                return ApoConfigResult::BackupFailed;
        }
    }

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
        const size_t afterEnd = endPos + std::string(kEndMarker).length();
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
        newContent = existingContent;
        if (!newContent.empty() && newContent.back() != '\n')
            newContent += "\r\n";
        newContent += managedBlock;
    }

    const std::filesystem::path tempPath = configPath.string() + ".dash.tmp";
    std::ofstream ofs(tempPath, std::ios::binary | std::ios::trunc);
    if (!ofs)
    {
        if (outError) *outError = "Permiso denegado al escribir en carpeta de Equalizer APO.";
        return ApoConfigResult::PermissionDenied;
    }
    ofs.write(newContent.data(), static_cast<std::streamsize>(newContent.size()));
    ofs.flush();
    ofs.close();

#ifdef _WIN32
    if (!MoveFileExW(tempPath.c_str(), configPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        const DWORD err = GetLastError();
        std::error_code ecRemove;
        std::filesystem::remove(tempPath, ecRemove);
        if (err == ERROR_ACCESS_DENIED)
        {
            if (outError) *outError = "Permiso denegado al reemplazar atómicamente config.txt.";
            return ApoConfigResult::PermissionDenied;
        }
        if (outError) *outError = "Fallo al reemplazar atómicamente config.txt (Código Win32: " + std::to_string(err) + ")";
        return ApoConfigResult::WriteFailed;
    }
#else
    std::filesystem::rename(tempPath, configPath, ec);
    if (ec)
    {
        if (outError) *outError = "Fallo al renombrar atómicamente: " + ec.message();
        return ApoConfigResult::WriteFailed;
    }
#endif

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
        return ApoConfigResult::Success;
    }

    const size_t afterEnd = endPos + std::string(kEndMarker).length();
    size_t nextValid = afterEnd;
    if (nextValid < existingContent.size() && existingContent[nextValid] == '\r') ++nextValid;
    if (nextValid < existingContent.size() && existingContent[nextValid] == '\n') ++nextValid;

    std::string restoredContent = existingContent.substr(0, beginPos)
                                + existingContent.substr(nextValid);

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

#ifdef _WIN32
    if (!MoveFileExW(tempPath.c_str(), configPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        const DWORD err = GetLastError();
        std::error_code ecRemove;
        std::filesystem::remove(tempPath, ecRemove);
        if (err == ERROR_ACCESS_DENIED)
        {
            if (outError) *outError = "Permiso denegado al remover sección de config.txt.";
            return ApoConfigResult::PermissionDenied;
        }
        if (outError) *outError = "Fallo al remover sección de config.txt (Código Win32: " + std::to_string(err) + ")";
        return ApoConfigResult::WriteFailed;
    }
#else
    std::error_code ec;
    std::filesystem::rename(tempPath, configPath, ec);
    if (ec)
    {
        if (outError) *outError = "Fallo al renombrar atómicamente: " + ec.message();
        return ApoConfigResult::WriteFailed;
    }
#endif

    return ApoConfigResult::Success;
}

ApoConfigResult ApoConfigManager::restoreOriginalConfig(
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

    std::string backupContent;
    if (!readFullFile(backupPath, &backupContent, outError))
        return ApoConfigResult::PermissionDenied;

    const std::filesystem::path tempPath = configPath.string() + ".dash.restore.tmp";
    {
        std::ofstream ofs(tempPath, std::ios::binary | std::ios::trunc);
        if (!ofs)
        {
            if (outError) *outError = "Permiso denegado al preparar la restauración de config.txt.";
            return ApoConfigResult::PermissionDenied;
        }
        ofs.write(backupContent.data(), static_cast<std::streamsize>(backupContent.size()));
        ofs.flush();
    }

#ifdef _WIN32
    DWORD lastErr = 0;
    bool moved = false;
    for (int attempt = 1; attempt <= 3; ++attempt)
    {
        if (::MoveFileExW(tempPath.c_str(), configPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            moved = true;
            break;
        }
        lastErr = ::GetLastError();
        if (attempt < 3)
            ::Sleep(static_cast<DWORD>(15 * attempt));
    }

    if (!moved)
    {
        std::error_code ecRemove;
        std::filesystem::remove(tempPath, ecRemove);

        if (lastErr == ERROR_ACCESS_DENIED)
        {
            if (outError) *outError = "Permiso denegado al restaurar config.txt desde config.txt.bak.";
            return ApoConfigResult::PermissionDenied;
        }

        if (outError) *outError = "Fallo al restaurar config.txt (Código Win32: " + std::to_string(lastErr) + ").";
        return ApoConfigResult::WriteFailed;
    }
#else
    std::filesystem::rename(tempPath, configPath, ec);
    if (ec)
    {
        std::error_code ecRemove;
        std::filesystem::remove(tempPath, ecRemove);
        if (outError) *outError = "Fallo al restaurar config.txt: " + ec.message();
        return ApoConfigResult::WriteFailed;
    }
#endif

    return ApoConfigResult::Success;
}

ApoConfigResult ApoConfigManager::restoreOriginalConfig(std::string* outError)
{
    return restoreOriginalConfig(ApoConfigInjector::getDefaultApoConfigPath(), outError);
}

ApoConfigResult ApoConfigManager::restoreOriginalBackup(
    const std::filesystem::path& configPath,
    std::string* outError)
{
    return restoreOriginalConfig(configPath, outError);
}

} // namespace dash::audio
