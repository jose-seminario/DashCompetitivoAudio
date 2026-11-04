#pragma once

#include <cstddef>
#include <cstdint>

namespace dash::core
{

/**
 * Capacidad de canales del motor DashAudioCore: Estéreo nativo puro (2 canales: L / R).
 */
inline constexpr std::size_t kMaxSupportedChannels = 2;

/**
 * Rango extremo permitido para el Preamp y las 6 bandas del ecualizador (-18.0 dB a +18.0 dB).
 */
inline constexpr float kMinEqGainDb = -18.0f;
inline constexpr float kMaxEqGainDb = 18.0f;

/**
 * Techo nominal del limitador estéreo de pico (-0.5 dBFS).
 */
inline constexpr float kLimiterCeilingDb = -0.5f;
inline constexpr float kDefaultCeilingDb = -0.5f;

/**
 * Frecuencias de muestreo nominales soportadas por el motor.
 */
inline constexpr double kSampleRate44100  = 44100.0;
inline constexpr double kSampleRate48000  = 48000.0;
inline constexpr double kSampleRate88200  = 88200.0;
inline constexpr double kSampleRate96000  = 96000.0;
inline constexpr double kSampleRate192000 = 192000.0;

namespace SpeakerMask
{
    inline constexpr uint32_t kFrontLeft  = 0x00000001;
    inline constexpr uint32_t kFrontRight = 0x00000002;
    inline constexpr uint32_t kStereo     = kFrontLeft | kFrontRight;
}

} // namespace dash::core
