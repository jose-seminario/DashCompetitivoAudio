#include "HeSuViAdapter.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#endif

namespace dash::spatial
{

namespace
{
    std::filesystem::path getApoInstallPath()
    {
        std::filesystem::path result = "C:\\Program Files\\EqualizerAPO";
#ifdef _WIN32
        HKEY hKey = nullptr;
        if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\EqualizerAPO", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            wchar_t pathBuf[MAX_PATH] {};
            DWORD dataSize = sizeof(pathBuf);
            DWORD type = 0;
            if (::RegQueryValueExW(hKey, L"InstallPath", nullptr, &type, reinterpret_cast<LPBYTE>(pathBuf), &dataSize) == ERROR_SUCCESS)
            {
                if (type == REG_SZ && pathBuf[0] != L'\0')
                    result = pathBuf;
            }
            ::RegCloseKey(hKey);
        }
#endif
        return result;
    }
} // namespace

std::filesystem::path HeSuViAdapter::detectHeSuViPath()
{
    std::error_code ec;
    const auto apoPath = getApoInstallPath();
    const auto hesuviDir = apoPath / "config" / "HeSuVi";

    if (std::filesystem::exists(hesuviDir, ec) && std::filesystem::is_directory(hesuviDir, ec))
        return hesuviDir;

    return {};
}

bool HeSuViAdapter::isHeSuViInstalled()
{
    return !detectHeSuViPath().empty();
}

std::vector<HeSuViProfileInfo> HeSuViAdapter::enumerateInstalledProfiles(
    const std::filesystem::path& customHeSuViPath)
{
    std::vector<HeSuViProfileInfo> profiles;
    std::error_code ec;

    auto baseDir = customHeSuViPath;
    if (baseDir.empty())
        baseDir = detectHeSuViPath();

    if (baseDir.empty() || !std::filesystem::exists(baseDir, ec))
        return profiles;

    const auto hrirDir = baseDir / "hrir";
    if (!std::filesystem::exists(hrirDir, ec) || !std::filesystem::is_directory(hrirDir, ec))
        return profiles;

    for (const auto& entry : std::filesystem::directory_iterator(hrirDir, ec))
    {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".wav")
        {
            HeSuViProfileInfo info;
            info.name = entry.path().stem().string();
            info.filePath = entry.path();
            info.isValid = true;
            profiles.push_back(std::move(info));
        }
    }

    std::sort(profiles.begin(), profiles.end(), [](const auto& a, const auto& b) {
        return a.name < b.name;
    });

    return profiles;
}

bool HeSuViAdapter::loadProfileIntoDataset(
    const std::filesystem::path& wavFilePath,
    HRIRDataset& outDataset,
    std::string* outError)
{
    std::error_code ec;
    if (!std::filesystem::exists(wavFilePath, ec))
    {
        if (outError) *outError = "El archivo HRIR WAV no existe: " + wavFilePath.string();
        return false;
    }

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    const juce::File f(wavFilePath.string());
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(f));

    if (reader == nullptr)
    {
        if (outError) *outError = "Fallo al decodificar archivo WAV de HeSuVi: " + wavFilePath.string();
        return false;
    }

    const int numChannels = static_cast<int>(reader->numChannels);
    const int numSamples  = static_cast<int>(reader->lengthInSamples);
    const double sampleRate = reader->sampleRate;

    if (numChannels < 2 || numSamples <= 0 || numSamples > 8192)
    {
        if (outError) *outError = "Formato de canales o duración inválida en WAV: canales=" + std::to_string(numChannels);
        return false;
    }

    juce::AudioBuffer<float> buffer(numChannels, numSamples);
    reader->read(&buffer, 0, numSamples, 0, true, true);

    HRTFProfile profile;
    profile.id          = "hesuvi_" + wavFilePath.stem().string();
    profile.name        = wavFilePath.stem().string();
    profile.author      = "External HeSuVi Profile (User Local)";
    profile.license     = "External User-Installed Dataset";
    profile.sampleRate  = sampleRate;
    profile.irLengthTaps= static_cast<size_t>(numSamples);

    outDataset.setProfile(profile);

    // Mapeo canónico de HeSuVi (14 canales estándar):
    // 0: FL_L,  1: FL_R
    // 2: SL_L,  3: SL_R
    // 4: RL_L,  5: RL_R
    // 6: FC_L,  7: FC_R
    // 8: FR_L,  9: FR_R
    // 10: SR_L, 11: SR_R
    // 12: RR_L, 13: RR_R
    if (numChannels >= 14)
    {
        outDataset.setImpulsePair(0, buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);  // FL
        outDataset.setImpulsePair(1, buffer.getReadPointer(8), buffer.getReadPointer(9), numSamples);  // FR
        outDataset.setImpulsePair(2, buffer.getReadPointer(6), buffer.getReadPointer(7), numSamples);  // FC
        // Canal 3: LFE (impulso unitario atenuado)
        std::vector<float> lfeImp(numSamples, 0.0f);
        lfeImp[0] = 0.7071f;
        outDataset.setImpulsePair(3, lfeImp.data(), lfeImp.data(), numSamples);
        outDataset.setImpulsePair(4, buffer.getReadPointer(4), buffer.getReadPointer(5), numSamples);  // BL (RL)
        outDataset.setImpulsePair(5, buffer.getReadPointer(12), buffer.getReadPointer(13), numSamples);// BR (RR)
        outDataset.setImpulsePair(6, buffer.getReadPointer(2), buffer.getReadPointer(3), numSamples);  // SL
        outDataset.setImpulsePair(7, buffer.getReadPointer(10), buffer.getReadPointer(11), numSamples);// SR
    }
    // Mapeo básico estéreo o multicanal regular
    else
    {
        outDataset.setImpulsePair(0, buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);
        outDataset.setImpulsePair(1, buffer.getReadPointer(1), buffer.getReadPointer(0), numSamples);
    }

    return true;
}

} // namespace dash::spatial
