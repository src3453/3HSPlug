// The 16 operator waveforms: 12 built-in shapes and 4 user wavetables.
#pragma once

#include <array>
#include <cstdint>

#include "hs816/constants.hpp"

namespace hs816 {

class WaveTables {
public:
    WaveTables();

    // Rebuild the built-in shapes and clear the user wavetables.
    void reset();

    // Set one unsigned 8-bit sample of user wavetable `index` (wave 0xA+index).
    void setUserSample(int index, int position, std::uint8_t value);

    std::int8_t sample(int wave, int position) const { return tables_[wave][position]; }

private:
    void rebuildUserWave(int index);

    std::array<std::array<std::int8_t, kWaveTableLength>, kWaveCount> tables_{};
    std::array<std::array<std::uint8_t, kUserWaveLength>, kUserWaveCount> userSamples_{};
};

}  // namespace hs816
