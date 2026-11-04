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
