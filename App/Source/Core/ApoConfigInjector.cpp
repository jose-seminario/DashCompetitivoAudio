#include "ApoConfigInjector.h"
#include <DashAudioCore/EQProcessor.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace dash::audio
{

namespace
{
    std::string formatFixed6(double value)
    {
        std::ostringstream oss;
        oss.imbue(std::locale::classic());
        oss << std::fixed << std::setprecision(6) << value;
        return oss.str();
    }

    std::string formatCompact(double value, int maxDecimals = 6)
    {
        std::ostringstream oss;
        oss.imbue(std::locale::classic());
        oss << std::fixed << std::setprecision(maxDecimals) << value;
        std::string str = oss.str();

        auto dotPos = str.find('.');
        if (dotPos != std::string::npos)
        {
            while (!str.empty() && str.back() == '0')
                str.pop_back();
            if (!str.empty() && str.back() == '.')
                str.pop_back();
        }
        if (str == "-0" || str.empty())
            return "0";
        return str;
    }

#ifdef _WIN32
    std::filesystem::path queryEqualizerApoRegistryDir()
    {
        HKEY hKey = nullptr;
        if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\EqualizerAPO", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
            return {};

        wchar_t buf[MAX_PATH] {};
        DWORD bufBytes = sizeof(buf);
        DWORD type = 0;
        const LONG rc = ::RegQueryValueExW(hKey, L"InstallPath", nullptr, &type, reinterpret_cast<LPBYTE>(buf), &bufBytes);
        ::RegCloseKey(hKey);

        if (rc == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ) && buf[0] != L'\0')
            return std::filesystem::path(buf);

        return {};
    }

    bool writeBytesAndFlushWin32(HANDLE hFile, const std::string& content, std::string* outError)
    {
        const char* dataPtr = content.data();
        DWORD remaining = static_cast<DWORD>(content.size());

        while (remaining > 0)
        {
            DWORD written = 0;
            if (!::WriteFile(hFile, dataPtr, remaining, &written, nullptr))
            {
                if (outError)
                    *outError = "Fallo en WriteFile (Win32 error " + std::to_string(::GetLastError()) + ").";
                return false;
            }
            dataPtr += written;
            remaining -= written;
        }

        if (!::SetEndOfFile(hFile))
        {
            if (outError)
                *outError = "Fallo en SetEndOfFile (Win32 error " + std::to_string(::GetLastError()) + ").";
            return false;
        }

        if (!::FlushFileBuffers(hFile))
        {
            if (outError)
                *outError = "Fallo en FlushFileBuffers (Win32 error " + std::to_string(::GetLastError()) + ").";
            return false;
        }

        return true;
    }

    bool verifyPersistedBytes(const std::filesystem::path& targetPath, const std::string& expectedContent)
    {
        std::ifstream ifs(targetPath, std::ios::binary);
        if (!ifs)
            return false;

        std::string diskBytes((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return diskBytes == expectedContent;
    }
#endif
} // namespace

std::filesystem::path ApoConfigInjector::getDefaultApoConfigPath()
{
    std::error_code ec;
#ifdef _WIN32
    const auto regDir = queryEqualizerApoRegistryDir();
    if (!regDir.empty() && std::filesystem::exists(regDir, ec))
        return regDir / "config" / "config.txt";
#endif

    char* progFiles = nullptr;
    size_t len = 0;
    std::filesystem::path baseDir = "C:\\Program Files";

#ifdef _WIN32
    if (_dupenv_s(&progFiles, &len, "ProgramFiles") == 0 && progFiles != nullptr)
    {
        baseDir = progFiles;
        std::free(progFiles);
    }
#endif

    return baseDir / "EqualizerAPO" / "config" / "config.txt";
}

std::string ApoConfigInjector::resolveHeSuViIncludePath(
    const std::filesystem::path& configFilePath,
    const std::string& customOverride)
{
    if (!customOverride.empty())
        return customOverride;

    std::error_code ec;
    const std::filesystem::path effectiveConfig = configFilePath.empty()
        ? getDefaultApoConfigPath()
        : configFilePath;

    const auto configDir = effectiveConfig.parent_path();

    // 1. Ruta estándar relativa a <EqualizerAPO>\config\: "HeSuVi\hesuvi.txt"
    if (!configDir.empty() && std::filesystem::exists(configDir / "HeSuVi" / "hesuvi.txt", ec))
        return kDefaultHeSuViInclude;

    // 2. Búsqueda dinámica en %LOCALAPPDATA%\DashCompetitivoAudio\HeSuVi\hesuvi.txt (fallback empaquetado)
#ifdef _WIN32
    char* localAppData = nullptr;
    size_t len = 0;
    if (_dupenv_s(&localAppData, &len, "LOCALAPPDATA") == 0 && localAppData != nullptr)
    {
        const std::filesystem::path localHeSuVi =
            std::filesystem::path(localAppData) / "DashCompetitivoAudio" / "HeSuVi" / "hesuvi.txt";
        std::free(localAppData);

        if (std::filesystem::exists(localHeSuVi, ec))
            return localHeSuVi.string();
    }

    const auto regDir = queryEqualizerApoRegistryDir();
    if (!regDir.empty())
    {
        const auto regHeSuVi = regDir / "config" / "HeSuVi" / "hesuvi.txt";
        if (std::filesystem::exists(regHeSuVi, ec))
        {
            if (regDir / "config" == configDir)
                return kDefaultHeSuViInclude;
            return regHeSuVi.string();
        }
    }
#endif

    // Por defecto emite siempre la directiva estándar "Include: HeSuVi\hesuvi.txt"
    return kDefaultHeSuViInclude;
}

std::string ApoConfigInjector::buildGraphicEqDirective(double footstepsDb, double bassDb)
{
    if (std::abs(footstepsDb - 1.6) < 1.0e-4 && std::abs(bassDb - 1.5) < 1.0e-4)
        return kTacticalGraphicEq;

    const auto gains = dash::core::EQProcessor::computeBandGainsDb(footstepsDb, bassDb);

    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "GraphicEQ: ";

    for (size_t i = 0; i < dash::core::EQProcessor::kNumBands; ++i)
    {
        if (i > 0)
            out << "; ";
        out << dash::core::EQProcessor::kIsoFrequencyLabels[i] << " " << formatCompact(gains[i], 2);
    }

    return out.str();
}

std::string ApoConfigInjector::buildConfigContent(
    const ApoChainConfig& config,
    const std::filesystem::path& targetConfigFilePath)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());

    out << "# DASH_COMPETITIVO_AUDIO_GRAPH: v1.0 (" << config.profileName << ")\r\n";

    if (!config.targetEndpointGuid.empty())
    {
        out << "Device: " << config.targetEndpointGuid << "\r\n";
    }

    // 1. Ganancia Previa (Headroom anti-clipping)
    out << "Preamp: " << formatCompact(config.preampDb) << " dB\r\n";

    // 2. Directiva del Supresor de Disparos (Canal Central / Gun Crack)
    if (config.enableTransientSuppressor)
    {
        out << "VSTPlugin: Library TransientSuppressor.vst3"
            << " Gun " << formatFixed6(config.gunVolume)
            << " Steps " << config.gunSteps
            << " Front " << config.gunFrontOnly
            << " Restore " << config.gunRestore
            << " Bypass " << config.gunBypass
            << "\r\n";
    }

    // 3. Directiva del Compresor Dinámico Híbrido (Sonoridad BS.1770)
    if (config.enableLoudnessCompressor)
    {
        out << "VSTPlugin: Library LoudnessCompressor.vst3"
            << " Amount " << formatFixed6(config.compAmount)
            << " Target " << formatFixed6(config.compTarget)
            << " Release " << formatCompact(config.compRelease)
            << " DetRel " << formatFixed6(config.compDetRel)
            << " LimRel " << formatFixed6(config.compLimRel)
            << " SCLow " << formatCompact(config.compScLow)
            << " SCHigh " << formatCompact(config.compScHigh)
            << "\r\n";
    }

    // 4. Directiva del Protector Ambiental Multibanda
    if (config.enableAmbienceGuard)
    {
        out << "VSTPlugin: Library AmbienceGuard.vst3"
            << " AmbGuar " << formatCompact(config.ambGuard)
            << " Distant " << config.ambDistant
            << " Target " << formatFixed6(config.ambTarget)
            << " Release " << formatCompact(config.ambRelease)
            << " DetRel " << formatFixed6(config.ambDetRel)
            << " SCLow " << formatCompact(config.ambScLow)
            << " SCHigh " << formatCompact(config.ambScHigh)
            << " BandMod " << config.ambBandMod
            << "\r\n";
    }

    // 5. Ecualización Gráfica Acústica de 31 Bandas
    if (config.enableGraphicEq)
    {
        out << buildGraphicEqDirective(config.footstepsEqDb, config.bassEqDb) << "\r\n";
    }

    // 6. Virtualización Binaural Convolutiva HRIR (7.1 -> Estéreo vía HeSuVi)
    if (config.enableHeSuVi)
    {
        const std::string hesuviInclude = resolveHeSuViIncludePath(targetConfigFilePath, config.hesuviIncludePath);
        out << "Include: " << hesuviInclude << "\r\n";
    }

    return out.str();
}

