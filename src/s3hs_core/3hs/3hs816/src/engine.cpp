#include "hs816/engine.hpp"

#include <algorithm>

namespace hs816 {

namespace {

// Channel output (Q15) times pan (fifteenths) to DAC units. The reference
// scaled each channel by 255*127, divided pan by 15 and the mix by 32768,
// and sent a quarter of int16 per 1.0:
//   x/2^15 * 32385 * pan/15 / 2^15 * 32767/4 = x * pan * 2159 * 32767 >> 32
// (32385 / 15 = 2159 exactly).
constexpr std::int64_t kDacGain = 2159 * 32767;
constexpr int kDacShift = 32;

// Truncates toward zero like the reference's float-to-int conversion.
std::int32_t toDacUnits(std::int64_t weighted) {
    const std::int64_t scaled = weighted * kDacGain;
    return static_cast<std::int32_t>(scaled >= 0 ? scaled >> kDacShift : -((-scaled) >> kDacShift));
}

std::int16_t saturate(std::int32_t value) {
    return static_cast<std::int16_t>(std::clamp(value, -32768, 32767));
}

}  // namespace

Engine::Engine() {
    reset();
}

void Engine::reset() {
    for (Channel& channel : channels_) {
        channel.reset();
    }
    waves_.reset();
}

void Engine::setUserWaveSample(int wave, int position, std::uint8_t value) {
    waves_.setUserSample(wave, position, value);
}

void Engine::render(std::int16_t* left, std::int16_t* right, std::size_t frames,
                    const ChannelOutputs* channelOutputs) {
    for (std::size_t i = 0; i < frames; i++) {
        // Sum of channel output * pan; at most 16 * 15 * ~2^19.
        std::int64_t mixLeft = 0;
        std::int64_t mixRight = 0;
        for (int ch = 0; ch < kChannelCount; ch++) {
            Channel& channel = channels_[ch];
            // Muted channels still run so their state stays in step.
            const std::int64_t raw = channel.tick(waves_);
            const std::int64_t weightedLeft = channel.params().muted ? 0 : raw * channel.panGainLeft();
            const std::int64_t weightedRight = channel.params().muted ? 0 : raw * channel.panGainRight();
            mixLeft += weightedLeft;
            mixRight += weightedRight;
            if (channelOutputs) {
                if (std::int32_t* tap = channelOutputs->left[ch]) tap[i] = toDacUnits(weightedLeft);
                if (std::int32_t* tap = channelOutputs->right[ch]) tap[i] = toDacUnits(weightedRight);
            }
        }
        left[i] = saturate(toDacUnits(mixLeft));
        right[i] = saturate(toDacUnits(mixRight));
    }
}

}  // namespace hs816
