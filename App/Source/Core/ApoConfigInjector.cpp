#include "ApoConfigInjector.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

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

    std::string formatCompact(double value)
    {
        std::ostringstream oss;
        oss.imbue(std::locale::classic());
        oss << std::fixed << std::setprecision(6) << value;
        std::string str = oss.str();

        // Recortar ceros finales innecesarios preservando valores enteros limpios ("0", "1", "0.5", "0.2269")
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

std::string ApoConfigInjector::buildConfigContent(const ApoChainConfig& config)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());

    out << "# DASH_COMPETITIVO_AUDIO_GRAPH: v1.0\r\n";

    if (!config.targetEndpointGuid.empty())
    {
        out << "Device: " << config.targetEndpointGuid << "\r\n";
    }

    // 1. Ganancia Previa (Headroom obligatorio antes/después del procesamiento lineal)
    out << "Preamp: " << formatCompact(config.preampDb) << " dB\r\n";

    // 2. Directiva del Supresor de Disparos (Canal Central / Transitorios)
    if (config.enableTransientSuppressor)
    {
        out << "VSTPlugin: Library TransientSuppressor.dll"
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
        out << "VSTPlugin: Library LoudnessCompressor.dll"
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
        out << "VSTPlugin: Library AmbienceGuard.dll"
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
        out << kTacticalGraphicEq << "\r\n";
    }

    // 6. Virtualización Binaural Convolutiva HRIR (7.1 -> Estéreo)
    if (config.enableHeSuVi)
    {
        out << "Include: HeSuVi\\hesuvi.txt\r\n";
    }

    return out.str();
}

bool ApoConfigInjector::writeConfigAtomically(
    const std::filesystem::path& configFilePath,
    const ApoChainConfig& config,
    std::string* outError)
{
    const std::string payload = buildConfigContent(config);

    std::error_code ec;
    const auto parentDir = configFilePath.parent_path();
    if (!parentDir.empty() && !std::filesystem::exists(parentDir, ec))
    {
        if (!std::filesystem::create_directories(parentDir, ec))
        {
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

    // Paso 1: Escribir y vaciar a disco en archivo temporal adyacente
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

        // Paso 2: Reemplazo atómico en el sistema de archivos NTFS
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

        // Limpiar el temporal antes del fallback en sitio
        ::DeleteFileW(tempWide.c_str());
    }

    // Paso 3: Fallback seguro en sitio cuando EqualizerAPO.dll mantiene abierto config.txt
    // con FILE_SHARE_WRITE pero sin FILE_SHARE_DELETE (rechaza MoveFileExW pero admite truncado/escritura).
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
        if (outError)
        {
            *outError = "Acceso denegado al escribir '" + configFilePath.string()
                      + "' (Win32 error " + std::to_string(err)
                      + "). Ejecuta DashCompetitivoAudio como Administrador si la ruta está en Program Files.";
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
        if (!ofs)
        {
            if (outError) *outError = "Error al volcar bytes en archivo temporal.";
            return false;
        }
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
