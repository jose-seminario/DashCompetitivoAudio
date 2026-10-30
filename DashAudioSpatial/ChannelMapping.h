#pragma once

#include "ChannelLayout.h"

#include <array>
#include <cstddef>

namespace dash::spatial
{

/**
 * Matriz de Enrutamiento y Remapeo de Canales (ChannelMapping).
 *
 * Conecta de forma determinista cualquier layout de entrada con el layout del procesador o renderizador,
 * sin suposiciones arbitrarias ni asignaciones de memoria dinámica en tiempo de audio.
 */
class ChannelMapping
{
public:
    static constexpr size_t kMaxChannels = ChannelLayout::kMaxChannels;

    ChannelMapping() noexcept;

    /**
     * Construye la matriz de coeficientes a partir de los roles de los altavoces de entrada y salida.
     */
    void configure(const ChannelLayout& sourceLayout, const ChannelLayout& targetLayout) noexcept;

    /**
     * Aplica la matriz de mezcla y enrutamiento en tiempo real.
     */
    void apply(
        const float* const* sourcePointers,
        float* const* targetPointers,
        int numSamples) const noexcept;

    [[nodiscard]] float getRoutingGain(size_t targetCh, size_t sourceCh) const noexcept;
    [[nodiscard]] const ChannelLayout& getSourceLayout() const noexcept { return sourceLayout_; }
    [[nodiscard]] const ChannelLayout& getTargetLayout() const noexcept { return targetLayout_; }
    [[nodiscard]] bool isPassThrough() const noexcept { return isIdentity_; }

private:
    ChannelLayout sourceLayout_;
    ChannelLayout targetLayout_;
    bool isIdentity_ = true;

    // Matriz de ganancias M[target][source]
    std::array<std::array<float, kMaxChannels>, kMaxChannels> matrix_ {};
};

} // namespace dash::spatial
