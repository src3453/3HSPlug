// Fixed-point direct-form-I biquad (WT/PCM channel filters, master EQ and the
// compressor detector). Coefficients are prenormalized by a0.
#pragma once

#include <cstdint>

namespace hs88 {

struct BiquadCoefficients {
    std::int32_t b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
};

class Biquad {
public:
    // Outputs saturate here; far above any audible level, and it keeps the
    // 64-bit accumulator from overflowing.
    static constexpr std::int32_t kLimit = 1 << 30;

    void setCoefficients(const BiquadCoefficients& coefficients, int fracBits);
    void setCoefficients(const std::int32_t (&coefficients)[5], int fracBits);
    // y = (b0 x + b1 x1 + b2 x2 - a1 y1 - a2 y2) >> frac, rounded to nearest.
    std::int32_t process(std::int32_t in);
    void clearHistory();

private:
    BiquadCoefficients c_;
    int fracBits_ = 0;
    std::int32_t in1_ = 0, in2_ = 0;
    std::int32_t out1_ = 0, out2_ = 0;
};

}  // namespace hs88
