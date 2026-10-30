#include "AudioProfile.h"

namespace dash::profile
{

void AudioProfile::resetToDefaults()
{
    schemaVersion = kCurrentSchemaVersion;
    profileId     = "default-profile";
    name          = "Default Tactical Profile";
    description   = "Perfil balanceado para detección acústica competitiva.";

    game      = ProfileGameInfo {};
    headphone = ProfileHeadphoneInfo {};
    audio     = ProfileAudioSettings {};
    metadata  = ProfileMetadata {};

    dspParams = dash::core::AudioGraphParameters {};
    // Valores recomendados nominales para modo táctico
    dspParams.headroomDb              = -6.0f;
    dspParams.equalizer.footstepsDb   = 1.6f;
    dspParams.equalizer.bassDb        = 1.5f;
    dspParams.equalizer.preampDb      = 0.0f;
    dspParams.transient.centerReductionDb = 4.0f;
    dspParams.ambience.reductionDb    = 6.0f;
    dspParams.dynamics.targetLufs     = -16.0f;
    dspParams.limiter.ceilingDb       = -0.5f;

    spatialParams = dash::spatial::SpatialProcessorParameters {};
    spatialParams.soundstageWidth     = 1.0f;
    spatialParams.apparentDistanceM   = 1.4f;
    spatialParams.frontBackSeparation = 1.0f;
    enableSpatial                     = false;
}

bool AudioProfile::isValid() const noexcept
{
    if (profileId.empty() || name.empty())
        return false;

    if (schemaVersion < 1 || schemaVersion > kCurrentSchemaVersion)
        return false;

    return true;
}

} // namespace dash::profile
