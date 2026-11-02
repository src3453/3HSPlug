// 3HS816 chip-wide constants. See doc/spec_3HS816.md.
//
// The chip is integer-only so the C++ model maps directly to RTL. Fixed-point
// formats are named Qn (n fractional bits).
#pragma once

#include <cstddef>
#include <cstdint>

namespace hs816 {

constexpr int kChannelCount = 16;
constexpr int kOperatorCount = 8;
constexpr int kWaveCount = 16;
constexpr int kAlgorithmCount = 13;

// Waves 0xA..0xD read the four user wavetables.
constexpr int kUserWaveCount = 4;
constexpr int kUserWaveLength = 32;
constexpr int kFirstUserWave = 10;
constexpr int kWaveTableLength = 256;

// Internal generation rate. The DAC runs at this rate; resampling to a host
// rate is the caller's job.
constexpr int kSampleRate = 48000;

// Operator ratios are Q12 (4096 = 1.0). OP1 always runs at 1.0.
constexpr std::uint16_t kRatioOne = 4096;

// Operator output: signed wave sample (-128..127) times amplitude (0..255),
// i.e. a Q15 signal with 1.0 = 32768.
constexpr int kSignalFracBits = 15;

// Envelope level: Q16, 0..65536 (1.0 inclusive).
constexpr std::int32_t kLevelOne = 1 << 16;

}  // namespace hs816