bool ApoConfigInjector::writeConfigAtomically(
    const std::filesystem::path& configFilePath,
    const ApoChainConfig& config,
    std::string* outError,
    bool* outPermissionDenied)
{
    const std::string payload = buildConfigContent(config, configFilePath);
    return writeRawPayloadAtomically(configFilePath, payload, outError, outPermissionDenied);
}

bool ApoConfigInjector::writeEmptyConfigAtomically(
    const std::filesystem::path& configFilePath,
    std::string* outError,
    bool* outPermissionDenied)
{
    return writeRawPayloadAtomically(configFilePath, "", outError, outPermissionDenied);
}

bool ApoConfigInjector::writeRawPayloadAtomically(
    const std::filesystem::path& configFilePath,
    const std::string& payload,
    std::string* outError,
    bool* outPermissionDenied)
{
    if (outPermissionDenied)
        *outPermissionDenied = false;

    std::error_code ec;
    const auto parentDir = configFilePath.parent_path();
    if (!parentDir.empty() && !std::filesystem::exists(parentDir, ec))
    {
        if (!std::filesystem::create_directories(parentDir, ec))
        {
            if (ec == std::errc::permission_denied && outPermissionDenied)
                *outPermissionDenied = true;

            if (outError)
                *outError = "No se pudo crear el directorio destino '" + parentDir.string() + "': " + ec.message();
            return false;
        }
    }

#ifdef _WIN32
    const std::wstring targetWide = configFilePath.wstring();
    const std::wstring tempWide = targetWide + L".dash.tmp."
        + std::to_wstring(::GetCurrentProcessId()) + L"."
        + std::to_wstring(::GetTickCount64());

    HANDLE hTemp = ::CreateFileW(
        tempWide.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        nullptr);

    if (hTemp != INVALID_HANDLE_VALUE)
    {
        const bool tempOk = writeBytesAndFlushWin32(hTemp, payload, outError);
        ::CloseHandle(hTemp);

        if (!tempOk)
        {
            ::DeleteFileW(tempWide.c_str());
            return false;
        }

        constexpr int kMaxRenameAttempts = 4;
        for (int attempt = 1; attempt <= kMaxRenameAttempts; ++attempt)
        {
            if (::MoveFileExW(tempWide.c_str(), targetWide.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            {
                return true;
            }

            if (attempt < kMaxRenameAttempts)
                ::Sleep(15 * attempt);
        }

        ::DeleteFileW(tempWide.c_str());
    }
    else if (::GetLastError() == ERROR_ACCESS_DENIED && outPermissionDenied)
    {
        *outPermissionDenied = true;
    }

    HANDLE hDirect = ::CreateFileW(
        targetWide.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hDirect == INVALID_HANDLE_VALUE)
    {
        const DWORD err = ::GetLastError();
        if (err == ERROR_ACCESS_DENIED && outPermissionDenied)
            *outPermissionDenied = true;

        if (outError)
        {
            *outError = "Acceso denegado al escribir '" + configFilePath.string()
                      + "' (Win32 error " + std::to_string(err)
                      + "). Reinicia Dash Competitivo Audio como Administrador.";
        }
        return false;
    }

    const bool directOk = writeBytesAndFlushWin32(hDirect, payload, outError);
    ::CloseHandle(hDirect);

    if (!directOk)
        return false;

    if (!verifyPersistedBytes(configFilePath, payload))
    {
        if (outError)
            *outError = "Fallo de verificación de integridad tras la escritura en '" + configFilePath.string() + "'.";
        return false;
    }

    return true;
#else
    const auto tempPath = configFilePath.string() + ".dash.tmp";
    {
        std::ofstream ofs(tempPath, std::ios::binary | std::ios::trunc);
        if (!ofs)
        {
            if (outError) *outError = "No se pudo abrir archivo temporal: " + tempPath;
            return false;
        }
        ofs.write(payload.data(), static_cast<std::streamsize>(payload.size()));
        ofs.flush();
    }
    std::filesystem::rename(tempPath, configFilePath, ec);
    if (ec)
    {
        if (outError) *outError = "Fallo al renombrar atómicamente: " + ec.message();
        return false;
    }
    return true;
#endif
}

} // namespace dash::audio
