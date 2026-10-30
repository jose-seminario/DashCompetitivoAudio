#include "ApoConfigInjector.h"

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

std::string ApoConfigInjector::buildGraphicEqDirective(double footstepsDb, double bassDb)
{
    // Si está en los valores exactos de la auditoría Campaign (Pasos +1.6 dB, Graves +1.5 dB),
    // devolvemos literalmente la constante original de 31 bandas sin alteración alguna.
    if (std::abs(footstepsDb - 1.6) < 1.0e-4 && std::abs(bassDb - 1.5) < 1.0e-4)
        return kTacticalGraphicEq;

    struct EqBand { const char* freq; double gain; };
    std::array<EqBand, 31> bands {{
        { "20", -3.0 },   { "25", -2.0 },   { "31.5", -0.8 }, { "40", 0.5 },
        { "50", 1.5 },    { "63", 2.2 },    { "80", 2.0 },    { "100", 1.5 },
        { "125", 1.2 },   { "160", 0.8 },   { "200", 0.2 },   { "250", -0.6 },
        { "315", -1.4 },  { "400", -1.8 },  { "500", -1.5 },  { "630", -1.0 },
        { "800", -0.5 },  { "1000", 0.0 },  { "1250", 0.3 },  { "1600", 0.6 },
        { "2000", 1.0 },  { "2500", 1.4 },  { "3150", 1.6 },  { "4000", 1.2 },
        { "5000", 0.8 },  { "6300", 2.2 },  { "8000", 1.8 },  { "10000", 0.7 },
        { "12500", 0.2 }, { "16000", -0.5 },{ "20000", -1.2 }
    }};

    const double deltaBass  = bassDb - 1.5;      // Control sobre 100-250 Hz (y hombro suave en 80 Hz)
    const double deltaSteps = footstepsDb - 1.6; // Control sobre 3.15 kHz (+1.6) y 6.3 kHz (+2.2)

    // Banda Graves: 80 Hz (idx 6) hombro 50%, 100..250 Hz (idx 7..11) 100%
    bands[6].gain  += deltaBass * 0.5;
    bands[7].gain  += deltaBass; // 100 Hz
    bands[8].gain  += deltaBass; // 125 Hz
    bands[9].gain  += deltaBass; // 160 Hz
    bands[10].gain += deltaBass; // 200 Hz
    bands[11].gain += deltaBass; // 250 Hz

    // Banda Pasos (Warzone Footsteps): 3150 Hz (idx 22) y 6300 Hz (idx 25) + hombros acústicos suaves
    bands[21].gain += deltaSteps * 0.50; // 2500 Hz
    bands[22].gain  = footstepsDb;       // 3150 Hz (base +1.6 dB)
    bands[23].gain += deltaSteps * 0.45; // 4000 Hz
    bands[25].gain  = 2.2 + deltaSteps * (2.2 / 1.6); // 6300 Hz (base +2.2 dB escalada proporcionalmente)
    bands[26].gain += deltaSteps * 0.55; // 8000 Hz

    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "GraphicEQ: ";

    for (size_t i = 0; i < bands.size(); ++i)
    {
        if (i > 0)
            out << "; ";
        out << bands[i].freq << " " << formatCompact(bands[i].gain, 2);
    }

    return out.str();
}

std::string ApoConfigInjector::buildConfigContent(const ApoChainConfig& config)
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

    // 5. Ecualización Gráfica Acústica de 31 Bandas (con ajuste dinámico de Pasos y Graves)
    if (config.enableGraphicEq)
    {
        out << buildGraphicEqDirective(config.footstepsEqDb, config.bassEqDb) << "\r\n";
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
    std::string* outError,
    bool* outPermissionDenied)
{
    return writeRawPayloadAtomically(configFilePath, buildConfigContent(config), outError, outPermissionDenied);
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

        ::DeleteFileW(tempWide.c_str());
    }
    else if (::GetLastError() == ERROR_ACCESS_DENIED && outPermissionDenied)
    {
        *outPermissionDenied = true;
    }

    // Paso 3: Fallback seguro en sitio cuando EqualizerAPO.dll mantiene abierto config.txt
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
