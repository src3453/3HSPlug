// 3HS816 sound engine: 16 harmonic channels and the stereo mixer.
//
// The engine knows nothing about addresses; RegisterInterface (registers.hpp)
// decodes bus writes into these calls. Hosts that drive the chip from their
// own data (a tracker, a test) can call the engine directly.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "hs816/channel.hpp"
#include "hs816/constants.hpp"
#include "hs816/wavetables.hpp"

namespace hs816 {

// Optional per-channel taps (e.g. for oscilloscopes). Each non-null pointer
// receives `frames` values: that channel's contribution after pan and mute,
// in DAC units but not saturated.
struct ChannelOutputs {
    std::array<std::int32_t*, kChannelCount> left{};
    std::array<std::int32_t*, kChannelCount> right{};
};

class Engine {
public:
    Engine();

    // Power-on state for every channel and wavetable.
    void reset();

    Channel& channel(int index) { return channels_[index]; }
    const Channel& channel(int index) const { return channels_[index]; }

    void setUserWaveSample(int wave, int position, std::uint8_t value);
    const WaveTables& waves() const { return waves_; }

    // Render `frames` samples at kSampleRate. Output saturates to int16.
    void render(std::int16_t* left, std::int16_t* right, std::size_t frames,
                const ChannelOutputs* channelOutputs = nullptr);

private:
    std::array<Channel, kChannelCount> channels_;
    WaveTables waves_;
};

}  // namespace hs816
