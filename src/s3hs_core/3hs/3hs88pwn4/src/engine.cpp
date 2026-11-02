#include "hs88/engine.hpp"

#include <algorithm>

namespace hs88 {

namespace {

// Mix weights to DAC units in Q32 (see doc/spec_3HS816.md):
// harmonic: Q15 output x pan: 32385/15 * 32767/4 / 2^30 -> 2159 * 32767;
// PCM: (sample - 128) * volume x pan / 15 / 32768 * 1.3 * 32767/4
//      -> round(1.3 * 32767 / (4 * 32768 * 15) * 2^32).
constexpr std::int64_t kHarmonicGain = 2159 * 32767;
constexpr std::int64_t kPcmGain = 93054785;
// The bus keeps 8 fraction bits below DAC units for the effects.
constexpr int kBusShift = 24;
constexpr int kBusFracBits = 8;

std::int64_t truncatedShift(std::int64_t value, int shift) {
    return value >= 0 ? value >> shift : -((-value) >> shift);
}

std::int32_t saturateBus(std::int64_t value) {
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(value, -Biquad::kLimit, Biquad::kLimit));
}

std::int16_t toDac(std::int32_t bus) {
    return static_cast<std::int16_t>(std::clamp<std::int64_t>(truncatedShift(bus, kBusFracBits), -32768, 32767));
}

}  // namespace

Engine::Engine() : memory_(kPcmMemorySize, 0) {
    reset();
}

void Engine::reset() {
    for (hs816::Channel& channel : harmonic_) {
        channel.reset();
    }
    for (PcmChannel& channel : pcm_) {
        channel.reset();
    }
    waves_.reset();
    std::fill(memory_.begin(), memory_.end(), std::uint8_t{0});
    effects_.reset();
}

void Engine::setTableByte(int pcmChannel, int position, std::uint8_t value) {
    pcm_[pcmChannel].setTableByte(position, value);
    waves_.setUserSample(pcmChannel, position, value);
}

std::uint8_t Engine::readPcmMemory(std::uint32_t address) const {
    return address < memory_.size() ? memory_[address] : 0;
}

void Engine::writePcmMemory(std::uint32_t address, std::uint8_t value) {
    if (address < memory_.size()) {
        memory_[address] = value;
    }
}

void Engine::render(std::int16_t* left, std::int16_t* right, std::size_t frames,
                    const ChannelOutputs* channelOutputs) {
    const auto tap = [channelOutputs](int ch, std::size_t i, std::int64_t weightedLeft, std::int64_t weightedRight) {
        if (!channelOutputs) return;
        if (std::int32_t* out = channelOutputs->left[ch]) out[i] = static_cast<std::int32_t>(truncatedShift(weightedLeft, 32));
        if (std::int32_t* out = channelOutputs->right[ch]) out[i] = static_cast<std::int32_t>(truncatedShift(weightedRight, 32));
    };

    for (std::size_t i = 0; i < frames; i++) {
        std::int64_t mixLeft = 0;
        std::int64_t mixRight = 0;
        for (int ch = 0; ch < kHarmonicChannels; ch++) {
            hs816::Channel& channel = harmonic_[ch];
            // Muted channels still run so their state stays in step.
            const std::int64_t raw = channel.tick(waves_);
            const bool audible = !channel.params().muted;
            const std::int64_t weightedLeft = audible ? raw * channel.panGainLeft() * kHarmonicGain : 0;
            const std::int64_t weightedRight = audible ? raw * channel.panGainRight() * kHarmonicGain : 0;
            mixLeft += weightedLeft;
            mixRight += weightedRight;
            tap(ch, i, weightedLeft, weightedRight);
        }
        for (int p = 0; p < kPcmChannels; p++) {
            PcmChannel& channel = pcm_[p];
            const std::int64_t raw = channel.tick(memory_);
            const bool audible = !channel.params().muted;
            const std::int64_t weightedLeft = audible ? raw * channel.panGainLeft() * kPcmGain : 0;
            const std::int64_t weightedRight = audible ? raw * channel.panGainRight() * kPcmGain : 0;
            mixLeft += weightedLeft;
            mixRight += weightedRight;
            tap(kHarmonicChannels + p, i, weightedLeft, weightedRight);
        }
        const StereoBus bus = {saturateBus(truncatedShift(mixLeft, kBusShift)),
                               saturateBus(truncatedShift(mixRight, kBusShift))};
        const StereoBus out = effects_.process(bus);
        left[i] = toDac(out.left);
        right[i] = toDac(out.right);
    }
}

}  // namespace hs88
