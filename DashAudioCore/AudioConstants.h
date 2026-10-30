#pragma once

#include <cstddef>
#include <cstdint>

namespace dash::core
{

/**
 * Constante unificada de capacidad multicanal máxima para todo el motor DashAudioCore.
 * Admite configuraciones desde Mono (1.0), Estéreo (2.0), 5.1, 7.1, hasta formatos
 * inmersivos con altavoces de altura como 7.1.4 (12 canales) y 9.1.6 (16 canales).
 */
inline constexpr std::size_t kMaxSupportedChannels = 16;

/**
 * Frecuencias de muestreo nominales soportadas por el motor.
 */
inline constexpr double kSampleRate44100  = 44100.0;
inline constexpr double kSampleRate48000  = 48000.0;
inline constexpr double kSampleRate88200  = 88200.0;
inline constexpr double kSampleRate96000  = 96000.0;
inline constexpr double kSampleRate192000 = 192000.0;

/**
 * Máscaras canónicas de altavoces de Windows (compatibles con WAVEFORMATEXTENSIBLE y MMDevice).
 */
namespace SpeakerMask
{
    inline constexpr uint32_t kFrontLeft            = 0x00000001;
    inline constexpr uint32_t kFrontRight           = 0x00000002;
    inline constexpr uint32_t kFrontCenter          = 0x00000004;
    inline constexpr uint32_t kLowFrequency         = 0x00000008;
    inline constexpr uint32_t kBackLeft             = 0x00000010;
    inline constexpr uint32_t kBackRight            = 0x00000020;
    inline constexpr uint32_t kFrontLeftOfCenter    = 0x00000040;
    inline constexpr uint32_t kFrontRightOfCenter   = 0x00000080;
    inline constexpr uint32_t kBackCenter           = 0x00000100;
    inline constexpr uint32_t kSideLeft             = 0x00000200;
    inline constexpr uint32_t kSideRight            = 0x00000400;
    inline constexpr uint32_t kTopCenter            = 0x00000800;
    inline constexpr uint32_t kTopFrontLeft         = 0x00001000;
    inline constexpr uint32_t kTopFrontCenter       = 0x00002000;
    inline constexpr uint32_t kTopFrontRight        = 0x00004000;
    inline constexpr uint32_t kTopBackLeft          = 0x00008000;
    inline constexpr uint32_t kTopBackCenter        = 0x00010000;
    inline constexpr uint32_t kTopBackRight         = 0x00020000;

    // Layouts predefinidos
    inline constexpr uint32_t kMono                 = kFrontCenter;
    inline constexpr uint32_t kStereo               = kFrontLeft | kFrontRight;
    inline constexpr uint32_t kSurround5Point1      = kFrontLeft | kFrontRight | kFrontCenter | kLowFrequency | kSideLeft | kSideRight;
    inline constexpr uint32_t kSurround7Point1      = kFrontLeft | kFrontRight | kFrontCenter | kLowFrequency | kBackLeft | kBackRight | kSideLeft | kSideRight;
    inline constexpr uint32_t kImmersive7Point1Point4 = kSurround7Point1 | kTopFrontLeft | kTopFrontRight | kTopBackLeft | kTopBackRight;
}

} // namespace dash::core
