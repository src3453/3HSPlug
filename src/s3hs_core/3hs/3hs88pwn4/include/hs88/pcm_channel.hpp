// One 3HS88PWN4 PCM/WT/noise channel with its 2-pole filter. Integer-only.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "hs88/biquad.hpp"

namespace hs88 {

constexpr std::size_t kPcmMemorySize = 0x400000;  // 4 MiB unsigned 8-bit
constexpr int kTableLength = 32;
constexpr std::size_t kFifoCapacity = 4096;

enum class PcmMode : std::uint8_t {
    Pcm = 0,             // RAM sample, linear interpolation
    WaveInterpolated = 1,
    Noise = 2,           // 65,536-entry noise
    ShortNoise = 3,      // first 64 noise entries
    Wave = 4,            // nearest sample
    Fifo = 5,            // one queued byte per sample
};

enum class FilterMode : std::uint8_t { LowPass = 0, HighPass = 1, BandPass = 2, Notch = 3 };

struct PcmChannelParams {
    std::uint16_t frequency = 0;  // Hz; PCM plays at 32x this rate
    std::uint8_t volume = 0;
    std::uint8_t mode = 0;
    std::uint8_t filterMode = 0;  // FilterMode; other values: low-pass, gentler resonance
    std::uint8_t cutoff = 0;      // 0 bypasses the filter (it still runs)
    std::uint8_t resonance = 0;
    std::uint8_t panLeft = 0;
    std::uint8_t panRight = 0;
    bool muted = false;
    // The 32 unsigned wave samples; in PCM mode bytes 0..8 are the big-endian
    // 24-bit start, end (inclusive) and loop start addresses.
    std::array<std::uint8_t, kTableLength> table{};
};

class PcmChannel {
public:
    PcmChannel();
    void reset();

    const PcmChannelParams& params() const { return params_; }

    void setFrequency(std::uint16_t hz);
    void setVolume(std::uint8_t volume) { params_.volume = volume; }
    void setMode(std::uint8_t mode) { params_.mode = mode; }
    void setFilter(std::uint8_t mode, std::uint8_t cutoff, std::uint8_t resonance);
    void setPan(std::uint8_t left, std::uint8_t right);
    void setMuted(bool muted) { params_.muted = muted; }
    void setTableByte(int position, std::uint8_t value) { params_.table[position] = value; }

    // Restart the cursor from position 0 (WTSync). Filter and FIFO keep state.
    void sync();

    // Queue bytes for FIFO mode. Returns the new queue length, or -2 if it
    // overflowed (as much as fits is kept).
    int pushFifo(const std::uint8_t* data, std::size_t size);
    std::size_t fifoLength() const { return fifoCount_; }

    // Advance one 48 kHz sample. Returns the channel output before pan, about
    // +-32640 at full scale (sample - 128 times volume).
    std::int32_t tick(const std::vector<std::uint8_t>& memory);

    int panGainLeft() const { return panGainLeft_; }
    int panGainRight() const { return panGainRight_; }

private:
    int readSample(const std::vector<std::uint8_t>& memory);
    int readPcm(const std::vector<std::uint8_t>& memory) const;
    std::uint32_t tableAddress(int offset) const;

    PcmChannelParams params_;
    // Cursor in table/PCM samples: whole part and Q32 fraction.
    std::uint64_t position_ = 0;
    std::uint32_t fraction_ = 0;
    std::uint32_t stepWhole_ = 0;
    std::uint32_t stepFraction_ = 0;
    Biquad filter_;
    std::array<std::uint8_t, kFifoCapacity> fifo_{};
    std::size_t fifoHead_ = 0;
    std::size_t fifoCount_ = 0;
    int fifoLast_ = 0;
    int panGainLeft_ = 15;
    int panGainRight_ = 15;
};

}  // namespace hs88
