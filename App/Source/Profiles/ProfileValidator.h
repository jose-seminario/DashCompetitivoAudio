#pragma once

#include "AudioProfile.h"

#include <string>
#include <vector>

namespace dash::profile
{

/**
 * Reporte de un problema individual de validación.
 */
struct ValidationIssue
{
    enum class Severity { Warning, Error };

    Severity    severity = Severity::Error;
    std::string property;
    std::string message;
};

/**
 * Resultado completo del proceso de validación.
 */
struct ProfileValidationResult
{
    bool valid = true;
    std::vector<ValidationIssue> issues;

    void addError(const std::string& prop, const std::string& msg);
    void addWarning(const std::string& prop, const std::string& msg);

    [[nodiscard]] bool hasErrors() const noexcept { return !valid; }
    [[nodiscard]] std::string formatSummary() const;
};

/**
 * Validador de Seguridad y Rango Acústico de Perfiles (ProfileValidator).
 *
 * Implementa las restricciones de seguridad:
 *  - Cero código ejecutable (prohíbe tokens EXE, DLL, BAT, PS1, PowerShell, CMD).
 *  - Límite estricto de tamaño de archivo (1 MB máximo).
 *  - Prevención de Path Traversal en profileId (prohíbe '..', '/', '\', carácteres de escape).
 *  - Rango físico y matemático en todos los parámetros DSP y Espaciales.
 */
class ProfileValidator
{
public:
    static constexpr size_t kMaxJsonSizeBytes = 1024 * 1024; // 1 MB
    static constexpr size_t kMaxIdLength      = 64;
    static constexpr size_t kMaxNameLength    = 128;
    static constexpr size_t kMaxTextLength    = 2048;

    /**
     * Valida el texto crudo del archivo JSON antes de cualquier procesamiento o deserialización.
     */
    static ProfileValidationResult validateRawJsonText(const std::string& rawJson);

    /**
     * Valida la estructura deserializada en memoria.
     */
    static ProfileValidationResult validateProfile(const AudioProfile& profile);

    /**
     * Valida que un identificador de perfil sea un slug seguro (alfanumérico, guiones y guiones bajos).
     */
    static bool isSafeProfileId(const std::string& profileId) noexcept;
};

} // namespace dash::profile
