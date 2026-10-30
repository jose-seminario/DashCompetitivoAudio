#include "ChannelLayout.h"

#include <algorithm>

namespace dash::spatial
{

namespace
{
    const SpeakerPosition kDefaultDiscreteSpeaker {
        ChannelRole::DiscreteChannel, 0.0f, 0.0f, 1.5f, 0, "Discrete"
    };
}

ChannelLayout::ChannelLayout() noexcept
{
    speakers_.fill(kDefaultDiscreteSpeaker);
}

ChannelLayout::ChannelLayout(LayoutType type, size_t numChannels) noexcept
    : type_(type),
      numChannels_(std::min(numChannels, kMaxChannels))
{
    speakers_.fill(kDefaultDiscreteSpeaker);
}

ChannelLayout ChannelLayout::mono() noexcept
{
    ChannelLayout l(LayoutType::Mono, 1);
    l.windowsMask_ = dash::core::SpeakerMask::kMono;
    l.speakers_[0] = { ChannelRole::FrontCenter, 0.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kFrontCenter, "FC" };
    return l;
}

ChannelLayout ChannelLayout::stereo() noexcept
{
    ChannelLayout l(LayoutType::Stereo, 2);
    l.windowsMask_ = dash::core::SpeakerMask::kStereo;
    l.speakers_[0] = { ChannelRole::FrontLeft,  -30.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kFrontLeft,  "FL" };
    l.speakers_[1] = { ChannelRole::FrontRight, +30.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kFrontRight, "FR" };
    return l;
}

ChannelLayout ChannelLayout::surround5_1() noexcept
{
    ChannelLayout l(LayoutType::Surround5_1, 6);
    l.windowsMask_ = dash::core::SpeakerMask::kSurround5Point1;
    l.speakers_[0] = { ChannelRole::FrontLeft,   -30.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kFrontLeft,    "FL" };
    l.speakers_[1] = { ChannelRole::FrontRight,  +30.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kFrontRight,   "FR" };
    l.speakers_[2] = { ChannelRole::FrontCenter,   0.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kFrontCenter,  "FC" };
    l.speakers_[3] = { ChannelRole::LFE,           0.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kLowFrequency, "LFE" };
    l.speakers_[4] = { ChannelRole::SideLeft,   -110.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kSideLeft,     "SL" };
    l.speakers_[5] = { ChannelRole::SideRight,  +110.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kSideRight,    "SR" };
    return l;
}

ChannelLayout ChannelLayout::surround7_1() noexcept
{
    ChannelLayout l(LayoutType::Surround7_1, 8);
    l.windowsMask_ = dash::core::SpeakerMask::kSurround7Point1;
    l.speakers_[0] = { ChannelRole::FrontLeft,   -30.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kFrontLeft,    "FL" };
    l.speakers_[1] = { ChannelRole::FrontRight,  +30.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kFrontRight,   "FR" };
    l.speakers_[2] = { ChannelRole::FrontCenter,   0.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kFrontCenter,  "FC" };
    l.speakers_[3] = { ChannelRole::LFE,           0.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kLowFrequency, "LFE" };
    l.speakers_[4] = { ChannelRole::BackLeft,   -150.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kBackLeft,     "BL" };
    l.speakers_[5] = { ChannelRole::BackRight,  +150.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kBackRight,    "BR" };
    l.speakers_[6] = { ChannelRole::SideLeft,    -90.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kSideLeft,     "SL" };
    l.speakers_[7] = { ChannelRole::SideRight,   +90.0f, 0.0f, 1.5f, dash::core::SpeakerMask::kSideRight,    "SR" };
    return l;
}

ChannelLayout ChannelLayout::immersive7_1_4() noexcept
{
    ChannelLayout l = surround7_1();
    l.type_ = LayoutType::Immersive7_1_4;
    l.numChannels_ = 12;
    l.windowsMask_ = dash::core::SpeakerMask::kImmersive7Point1Point4;

    l.speakers_[8]  = { ChannelRole::TopFrontLeft,   -35.0f, 35.0f, 1.5f, dash::core::SpeakerMask::kTopFrontLeft,  "TFL" };
    l.speakers_[9]  = { ChannelRole::TopFrontRight,  +35.0f, 35.0f, 1.5f, dash::core::SpeakerMask::kTopFrontRight, "TFR" };
    l.speakers_[10] = { ChannelRole::TopBackLeft,   -145.0f, 35.0f, 1.5f, dash::core::SpeakerMask::kTopBackLeft,   "TBL" };
    l.speakers_[11] = { ChannelRole::TopBackRight,  +145.0f, 35.0f, 1.5f, dash::core::SpeakerMask::kTopBackRight,  "TBR" };
    return l;
}

ChannelLayout ChannelLayout::immersive9_1_6() noexcept
{
    ChannelLayout l = immersive7_1_4();
    l.type_ = LayoutType::Immersive9_1_6;
    l.numChannels_ = 16;

    // Altavoces Top Side y Front Wide en 16 canales
    l.speakers_[12] = { ChannelRole::TopSideLeft,    -90.0f, 45.0f, 1.5f, 0, "TSL" };
    l.speakers_[13] = { ChannelRole::TopSideRight,   +90.0f, 45.0f, 1.5f, 0, "TSR" };
    l.speakers_[14] = { ChannelRole::TopFrontCenter,   0.0f, 45.0f, 1.5f, dash::core::SpeakerMask::kTopFrontCenter, "TFC" };
    l.speakers_[15] = { ChannelRole::DiscreteChannel,  0.0f,  0.0f, 1.5f, 0, "EXT" };
    return l;
}

ChannelLayout ChannelLayout::discrete(size_t numChannels) noexcept
{
    const size_t clamped = std::min(numChannels, kMaxChannels);
    ChannelLayout l(LayoutType::DiscreteCustom, clamped);
    l.windowsMask_ = 0;
    return l;
}

ChannelLayout ChannelLayout::fromWindowsSpeakerMask(uint32_t mask) noexcept
{
    if (mask == dash::core::SpeakerMask::kMono)
        return mono();
    if (mask == dash::core::SpeakerMask::kStereo)
        return stereo();
    if (mask == dash::core::SpeakerMask::kSurround5Point1)
        return surround5_1();
    if (mask == dash::core::SpeakerMask::kSurround7Point1)
        return surround7_1();
    if (mask == dash::core::SpeakerMask::kImmersive7Point1Point4)
        return immersive7_1_4();

    // Si la máscara no es un layout estándar reconocido, creamos un layout discreto sin inferencias ficticias
    uint32_t count = 0;
    for (uint32_t m = mask; m > 0; m >>= 1)
        count += (m & 1);

    ChannelLayout l(LayoutType::DiscreteCustom, count);
    l.windowsMask_ = mask;
    return l;
}

const SpeakerPosition& ChannelLayout::getSpeaker(size_t channelIndex) const noexcept
{
    if (channelIndex < numChannels_)
        return speakers_[channelIndex];
    return kDefaultDiscreteSpeaker;
}

int ChannelLayout::getChannelIndex(ChannelRole role) const noexcept
{
    for (size_t i = 0; i < numChannels_; ++i)
    {
        if (speakers_[i].role == role)
            return static_cast<int>(i);
    }
    return -1;
}

bool ChannelLayout::hasRole(ChannelRole role) const noexcept
{
    return getChannelIndex(role) >= 0;
}

bool ChannelLayout::isSpatialized() const noexcept
{
    return type_ == LayoutType::Surround5_1
        || type_ == LayoutType::Surround7_1
        || type_ == LayoutType::Immersive7_1_4
        || type_ == LayoutType::Immersive9_1_6;
}

std::string ChannelLayout::getLayoutName() const
{
    switch (type_)
    {
        case LayoutType::Mono:            return "Mono (1.0)";
        case LayoutType::Stereo:          return "Stereo (2.0)";
        case LayoutType::Surround5_1:     return "Surround (5.1)";
        case LayoutType::Surround7_1:     return "Surround (7.1)";
        case LayoutType::Immersive7_1_4:  return "Immersive (7.1.4)";
        case LayoutType::Immersive9_1_6:  return "Immersive (9.1.6)";
        case LayoutType::DiscreteCustom:  return "Discrete (" + std::to_string(numChannels_) + " Channels)";
        case LayoutType::Unknown:
        default:                          return "Unknown";
    }
}

bool ChannelLayout::setSpeaker(size_t channelIndex, const SpeakerPosition& speaker) noexcept
{
    if (channelIndex >= kMaxChannels)
        return false;

    speakers_[channelIndex] = speaker;
    if (channelIndex >= numChannels_)
        numChannels_ = channelIndex + 1;

    return true;
}

} // namespace dash::spatial
