#include "ChannelMapping.h"

#include <algorithm>
#include <cmath>

namespace dash::spatial
{

ChannelMapping::ChannelMapping() noexcept
{
    for (size_t t = 0; t < kMaxChannels; ++t)
    {
        for (size_t s = 0; s < kMaxChannels; ++s)
            matrix_[t][s] = (t == s) ? 1.0f : 0.0f;
    }
}

void ChannelMapping::configure(const ChannelLayout& sourceLayout, const ChannelLayout& targetLayout) noexcept
{
    sourceLayout_ = sourceLayout;
    targetLayout_ = targetLayout;

    for (size_t t = 0; t < kMaxChannels; ++t)
    {
        for (size_t s = 0; s < kMaxChannels; ++s)
            matrix_[t][s] = 0.0f;
    }

    const size_t numSrc = sourceLayout.getNumChannels();
    const size_t numDst = targetLayout.getNumChannels();

    // 1. Mapeo directo por identidad de roles acústicos
    for (size_t t = 0; t < numDst; ++t)
    {
        const auto dstRole = targetLayout.getSpeaker(t).role;
        const int srcIdx = sourceLayout.getChannelIndex(dstRole);

        if (srcIdx >= 0)
        {
            matrix_[t][static_cast<size_t>(srcIdx)] = 1.0f;
        }
    }

    // 2. Mapeos de compatibilidad acotados para formatos no equivalentes
    // Ejemplo: 7.1 de entrada hacia 5.1 de salida (BackLeft/BackRight mapeados a SideLeft/SideRight con -3 dB)
    if (sourceLayout.getType() == LayoutType::Surround7_1 && targetLayout.getType() == LayoutType::Surround5_1)
    {
        const int slDst = targetLayout.getChannelIndex(ChannelRole::SideLeft);
        const int srDst = targetLayout.getChannelIndex(ChannelRole::SideRight);
        const int blSrc = sourceLayout.getChannelIndex(ChannelRole::BackLeft);
        const int brSrc = sourceLayout.getChannelIndex(ChannelRole::BackRight);

        if (slDst >= 0 && blSrc >= 0)
            matrix_[static_cast<size_t>(slDst)][static_cast<size_t>(blSrc)] = 0.7071f; // -3 dB
        if (srDst >= 0 && brSrc >= 0)
            matrix_[static_cast<size_t>(srDst)][static_cast<size_t>(brSrc)] = 0.7071f; // -3 dB
    }

    // Comprobar si la matriz es identidad pura
    isIdentity_ = (numSrc == numDst);
    if (isIdentity_)
    {
        for (size_t t = 0; t < numDst; ++t)
        {
            for (size_t s = 0; s < numSrc; ++s)
            {
                const float expected = (t == s) ? 1.0f : 0.0f;
                if (std::abs(matrix_[t][s] - expected) > 1.0e-4f)
                {
                    isIdentity_ = false;
                    break;
                }
            }
            if (!isIdentity_)
                break;
        }
    }
}

void ChannelMapping::apply(
    const float* const* sourcePointers,
    float* const* targetPointers,
    int numSamples) const noexcept
{
    if (sourcePointers == nullptr || targetPointers == nullptr || numSamples <= 0)
        return;

    const size_t numSrc = sourceLayout_.getNumChannels();
    const size_t numDst = targetLayout_.getNumChannels();

    if (isIdentity_ && sourcePointers == targetPointers)
        return; // Sin cambios

    for (size_t t = 0; t < numDst; ++t)
    {
        float* dst = targetPointers[t];
        if (dst == nullptr)
            continue;

        for (int i = 0; i < numSamples; ++i)
        {
            float sum = 0.0f;
            for (size_t s = 0; s < numSrc; ++s)
            {
                const float gain = matrix_[t][s];
                if (gain > 1.0e-5f && sourcePointers[s] != nullptr)
                    sum += gain * sourcePointers[s][i];
            }
            dst[i] = sum;
        }
    }
}

float ChannelMapping::getRoutingGain(size_t targetCh, size_t sourceCh) const noexcept
{
    if (targetCh < kMaxChannels && sourceCh < kMaxChannels)
        return matrix_[targetCh][sourceCh];
    return 0.0f;
}

} // namespace dash::spatial
