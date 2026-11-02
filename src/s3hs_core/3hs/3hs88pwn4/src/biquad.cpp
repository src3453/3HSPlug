#include "hs88/biquad.hpp"

#include <algorithm>

namespace hs88 {

void Biquad::setCoefficients(const BiquadCoefficients& coefficients, int fracBits) {
    c_ = coefficients;
    fracBits_ = fracBits;
}

void Biquad::setCoefficients(const std::int32_t (&coefficients)[5], int fracBits) {
    setCoefficients(BiquadCoefficients{coefficients[0], coefficients[1], coefficients[2], coefficients[3], coefficients[4]},
                    fracBits);
}

std::int32_t Biquad::process(std::int32_t in) {
    const std::int64_t acc = static_cast<std::int64_t>(c_.b0) * in + static_cast<std::int64_t>(c_.b1) * in1_ +
                             static_cast<std::int64_t>(c_.b2) * in2_ - static_cast<std::int64_t>(c_.a1) * out1_ -
                             static_cast<std::int64_t>(c_.a2) * out2_;
    const std::int64_t rounded = (acc + (std::int64_t{1} << (fracBits_ - 1))) >> fracBits_;
    const auto out = static_cast<std::int32_t>(std::clamp<std::int64_t>(rounded, -kLimit, kLimit));
    in2_ = in1_;
    in1_ = in;
    out2_ = out1_;
    out1_ = out;
    return out;
}

void Biquad::clearHistory() {
    in1_ = in2_ = out1_ = out2_ = 0;
}

}  // namespace hs88
