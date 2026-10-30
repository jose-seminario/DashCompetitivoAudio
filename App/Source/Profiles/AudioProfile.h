#pragma once

#include <DashAudioCore/AudioGraphParameters.h>
#include <DashAudioCore/ParametricEQProcessor.h>
#include <DashAudioSpatial/SpatialProcessor.h>

#include <string>
#include <vector>

namespace dash::profile
{

/**
 * Metadatos del videojuego o contexto táctico al que apunta el perfil.
 */
struct ProfileGameInfo
{
    std::string id               = "call-of-duty-warzone";
    std::string name             = "Call of Duty: Warzone";
    std::string supportedVersion = "MW3 / Warzone 2.0+";
    std::string tacticalMode     = "Competitive Footsteps Focus";
};

/**
 * Información de calibración y modelo de auricular.
 */
struct ProfileHeadphoneInfo
{
    std::string manufacturer      = "Generic";
    std::string model             = "Headphones";
    std::string variant           = "Default";
    std::string calibrationStatus = "uncalibrated"; // "verified", "uncalibrated_pending_verified_measurement", "community", "experimental"
    std::string targetCurve       = "Harman Over-Ear 2018";
    std::string measurementSource = "None";
    std::string measurementDate   = "";
};

/**
 * Parámetros de enrutamiento y frecuencia del motor de audio.
 */
struct ProfileAudioSettings
{
    double      sampleRate    = 48000.0;
    std::string channelLayout = "7.1"; // "stereo", "5.1", "7.1", "7.1.4"
    float       preampDb      = -6.0f;
};

/**
 * Metadatos de autoría, licenciamiento y versionado del perfil.
 */
struct ProfileMetadata
{
    std::string author      = "DashAudio";
    std::string license     = "CC-BY-4.0";
    std::string version     = "1.0.0";
    std::string createdAt   = "2026-10-30";
    std::string changelog   = "Initial profile release";
    bool        verified    = false;
    std::vector<std::string> tags;
};

/**
 * Entidad Canónica de Perfil de Audio (AudioProfile).
 *
 * Encapsula la configuración acústica completa de DashCompetitivoAudio:
 *  - Cadenas DSP (Dynamics, Transient, Ambience, 31-Band ISO EQ, 10-Band Parametric EQ, Limiter).
 *  - Motor Espacial (anchura de escenario, distancia aparente, discriminación frente/atrás, HRTF).
 *  - Metadatos de juego, auricular y autoría.
 */
struct AudioProfile
{
    static constexpr int kCurrentSchemaVersion = 1;

    int         schemaVersion = kCurrentSchemaVersion;
    std::string profileId     = "default-profile";
    std::string name          = "Default Tactical Profile";
    std::string description   = "Perfil balanceado para detección acústica competitiva.";

    ProfileGameInfo      game;
    ProfileHeadphoneInfo headphone;
    ProfileAudioSettings audio;
    ProfileMetadata      metadata;

    // 1. Cadena de procesamiento central DSP
    dash::core::AudioGraphParameters dspParams {};

    // 2. Motor espacial 3D inmersivo
    dash::spatial::SpatialProcessorParameters spatialParams {};
    bool enableSpatial = false;

    /** Restablece todos los valores a los valores nominales seguros de fábrica. */
    void resetToDefaults();

    /** Valida la coherencia interna mínima (ID no vacío y versión de esquema compatible). */
    [[nodiscard]] bool isValid() const noexcept;
};

} // namespace dash::profile
