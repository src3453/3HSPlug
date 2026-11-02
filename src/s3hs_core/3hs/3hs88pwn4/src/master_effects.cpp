#include "hs88/master_effects.hpp"

#include <algorithm>

#include "generated_tables.hpp"

namespace hs88 {

namespace {

// 1.0 of the normalized bus in bus units: 32767/4 DAC steps, Q8.
constexpr std::int64_t kBusOne = 32767 * 64;

constexpr std::int64_t kGainOne = 1 << 16;
constexpr std::int64_t kGainMax = 256 * kGainOne;
constexpr std::int64_t kGainFall = 4 * kGainOne;

std::int32_t saturate(std::int64_t value) {
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(value, -Biquad::kLimit, Biquad::kLimit));
}

// value / 255 in Q16, within half an LSB and exact at 0 and 255.
std::int64_t byteFraction(std::uint8_t value) {
    return value * 257 + (value >> 7);
}

}  // namespace

void Equalizer::configure(std::uint8_t low, std::uint8_t mid, std::uint8_t high) {
    lowL_.setCoefficients(tables::kEq[0][low], tables::kEqFracBits);
    lowR_.setCoefficients(tables::kEq[0][low], tables::kEqFracBits);
    midL_.setCoefficients(tables::kEq[1][mid], tables::kEqFracBits);
    midR_.setCoefficients(tables::kEq[1][mid], tables::kEqFracBits);
    highL_.setCoefficients(tables::kEq[2][high], tables::kEqFracBits);
    highR_.setCoefficients(tables::kEq[2][high], tables::kEqFracBits);
}

StereoBus Equalizer::process(StereoBus in) {
    return {highL_.process(midL_.process(lowL_.process(in.left))),
            highR_.process(midR_.process(lowR_.process(in.right)))};
}

void Equalizer::reset() {
    for (Biquad* filter : {&lowL_, &lowR_, &midL_, &midR_, &highL_, &highR_}) {
        filter->clearHistory();
    }
}

std::int64_t SlewLimiter::process(std::int64_t target) {
    if (value_ < target) {
        value_ += std::min(target - value_, rise_);
    } else if (value_ > target) {
        value_ -= std::min(value_ - target, kGainFall);
    }
    value_ = std::clamp<std::int64_t>(value_, 0, kGainMax);
    return value_;
}

Compressor::Compressor() {
    detectorL_.setCoefficients(tables::kDetector, tables::kDetectorFracBits);
    detectorR_.setCoefficients(tables::kDetector, tables::kDetectorFracBits);
}

void Compressor::configure(std::uint8_t threshold, std::uint8_t rate, std::uint8_t volume) {
    threshold_ = (threshold * kBusOne + 127) / 255;
    volume_ = volume;
    gainL_.setRise(byteFraction(rate));
    gainR_.setRise(byteFraction(rate));
}

// Target gain = threshold / (50 Hz low-pass of |x|), capped at the 256 the
// slew limiter clamps to anyway. Silence asks for the cap (the reference's
// infinity) unless the threshold is 0 too (its NaN), which holds the gain; a
// negative detector overshoot asks for 0 (the reference's negative target).
std::int32_t Compressor::processSide(std::int32_t in, Biquad& detector, SlewLimiter& gain) const {
    const std::int64_t detected = detector.process(in < 0 ? -in : in);
    std::int64_t target;
    if (detected > 0) {
        target = std::min((threshold_ << 16) / detected, kGainMax);
    } else if (detected == 0) {
        target = threshold_ == 0 ? gain.value() : kGainMax;
    } else {
        target = 0;
    }
    const std::int64_t g = gain.process(target);
    return saturate((((static_cast<std::int64_t>(in) * g) >> 16) * volume_) >> 5);
}

StereoBus Compressor::process(StereoBus in) {
    return {processSide(in.left, detectorL_, gainL_), processSide(in.right, detectorR_, gainR_)};
}

void Compressor::reset() {
    detectorL_.clearHistory();
    detectorR_.clearHistory();
    gainL_.reset();
    gainR_.reset();
}

MasterEffects::MasterEffects() {
    configure(MasterSettings{});
}

void MasterEffects::configure(const MasterSettings& settings) {
    settings_ = settings;
    equalizer_.configure(settings.eqLow, settings.eqMid, settings.eqHigh);
    compressor_.configure(settings.threshold, settings.compressorRate, settings.outputVolume);
}

StereoBus MasterEffects::process(StereoBus in) {
    StereoBus out = in;
    if (settings_.equalizerEnabled) {
        out = equalizer_.process(out);
    }
    if (settings_.compressorEnabled) {
        out = compressor_.process(out);
    }
    return out;
}

void MasterEffects::reset() {
    equalizer_.reset();
    compressor_.reset();
    configure(MasterSettings{});
}

}  // namespace hs88
