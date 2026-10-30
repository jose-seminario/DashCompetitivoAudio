#pragma once

#include <DashAudioCore/AudioConstants.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace dash::spatial
{

/**
 * Roles acústicos normalizados de canal en sistemas de audio espacial e inmersivo.
 */
enum class ChannelRole
{
    FrontLeft,
    FrontRight,
    FrontCenter,
    LFE,
    SideLeft,
    SideRight,
    BackLeft,
    BackRight,
    TopFrontLeft,
    TopFrontRight,
    TopFrontCenter,
    TopBackLeft,
    TopBackRight,
    TopSideLeft,
    TopSideRight,
    DiscreteChannel
};

/**
 * Tipos de layout espacial normalizados.
 */
enum class LayoutType
{
    Unknown,
    Mono,
    Stereo,
    Surround5_1,
    Surround7_1,
    Immersive7_1_4,
    Immersive9_1_6,
    DiscreteCustom
};

/**
 * Coordenadas esféricas y metadatos de un altavoz virtual.
 */
struct SpeakerPosition
{
    ChannelRole role         = ChannelRole::DiscreteChannel;
    float       azimuthDeg   = 0.0f; // [-180.0 .. +180.0°] 0 = Frente, -30 = L, +30 = R
    float       elevationDeg = 0.0f; // [-90.0 .. +90.0°] 0 = Altura de oído, +35..+45 = Altavoces de techo
    float       distanceM    = 1.5f; // Distancia nominal de referencia en metros
    uint32_t    winMaskBit   = 0;    // Bit en WAVEFORMATEXTENSIBLE dwChannelMask
    const char* label        = "Discrete";
};

/**
 * Sistema de representación de Layouts Multicanal (ChannelLayout).
 *
 * Principios de diseño acústico:
 *  - Soporta hasta 16 canales discretos (kMaxSupportedChannels = 16).
 *  - Almacena coordenadas esféricas tridimensionales para renderizado HRTF.
 *  - Mapea máscaras oficiales de Windows (`dwChannelMask`) para interoperabilidad WASAPI/APO.
 *  - No inventa estándares ficticios: cuando un layout no es conocido, no adivina
 *    y etiqueta los canales como `DiscreteChannel` sin asignación espacial no fundamentada.
 */
class ChannelLayout
{
public:
    static constexpr size_t kMaxChannels = dash::core::kMaxSupportedChannels;

    ChannelLayout() noexcept;
    explicit ChannelLayout(LayoutType type, size_t numChannels = 0) noexcept;

    // Constructores de layouts estandarizados
    [[nodiscard]] static ChannelLayout mono() noexcept;
    [[nodiscard]] static ChannelLayout stereo() noexcept;
    [[nodiscard]] static ChannelLayout surround5_1() noexcept;
    [[nodiscard]] static ChannelLayout surround7_1() noexcept;
    [[nodiscard]] static ChannelLayout immersive7_1_4() noexcept;
    [[nodiscard]] static ChannelLayout immersive9_1_6() noexcept;
    [[nodiscard]] static ChannelLayout discrete(size_t numChannels) noexcept;
    [[nodiscard]] static ChannelLayout fromWindowsSpeakerMask(uint32_t mask) noexcept;

    [[nodiscard]] LayoutType getType() const noexcept { return type_; }
    [[nodiscard]] size_t getNumChannels() const noexcept { return numChannels_; }
    [[nodiscard]] uint32_t getWindowsMask() const noexcept { return windowsMask_; }
    [[nodiscard]] const SpeakerPosition& getSpeaker(size_t channelIndex) const noexcept;
    [[nodiscard]] int getChannelIndex(ChannelRole role) const noexcept;
    [[nodiscard]] bool hasRole(ChannelRole role) const noexcept;
    [[nodiscard]] bool isSpatialized() const noexcept;
    [[nodiscard]] std::string getLayoutName() const;

    bool setSpeaker(size_t channelIndex, const SpeakerPosition& speaker) noexcept;

private:
    LayoutType type_       = LayoutType::Unknown;
    size_t     numChannels_= 0;
    uint32_t   windowsMask_= 0;
    std::array<SpeakerPosition, kMaxChannels> speakers_ {};
};

} // namespace dash::spatial
