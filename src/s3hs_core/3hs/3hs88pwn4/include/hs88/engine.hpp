// 3HS88PWN4 sound engine: 8 harmonic channels (the 3HS816 core), 4 PCM/WT
// channels, mixer, EQ and compressor. Integer-only. Addresses are handled by
// RegisterInterface (registers.hpp).
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "hs816/channel.hpp"
#include "hs816/wavetables.hpp"
#include "hs88/master_effects.hpp"
#include "hs88/pcm_channel.hpp"

namespace hs88 {

constexpr int kHarmonicChannels = 8;
constexpr int kPcmChannels = 4;
constexpr int kChannelCount = kHarmonicChannels + kPcmChannels;  // HS 0..7, PCM 8..11
constexpr int kSampleRate = 48000;

// Optional per-channel taps: each channel's contribution after pan and mute,
// before the master effects, in DAC units and not saturated.
struct ChannelOutputs {
    std::array<std::int32_t*, kChannelCount> left{};
    std::array<std::int32_t*, kChannelCount> right{};
};

class Engine {
public:
    Engine();

    // Power-on: channels, PCM RAM, wavetables and effects.
    void reset();

    hs816::Channel& harmonic(int index) { return harmonic_[index]; }
    const hs816::Channel& harmonic(int index) const { return harmonic_[index]; }
    PcmChannel& pcm(int index) { return pcm_[index]; }
    const PcmChannel& pcm(int index) const { return pcm_[index]; }

    // A PCM/WT channel's 32-byte table also feeds harmonic waves A..D.
    void setTableByte(int pcmChannel, int position, std::uint8_t value);

    std::uint8_t readPcmMemory(std::uint32_t address) const;
    void writePcmMemory(std::uint32_t address, std::uint8_t value);

    void setMasterSettings(const MasterSettings& settings) { effects_.configure(settings); }
    const MasterSettings& masterSettings() const { return effects_.settings(); }

    const hs816::WaveTables& waves() const { return waves_; }

    void render(std::int16_t* left, std::int16_t* right, std::size_t frames,
                const ChannelOutputs* channelOutputs = nullptr);

private:
    std::array<hs816::Channel, kHarmonicChannels> harmonic_;
    std::array<PcmChannel, kPcmChannels> pcm_;
    hs816::WaveTables waves_;
    std::vector<std::uint8_t> memory_;
    MasterEffects effects_;
};

}  // namespace hs88
