#include "hs816/wavetables.hpp"

#include "generated_tables.hpp"

namespace hs816 {

WaveTables::WaveTables() {
    reset();
}

void WaveTables::reset() {
    for (int wave = 0; wave < kWaveCount; wave++) {
        for (int i = 0; i < kWaveTableLength; i++) {
            tables_[wave][i] = tables::kWaves[wave][i];
        }
    }
    for (auto& samples : userSamples_) {
        samples.fill(0);
    }
    for (int index = 0; index < kUserWaveCount; index++) {
        rebuildUserWave(index);
    }
}

void WaveTables::setUserSample(int index, int position, std::uint8_t value) {
    userSamples_[index][position] = value;
    rebuildUserWave(index);
}

// Stretch the 32 unsigned samples to 256 points with linear interpolation
// (wrapping at the end) and center them on zero. The interpolated value lies
// between two non-negative samples, so the shift is the reference's
// truncation.
void WaveTables::rebuildUserWave(int index) {
    const auto& samples = userSamples_[index];
    auto& table = tables_[kFirstUserWave + index];
    for (int i = 0; i < kWaveTableLength; i++) {
        const int pre = samples[(i / 8) % kUserWaveLength];
        const int next = samples[(i / 8 + 1) % kUserWaveLength];
        const int value = (pre * 8 + (next - pre) * (i % 8)) >> 3;
        table[i] = static_cast<std::int8_t>(value - 128);
    }
}

}  // namespace hs816
