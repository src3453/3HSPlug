#include "hs88/pcm_channel.hpp"

#include <algorithm>

#include "generated_tables.hpp"

namespace hs88 {

namespace {

// One table sample per 1500 Hz-samples: the table/PCM rate is 32x the
// frequency and the chip runs at 48 kHz (48000 / 32 = 1500).
constexpr std::uint64_t kHzPerSampleStep = 1500;

constexpr int kFilterFracBits = 28;
constexpr int kTrigFracBits = tables::kTrigFracBits;
constexpr std::int64_t kTrigOne = std::int64_t{1} << kTrigFracBits;
constexpr std::int64_t kLn2Q30 = 744261118;  // round(ln 2 * 2^30)

constexpr std::uint32_t kNoLoop = 0xFFFFFF;

int noiseBit(std::uint64_t index) {
    return (tables::kNoise[(index >> 5) & 0x7FF] >> (index & 31)) & 1;
}

std::int64_t divideRounded(std::int64_t numerator, std::int64_t denominator) {
    return numerator >= 0 ? (numerator + denominator / 2) / denominator
                          : -((-numerator + denominator / 2) / denominator);
}

// sinh(x) for x in Q30, 0 <= x < 1 (the band-pass argument stays below 0.71);
// the series to x^9 is accurate to about 1e-7 there.
std::int64_t sinhQ30(std::int64_t x) {
    const std::int64_t x2 = (x * x) >> kTrigFracBits;
    std::int64_t term = x;
    std::int64_t sum = x;
    for (int k = 2; k <= 8; k += 2) {
        term = ((term * x2) >> kTrigFracBits) / (k * (k + 1));
        sum += term;
    }
    return sum;
}

// RBJ biquad for the channel filter, as the reference computed per sample:
// omega = 2pi (cutoff + 1) * 8 / 48000; low/high-pass alpha = sin / (1 +
// (res + 1) / 16) (/ 64 for undefined modes); band-pass/notch alpha =
// sin * sinh(ln 2 * (res + 1) / 256 * omega / sin).
BiquadCoefficients filterCoefficients(std::uint8_t mode, std::uint8_t cutoff, std::uint8_t resonance) {
    const std::int64_t s = tables::kCutoffSin[cutoff];
    const std::int64_t c = tables::kCutoffCos[cutoff];
    const std::int64_t r = resonance;

    std::int64_t alpha;
    switch (static_cast<FilterMode>(mode)) {
    case FilterMode::LowPass:
    case FilterMode::HighPass:
        alpha = s * 16 / (17 + r);
        break;
    case FilterMode::BandPass:
    case FilterMode::Notch: {
        const std::int64_t x = (((kLn2Q30 * (r + 1)) >> 8) * tables::kCutoffOmegaOverSin[cutoff]) >> kTrigFracBits;
        alpha = (s * sinhQ30(x)) >> kTrigFracBits;
        break;
    }
    default:
        alpha = s * 64 / (65 + r);
        break;
    }

    std::int64_t b0, b1, b2;
    switch (static_cast<FilterMode>(mode)) {
    case FilterMode::HighPass:
        b0 = (kTrigOne + c) / 2;
        b1 = -(kTrigOne + c);
        b2 = b0;
        break;
    case FilterMode::BandPass:
        b0 = alpha;
        b1 = 0;
        b2 = -alpha;
        break;
    case FilterMode::Notch:
        b0 = kTrigOne;
        b1 = -2 * c;
        b2 = kTrigOne;
        break;
    default:  // low-pass, and the undefined modes
        b0 = (kTrigOne - c) / 2;
        b1 = kTrigOne - c;
        b2 = b0;
        break;
    }
    const std::int64_t a0 = kTrigOne + alpha;
    const std::int64_t a1 = -2 * c;
    const std::int64_t a2 = kTrigOne - alpha;
    // v / a0 in Q28: both are Q30, so scale v by 2^28 before dividing.
    const auto q28 = [a0](std::int64_t v) {
        return static_cast<std::int32_t>(divideRounded(v * (std::int64_t{1} << kFilterFracBits), a0));
    };
    return {q28(b0), q28(b1), q28(b2), q28(a1), q28(a2)};
}

}  // namespace

PcmChannel::PcmChannel() {
    reset();
}

void PcmChannel::reset() {
    params_ = PcmChannelParams{};
    position_ = 0;
    fraction_ = 0;
    fifoHead_ = fifoCount_ = 0;
    fifoLast_ = 0;
    filter_.clearHistory();
    setFrequency(0);
    setFilter(0, 0, 0);
    setPan(0, 0);
}

// F Hz plays F / 1500 table samples per output sample, F * 2^32 / 1500 in
// Q32, rounded up so exact sample boundaries are reached (see the 3HS816
// phase notes). 0 Hz stops the cursor.
void PcmChannel::setFrequency(std::uint16_t hz) {
    params_.frequency = hz;
    const std::uint64_t step = ((static_cast<std::uint64_t>(hz) << 32) + kHzPerSampleStep - 1) / kHzPerSampleStep;
    stepWhole_ = static_cast<std::uint32_t>(step >> 32);
    stepFraction_ = static_cast<std::uint32_t>(step);
}

void PcmChannel::setFilter(std::uint8_t mode, std::uint8_t cutoff, std::uint8_t resonance) {
    params_.filterMode = mode;
    params_.cutoff = cutoff;
    params_.resonance = resonance;
    filter_.setCoefficients(filterCoefficients(mode, cutoff, resonance), kFilterFracBits);
}

void PcmChannel::setPan(std::uint8_t left, std::uint8_t right) {
    params_.panLeft = left & 0x0F;
    params_.panRight = right & 0x0F;
    const bool centered = params_.panLeft == 0 && params_.panRight == 0;
    panGainLeft_ = centered ? 15 : params_.panLeft;
    panGainRight_ = centered ? 15 : params_.panRight;
}

void PcmChannel::sync() {
    position_ = 0;
    fraction_ = 0;
}

int PcmChannel::pushFifo(const std::uint8_t* data, std::size_t size) {
    const std::size_t room = kFifoCapacity - fifoCount_;
    const std::size_t accepted = std::min(room, size);
    for (std::size_t i = 0; i < accepted; i++) {
        fifo_[(fifoHead_ + fifoCount_ + i) % kFifoCapacity] = data[i];
    }
    fifoCount_ += accepted;
    return accepted < size ? -2 : static_cast<int>(fifoCount_);
}

std::uint32_t PcmChannel::tableAddress(int offset) const {
    return static_cast<std::uint32_t>(params_.table[offset]) << 16 | params_.table[offset + 1] << 8 |
           params_.table[offset + 2];
}

// The reference's PCM addressing, including its loop rule: past the end with
// a loop start below it (and not 0xFFFFFF), it reads start + index modulo
// (end - loop start), without adding the loop start. Otherwise it clamps to
// the inclusive end. Addresses outside the 4 MiB RAM read 0.
int PcmChannel::readPcm(const std::vector<std::uint8_t>& memory) const {
    const std::uint64_t start = tableAddress(0);
    const std::uint64_t end = tableAddress(3);
    const std::uint64_t loop = tableAddress(6);
    const auto at = [&memory](std::uint64_t address) {
        return address < memory.size() ? static_cast<int>(memory[address]) : 0;
    };
    std::uint64_t pre, next;
    if (start + position_ > end && loop < end && loop != kNoLoop) {
        const std::uint64_t length = end - loop;
        pre = start + position_ % length;
        next = start + (position_ + 1) % length;
    } else {
        pre = std::min(start + position_, end);
        next = std::min(start + position_ + 1, end);
    }
    const int a = at(pre);
    const int b = at(next);
    return (a * 65536 + (b - a) * static_cast<int>(fraction_ >> 16)) >> 16;
}

int PcmChannel::readSample(const std::vector<std::uint8_t>& memory) {
    const auto& table = params_.table;
    switch (static_cast<PcmMode>(params_.mode)) {
    case PcmMode::Pcm:
        return readPcm(memory);
    case PcmMode::WaveInterpolated: {
        const int a = table[position_ % kTableLength];
        const int b = table[(position_ + 1) % kTableLength];
        return (a * 65536 + (b - a) * static_cast<int>(fraction_ >> 16)) >> 16;
    }
    case PcmMode::Noise:
        return noiseBit(position_ % 65536) * 255;
    case PcmMode::ShortNoise:
        return noiseBit(position_ % 64) * 255;
    case PcmMode::Wave:
        return table[position_ % kTableLength];
    case PcmMode::Fifo:
        if (fifoCount_ > 0) {
            fifoLast_ = fifo_[fifoHead_];
            fifoHead_ = (fifoHead_ + 1) % kFifoCapacity;
            fifoCount_--;
        }
        return fifoLast_;
    default:
        return 0;  // centers to -128 like the reference
    }
}

std::int32_t PcmChannel::tick(const std::vector<std::uint8_t>& memory) {
    const std::uint64_t advanced = static_cast<std::uint64_t>(fraction_) + stepFraction_;
    fraction_ = static_cast<std::uint32_t>(advanced);
    position_ += stepWhole_ + (advanced >> 32);

    const int value = readSample(memory) - 128;
    // The filter runs on Q16 samples even while bypassed, so enabling it
    // starts from a settled history.
    const std::int32_t filtered = filter_.process(value * 65536);
    if (params_.cutoff == 0) {
        return value * params_.volume;
    }
    const std::int64_t scaled = (static_cast<std::int64_t>(filtered) * params_.volume) >> 16;
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(scaled, -32768, 32767));
}

}  // namespace hs88
