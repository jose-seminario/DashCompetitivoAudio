#pragma once

#include "AudioProfile.h"

#include <filesystem>
#include <string>

namespace dash::profile
{

/**
 * Serializador y Deserializador JSON Robusto y Seguro (ProfileSerializer).
 *
 * Utiliza el parser JSON nativo de JUCE (juce::JSON, juce::var) para garantizar
 * que la lectura y escritura no dependan de bibliotecas externas y se ejecuten
 * de manera estricta y protegida contra desbordamientos.
 */
class ProfileSerializer
{
public:
    /**
     * Serializa un objeto AudioProfile a una cadena JSON formateada.
     */
    static std::string serializeToJson(const AudioProfile& profile, bool prettyPrint = true);

    /**
     * Deserializa una cadena JSON en un objeto AudioProfile.
     * Realiza validación de seguridad previa mediante ProfileValidator.
     */
    static bool deserializeFromJson(const std::string& jsonText, AudioProfile& outProfile, std::string* outError = nullptr);

    /**
     * Guarda el perfil de forma segura en un archivo en disco (usando escritura atómica temporal).
     */
    static bool saveToFile(const AudioProfile& profile, const std::filesystem::path& filePath, std::string* outError = nullptr);

    /**
     * Carga y deserializa un archivo de perfil desde el disco.
     */
    static bool loadFromFile(const std::filesystem::path& filePath, AudioProfile& outProfile, std::string* outError = nullptr);
};

} // namespace dash::profile
