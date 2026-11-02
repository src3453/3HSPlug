#include "hs816/envelope.hpp"

#include "generated_tables.hpp"
#include "hs816/constants.hpp"

namespace hs816 {

namespace {

// Segment lengths at 48 kHz in half-samples, which keeps them integral:
// attack = A * 0.33/64 s = A * 495 half-samples, decay/release = value *
// 1.5/64 s = value * 2250, and the zero-decay floor 0.5 * 1.5/64 s = 1125.
constexpr std::uint32_t kAttackUnit = 495;
constexpr std::uint32_t kDecayReleaseUnit = 2250;
constexpr std::uint32_t kMinDecay = 1125;

constexpr std::uint32_t kHalfSamplesPerSample = 2;
// Above the longest segment (255 * 2250 = 573750), so saturating here is
// indistinguishable from counting on; keeps the counter at 21 bits.
constexpr std::uint32_t kElapsedLimit = 1u << 20;

// t = elapsed / length as Q32, for elapsed < length.
std::uint64_t position(std::uint32_t elapsed, const EnvelopeSegment& segment) {
    const std::uint64_t t = (static_cast<std::uint64_t>(elapsed) * segment.reciprocal) >> 8;
    const std::uint64_t below_one = (std::uint64_t{1} << 32) - 1;
    return t < below_one ? t : below_one;
}

// from + (to - from) * (1 - 2^(-10 t)) for t = position / 2^32 in [0, 1).
std::int32_t exponential(std::int32_t from, std::int32_t to, std::uint64_t t) {
    const std::uint64_t y = t * 10;  // 2^(-y / 2^32)
    const auto whole = static_cast<unsigned>(y >> 32);
    const auto fraction = static_cast<std::uint32_t>(y);
    const unsigned index = fraction >> 24;
    const std::uint32_t weight = (fraction >> 8) & 0xFFFF;
    const std::uint32_t hi = tables::kExp2Neg[index];
    const std::uint32_t lo = tables::kExp2Neg[index + 1];
    const std::uint32_t power = hi - (((hi - lo) * weight) >> 16);  // Q16
    const std::int64_t shape = kLevelOne - static_cast<std::int64_t>(power >> whole);
    return from + static_cast<std::int32_t>((static_cast<std::int64_t>(to - from) * shape) >> 16);
}

}  // namespace

EnvelopeSegment EnvelopeSegment::ofLength(std::uint32_t halfSamples) {
    EnvelopeSegment segment;
    segment.length = halfSamples;
    if (halfSamples != 0) {
        segment.reciprocal = static_cast<std::uint32_t>(((std::uint64_t{1} << 40) + halfSamples - 1) / halfSamples);
    }
    return segment;
}

EnvelopeRates EnvelopeRates::fromRegisters(const EnvelopeRegisters& regs) {
    EnvelopeRates rates;
    rates.attack = EnvelopeSegment::ofLength(kAttackUnit * regs.attack);
    rates.decay = EnvelopeSegment::ofLength(regs.decay == 0 ? kMinDecay : kDecayReleaseUnit * regs.decay);
    rates.release = EnvelopeSegment::ofLength(kDecayReleaseUnit * regs.release);
    // S/255 in Q16 to within half an LSB, exact at 0 and 255.
    rates.sustain = regs.sustain * 257 + (regs.sustain >> 7);
    return rates;
}

void Envelope::restart() {
    elapsed_ = 0;
    stage_ = Stage::Attack;
}

void Envelope::release() {
    if (stage_ != Stage::Release) {
        elapsed_ = 0;
        stage_ = Stage::Release;
    }
    releaseFrom_ = level_;
}

// A finished segment hands its leftover time to the next one, as the
// reference did.
void Envelope::advance(const EnvelopeRates& rates) {
    switch (stage_) {
    case Stage::Attack:
        if (elapsed_ < rates.attack.length) {
            level_ = static_cast<std::int32_t>(position(elapsed_, rates.attack) >> 16);
            break;
        }
        elapsed_ -= rates.attack.length;
        stage_ = Stage::Decay;
        [[fallthrough]];

    case Stage::Decay:
        if (elapsed_ < rates.decay.length) {
            level_ = exponential(kLevelOne, rates.sustain, position(elapsed_, rates.decay));
            break;
        }
        elapsed_ -= rates.decay.length;
        stage_ = Stage::Sustain;
        [[fallthrough]];

    case Stage::Sustain:
        level_ = rates.sustain;
        break;

    case Stage::Release:
        level_ = elapsed_ < rates.release.length ? exponential(releaseFrom_, 0, position(elapsed_, rates.release)) : 0;
        break;
    }
    if (elapsed_ < kElapsedLimit) {
        elapsed_ += kHalfSamplesPerSample;
    }
}

}  // namespace hs816
